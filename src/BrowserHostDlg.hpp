#pragma once

// forward declaration
class JSHook;
class CWebBrowser2;

// CBrowserHostDlg dialog: This dialog wraps the CWebBrowser2 object to provide a layer
// of abstraction for the main dialog and popup windows.
//

class CBrowserHostDlg : public CDialog
{
	DECLARE_DYNAMIC(CBrowserHostDlg)

public:
	CBrowserHostDlg(UINT explorerCtrlID, UINT nIDTemplate, CWnd* pParent = NULL);   // standard constructor
	virtual ~CBrowserHostDlg();

// for derived classes
protected:
	HWND getExplorerHwnd() const;
	void Navigate(LPCTSTR URL);
	void GoBack();
	void GoForward();
	void Stop();

	virtual CComObject<JSHook>* getHook()=0;
	virtual bool isHookActive()=0;

	virtual void onTitleChange(LPCTSTR lpszText) {}
	virtual void onWindowSetHeight(long height) {}
	virtual void onWindowSetWidth(long width) {}
	virtual void onOuterDocumentLoad(MSHTML::IHTMLDocument2Ptr doc) {}
	virtual void onNewWindow(CBrowserHostDlg** ppDlg) { }
	virtual void onClosing() { }


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual const AFX_EVENTSINKMAP* GetEventSinkMap() const;

	DECLARE_MESSAGE_MAP()

private:
	void Event_TitleChange(LPCTSTR lpszText);
	void Event_WindowSetHeight(long Height);
	void Event_WindowSetWidth(long Width);
	void Event_DocumentCompleteExplorer(LPDISPATCH pDisp, VARIANT* URL);
	void Event_NavigateComplete2Explorer(LPDISPATCH pDisp, VARIANT* URL);
	void Event_NewWindow2Explorer(LPDISPATCH* ppDisp, BOOL* Cancel);
	void Event_WindowClosing(VARIANT_BOOL IsChildWindow, VARIANT_BOOL *&Cancel);

	UINT					m_eventsinkmapEntryCount;
	AFX_EVENTSINKMAP		m_eventsinkMap;

	CWebBrowser2*			m_explorer;
};
