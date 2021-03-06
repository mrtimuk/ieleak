// BrowserHostDlg.cpp : implementation file
//

#include "stdafx.h"
#include "BrowserHostDlg.hpp"
#include "JSHook.hpp"
#include "WebBrowser2.hpp"
#include "resource.h"

#include <exdisp.h>
#include <exdispid.h>

// CBrowserHostDlg dialog

IMPLEMENT_DYNAMIC(CBrowserHostDlg, CDialog)
CBrowserHostDlg::CBrowserHostDlg(CComObject<JSHook>* hook, UINT explorerCtrlID, UINT nIDTemplate, CWnd* pParent /*=NULL*/)
	: CDialog(nIDTemplate, pParent) {
	m_hook = hook;
	m_hook->AddRef();

	m_explorer = new CWebBrowser2;

	//fixing a weird compiler warning on END_EVENTSINK_MAP() -- something like:
	//#pragma warning(pop) without a push
	#pragma warning(push)

	// The event sink is complicated somewhat by the fact that the ID of the web browser control is not known
	// until the class is instantiated.
	AFX_EVENTSINKMAP_ENTRY templateEventSink[] = {
		ON_EVENT(CBrowserHostDlg, explorerCtrlID, DISPID_COMMANDSTATECHANGE, CBrowserHostDlg::Event_CommandStateChange, VTS_I4 VTS_BOOL)
		ON_EVENT(CBrowserHostDlg, explorerCtrlID, DISPID_TITLECHANGE, CBrowserHostDlg::Event_TitleChange, VTS_BSTR)
		ON_EVENT(CBrowserHostDlg, explorerCtrlID, DISPID_WINDOWSETHEIGHT, CBrowserHostDlg::Event_WindowSetHeight, VTS_I4)
		ON_EVENT(CBrowserHostDlg, explorerCtrlID, DISPID_WINDOWSETWIDTH, CBrowserHostDlg::Event_WindowSetWidth, VTS_I4)
		ON_EVENT(CBrowserHostDlg, explorerCtrlID, DISPID_DOCUMENTCOMPLETE/*259*/, CBrowserHostDlg::Event_DocumentCompleteExplorer, VTS_DISPATCH VTS_PVARIANT)
		ON_EVENT(CBrowserHostDlg, explorerCtrlID, DISPID_NAVIGATECOMPLETE2/*252*/, CBrowserHostDlg::Event_NavigateComplete2Explorer, VTS_DISPATCH VTS_PVARIANT)
		ON_EVENT(CBrowserHostDlg, explorerCtrlID, DISPID_NEWWINDOW2/*251*/, CBrowserHostDlg::Event_NewWindow2Explorer, VTS_PDISPATCH VTS_PBOOL)
		ON_EVENT(CBrowserHostDlg, explorerCtrlID, DISPID_WINDOWCLOSING, CBrowserHostDlg::Event_WindowClosing, VTS_BOOL VTS_BOOL)
	END_EVENTSINK_MAP()

	// Clone the template event sink
	AFX_EVENTSINKMAP_ENTRY *entries = new AFX_EVENTSINKMAP_ENTRY[sizeof(templateEventSink)/sizeof(templateEventSink[0])];
	memcpy(entries, &templateEventSink, sizeof(templateEventSink));
	m_eventsinkmapEntryCount = -1;

	m_eventsinkMap.pBaseMap = NULL;
	m_eventsinkMap.lpEntries = entries;
	m_eventsinkMap.lpEntryCount = &m_eventsinkmapEntryCount;
}

CBrowserHostDlg::~CBrowserHostDlg() {
	delete m_explorer;
	m_explorer = NULL;

	delete[] m_eventsinkMap.lpEntries;
	m_eventsinkMap.lpEntries = NULL;
	m_eventsinkMap.lpEntryCount = NULL;

	// Release the hook
	//
	m_hook->Release();
}

void CBrowserHostDlg::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CBrowserHostDlg)
	DDX_Control(pDX, IDC_EXPLORER, *m_explorer);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBrowserHostDlg, CDialog)
END_MESSAGE_MAP()


const AFX_EVENTSINKMAP* CBrowserHostDlg::GetEventSinkMap() const {
	return &m_eventsinkMap;
}

HWND CBrowserHostDlg::getExplorerHwnd() const {
	return m_explorer->m_hWnd;
}

void CBrowserHostDlg::Navigate(LPCTSTR URL) {
	m_explorer->Navigate(URL, 0, 0, 0, 0);
}

void CBrowserHostDlg::GoBack() {
	m_explorer->GoBack();
}

void CBrowserHostDlg::GoForward() {
	m_explorer->GoForward();
}

void CBrowserHostDlg::Stop() {
	m_explorer->Stop();
}

LPDISPATCH CBrowserHostDlg::getDocument() {
	return m_explorer->GetDocument();
}

void CBrowserHostDlg::Event_CommandStateChange(long lCommand, BOOL bEnable) {
	bool enable = bEnable ? true : false;

	switch (lCommand) {
		case CSC_NAVIGATEFORWARD:
			onUpdateNavigateForward(enable);
			break;

		case CSC_NAVIGATEBACK:
			onUpdateNavigateBack(enable);
			break;
	}
}

void CBrowserHostDlg::Event_TitleChange(LPCTSTR lpszText) {
	onTitleChange(lpszText);
}

void CBrowserHostDlg::Event_WindowSetHeight(long Height) {
	onWindowSetHeight(Height);
}

void CBrowserHostDlg::Event_WindowSetWidth(long Width) {
	onWindowSetWidth(Width);
}

// This event is fired whenever the document (or any of its frames) is fully
//   loaded (meaning that its HTML has been parsed and all elements created).
//
void CBrowserHostDlg::Event_DocumentCompleteExplorer(LPDISPATCH pDisp, VARIANT* URL) {
	// pDisp is the IWebBrowser2 control sending the DocumentComplete event.
	//   (it may not be the main control, if the document being loaded
	//   contains frames).
	//
	IWebBrowser2* sender = NULL;
	pDisp->QueryInterface(IID_IWebBrowser2, (void**)&sender);

	// Get the document interface from the browser control.
	//
	IDispatch* dispDoc = NULL;
	sender->get_Document(&dispDoc);
	if (!dispDoc) {
		sender->Release();
		return;
	}

	// The document pointer may be NULL if the user navigates to a folder on the hard drive
	//
	MSHTML::IHTMLDocument2Ptr doc = dispDoc;
	if (doc == NULL) {
		dispDoc->Release();
		return;
	}

	// If we're waiting for a normal document, hook all of its static elements.
	//
	if (isHookActive())
		getHook()->addStaticNodes(doc->parentWindow);

	// Determine whether the completed document is the outer one that we are
	//   actually waiting on (by comparing its URL with the outer browser's).
	//
	if (isOuterLocation(sender))
		onOuterDocumentLoad(doc);

	dispDoc->Release();
	sender->Release();
}

// This event is fired when the document has been downloaded but not yet parsed
//   (nor its onload event fired).
//
void CBrowserHostDlg::Event_NavigateComplete2Explorer(LPDISPATCH pDisp, VARIANT* URL) {
	// If we're waiting on the document (but not automatically refreshing), hook its
	//   createElement() method, so that we can collect all dynamically-
	//   created elements.
	//
	if (isHookActive()) {
		IWebBrowser2* sender = NULL;
		pDisp->QueryInterface(IID_IWebBrowser2, (void**)&sender);

		IDispatch* dispDoc = NULL;
		sender->get_Document(&dispDoc);
		MSHTML::IHTMLDocument2Ptr doc = dispDoc;

		if (isOuterLocation(sender))
			onURLChange(m_explorer->GetLocationURL());

		// The document pointer may be NULL if the user navigates to a folder on the hard drive
		//
		if (doc)
			getHook()->hookNewPage(doc);

		dispDoc->Release();
		sender->Release();
	}
}

void CBrowserHostDlg::Event_NewWindow2Explorer(LPDISPATCH* ppDisp, BOOL* Cancel) {
	// See http://support.microsoft.com/kb/q184876/
	//
	CBrowserHostDlg* dlg = NULL;
	onNewWindow(&dlg);
	if (dlg != NULL) {
		*Cancel = false;
		*ppDisp = dlg->m_explorer->GetApplication();
	}
	else
		*Cancel = true;
}

void CBrowserHostDlg::Event_WindowClosing(VARIANT_BOOL IsChildWindow, VARIANT_BOOL *&Cancel) {
	onClosing();
}

bool CBrowserHostDlg::isOuterLocation(IWebBrowser2* sender) {
	BSTR loadedLocation = NULL;
	sender->get_LocationURL(&loadedLocation);

	CString outerLocation = m_explorer->GetLocationURL();
	bool isOuter = (0 == outerLocation.Compare(loadedLocation));

	SysFreeString(loadedLocation);

	return isOuter;
}
