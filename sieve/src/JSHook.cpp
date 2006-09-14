#include "stdafx.h"
#include "JSHook.hpp"
#include "LeakDlg.hpp"
#include "PropDlg.hpp"
#include "HtmlResource.h"
#include "MainBrowserDlg.hpp"

JSHook::JSHook() {
	m_itNextNode = m_nodes.begin();
	m_leakDlg = NULL;
}

JSHook::~JSHook() {
	// When the hook is destroyed, make sure all nodes are released.
	//
	clearNodes();
}

void JSHook::setMainBrowserDlg(CMainBrowserDlg* dlg)
{
	m_mainBrowserDlg = dlg;
}

// IDispatch
//
STDMETHODIMP JSHook::GetTypeInfoCount(UINT *pctinfo) {
	// Don't need to provide type info to work in IE.
	//
	return E_NOTIMPL;
}

STDMETHODIMP JSHook::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo) {
	// Don't need to provide type info to work in IE.
	//
	return E_NOTIMPL;
}

STDMETHODIMP JSHook::GetIDsOfNames(REFIID iid, OLECHAR **names, UINT nameCount, LCID lcid, DISPID *dispIds) {
	// members are 'logNode' and 'unloadWindow' and 'rescanForNodes'
	//
	bool failed = false;
	for (int i = 0; i < (int)nameCount; ++i) {
		if (!wcscmp(names[i], L"logNode"))
			dispIds[i] = 0;
		else if (!wcscmp(names[i], L"unloadWindow"))
			dispIds[i] = 1;
		else if (!wcscmp(names[i], L"rescanForNodes"))
			dispIds[i] = 2;
		else if (!wcscmp(names[i], L"logDetectedCycle"))
			dispIds[i] = 3;
		else if (!wcscmp(names[i], L"logMessage"))
			dispIds[i] = 4;
		else	
		{
			dispIds[i] = -1;
			failed = true;
		}
	}

	if (failed)
		return DISP_E_MEMBERNOTFOUND;
	return S_OK;
}

STDMETHODIMP JSHook::Invoke(DISPID dispId, REFIID riid, LCID lcid, WORD flags, DISPPARAMS *dispParams, VARIANT *result, EXCEPINFO *exInfo, UINT *argErr) {


	if ( dispId == 0 )
	{
		// logNode() takes one argument, elem
		//
		if (dispParams->cArgs != 1)
			return DISP_E_BADPARAMCOUNT;

		if (dispParams->rgvarg[0].vt != VT_DISPATCH)
			return DISP_E_BADVARTYPE;

		// Get the node, and add the node to the list.
		//
		MSHTML::IHTMLDOMNodePtr node = dispParams->rgvarg[0].pdispVal;
		addNode(node);
		return S_OK;
	}
	else if ( dispId == 1 )
	{
		// unloadWindow
		MSHTML::IHTMLDocument2Ptr doc = dispParams->rgvarg[0].pdispVal;
		unloadWindow(doc);
		return S_OK;
	}
	else if ( dispId == 2 )
	{
		// rescanForNodes
		if (dispParams->cArgs == 1)
		{
			MSHTML::IHTMLDocument2Ptr doc = dispParams->rgvarg[0].pdispVal;
			rescanForNodes(doc);
		}
		else
		{
			rescanForNodes(NULL);
		}
		return S_OK;
	}
	else if ( dispId == 3 )
	{
		// logDetectedCycle() takes one argument, elem
		//
		if (dispParams->cArgs != 1)
			return DISP_E_BADPARAMCOUNT;

		if (dispParams->rgvarg[0].vt != VT_DISPATCH)
			return DISP_E_BADVARTYPE;

		// Get the node, and add the node to the list.
		//
		MSHTML::IHTMLDOMNodePtr node = dispParams->rgvarg[0].pdispVal;
		Elem* elem = getElement(node);
		if ( elem )
		{
			elem->cycleDetected = true;
		}
		return S_OK;
	}
	else if ( dispId == 4 )
	{
		// logMessage() takes one argument, str
		//
		m_mainBrowserDlg->logMessage(dispParams->rgvarg[0].bstrVal);
		return S_OK;
	}
	else 
		return DISP_E_MEMBERNOTFOUND;
}

// Add an node to the hook's list.  The node's URL will be retrieved from
//  its current document.
//
void JSHook::addNode(MSHTML::IHTMLDOMNode* node) {

	if ( node == NULL || node->nodeType == NODE_TEXT )
		return; // No need to register NODE_TEXT; They never leak;

	// In order to ensure that we maintain a durable reference to the node (as
	//   opposed to a tear-off interface), we have to query for IUnknown.
	//
	IUnknown* unkNode = NULL;

	// Do not register the node twice; doing so will cause it to be incorrectly reported as a leak.
	// This happens when an node is created by createElement and is added to the DOM before the DOM
	// is traversed. (Note that the createElement hook must be in place immediately in case the caller
	// saves a references to the node and adds it to the DOM after the page is loaded.) 
	//
	if ( node ) node->QueryInterface(IID_IUnknown, (void**)&unkNode);

	if ( unkNode )
	{
		if (m_nodes.find(unkNode) == m_nodes.end())
		{
			MSHTML::IHTMLDOMNode2Ptr	node2= node;
			MSHTML::IHTMLDocument2Ptr	doc = node2->ownerDocument;
			MSHTML::IHTMLWindow2Ptr		wnd = doc ? doc->parentWindow : NULL;
			BSTR nodeName = NULL;
			GetPropertyValueByName((CComQIPtr<IDispatchEx>)node, L"nodeName", &nodeName);	
			if ( doc )
			{
				int docId = getDocumentId(doc);
				BSTR url = NULL;
				if (wnd == NULL || ! GetLibraryURL((CComQIPtr<IDispatchEx>)wnd, &url) )  // Cordys Specific test
				{
					url = SysAllocString(doc->url);
				}
				Elem *cachedElem = new Elem(unkNode, url, nodeName);
				cachedElem->docId = docId;
				cachedElem->running = true;
				m_nodes.insert(std::pair<IUnknown*,Elem*>(unkNode,cachedElem));
				hookNewNode(node,doc);
			}
			else
			{
				Elem* cachedElem = new Elem(unkNode, SysAllocString(L"N/A"), nodeName);
				m_nodes.insert(std::pair<IUnknown*,Elem*>(unkNode,cachedElem));
			}
		}
		else
		{
			unkNode->Release();
		}
	}
}

Elem* JSHook::getElement(MSHTML::IHTMLDOMNodePtr node)
{
	if ( node )
	{
		IUnknown *unkNode;
		node->QueryInterface(IID_IUnknown, (void**)&unkNode);		
		std::map<IUnknown*,Elem*>::iterator pair = m_nodes.find(unkNode);
		unkNode->Release();

		if ( pair == m_nodes.end() ) return NULL;
		return pair->second;
	}
	return NULL;
}

void JSHook::hookNewNode(MSHTML::IHTMLDOMNodePtr node, MSHTML::IHTMLDocument2Ptr doc ) {

	if ( node->nodeType != NODE_TEXT )
	{
		// Create a parameter list containing the hook, then invoke the
		//   temporary function to attach it to the document.
		//
		VARIANT vHook;
		VariantInit(&vHook);
		vHook.vt = VT_DISPATCH;
		vHook.pdispVal = node;
		node->AddRef();

		DISPPARAMS params;
		memset(&params, 0, sizeof(DISPPARAMS));
		params.cArgs = 1;
		params.rgvarg = &vHook;

		CComPtr<IDispatch> scriptObj = doc->Script;

		DISPID dispId;
		OLECHAR *name = SysAllocString(L"__sIEve_hookNode");
		scriptObj->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_SYSTEM_DEFAULT, &dispId);
		SysFreeString(name);
		scriptObj->Invoke(dispId, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, NULL, NULL, NULL);

		VariantClear(&vHook);
	}
}

void JSHook::crossRefScan(MSHTML::IHTMLDocument2Ptr doc, CButton* button)
{
	int i = 0;
	TCHAR buttonTxt[50];
	TCHAR oldButtonTxt[50];
	int docId = getDocumentId(doc);

	if ( button ) button->GetWindowTextW(oldButtonTxt,50);
	for (std::map<IUnknown*,Elem*>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it) {
		IUnknown *unkNode = it->first;
		Elem* elem = it->second;

		MSHTML::IHTMLDOMNodePtr node = unkNode;
		if ( node )
		{
			// If 'doc' is NULL then Scan all nodes of all documents
			// Otherwise only scan the nodes of a specified doc;
			if ( (doc == NULL || elem->docId == docId) && elem->running && node->nodeType != NODE_TEXT)
			{
				if ( button )
				{
					wsprintf(buttonTxt,L"Scan: %d",++i);
					button->SetWindowTextW(buttonTxt);
				}
				crossRefScanNode(node);
			}
		}
	}
	if ( button ) button->SetWindowTextW(oldButtonTxt);
}

void JSHook::crossRefScanNode(MSHTML::IHTMLDOMNode2Ptr node)
{
	// Create a parameter list containing the hook, then invoke the
	//   temporary function to attach it to the document.
	//
	VARIANT vHook;
	VariantInit(&vHook);
	vHook.vt = VT_DISPATCH;
	vHook.pdispVal = node;
	node->AddRef();

	DISPPARAMS params;
	memset(&params, 0, sizeof(DISPPARAMS));
	params.cArgs = 1;
	params.rgvarg = &vHook;


	MSHTML::IHTMLDocument2Ptr doc = node->ownerDocument;
	CComPtr<IDispatch> scriptObj = doc->Script;

	DISPID dispId;
	OLECHAR *name = SysAllocString(L"___sIEve_crossRefScan");
	scriptObj->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_SYSTEM_DEFAULT, &dispId);
	SysFreeString(name);
	scriptObj->Invoke(dispId, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, NULL, NULL, NULL);

	VariantClear(&vHook);
}




// This method is called when a document is finished loading.  It will hook the
//   document's createElement() method, passing all dynamically-created nodes
//   to the hook as they are created.
//
void JSHook::hookNewPage(MSHTML::IHTMLDocument2Ptr doc) {
	if ( doc != NULL && wcscmp(doc->url,L"about:blank") )
	{
		MSHTML::IHTMLWindow2Ptr wnd = doc->parentWindow;

		IUnknown* unkWnd = NULL;
		wnd->QueryInterface(IID_IUnknown, (void**)&unkWnd);
		if ( unkWnd ) unkWnd->Release();
		if ( unkWnd == NULL || m_hookedwindows.find(unkWnd) != m_hookedwindows.end() )
		{
			// Log exceptional case;
			BSTR url;
			if (wnd == NULL || ! GetLibraryURL((CComQIPtr<IDispatchEx>)wnd, &url) )  // Cordys Specific test
			{
				url = SysAllocString(doc->url);
			}
			CStringW x;
			x.Format(L"ALREADY HOOKED %s",(LPCTSTR) url);
			m_mainBrowserDlg->logMessage(x);
			SysFreeString(url);
			return;
		}
		else
		{
			m_hookedwindows.insert(std::pair<IUnknown*,IUnknown*>(unkWnd,unkWnd));
		}

		if ( !m_js.GetLength())
			VERIFY(GetHTML(IDR_SIEVEHOOKS_JS, m_js));

		// Free up extra references to avoid a perpetual increase in memory usage
		//
		releaseExtraReferences(wnd);

		bstr_t js = m_js.GetBuffer();
		wnd->execScript(js, L"javascript");
		m_js.ReleaseBuffer();

		// Create a parameter list containing the hook, then invoke the
		//   temporary function to attach it to the document.
		//
		VARIANT vHook;
		VariantInit(&vHook);
		vHook.vt = VT_DISPATCH;
		vHook.pdispVal = this;
		this->AddRef();

		DISPPARAMS params;
		memset(&params, 0, sizeof(DISPPARAMS));
		params.cArgs = 1;
		params.rgvarg = &vHook;

		CComPtr<IDispatch> scriptObj = doc->Script;

		DISPID dispId;
		BSTR name = SysAllocString(L"__sIEve_initializeHooks");
		scriptObj->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_SYSTEM_DEFAULT, &dispId);
		SysFreeString(name);
		scriptObj->Invoke(dispId, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, NULL, NULL, NULL);

		VariantClear(&vHook);

		addStaticNodes(doc);
	}
}

// Add all nodes recursively from a given root node.
//
void JSHook::addNodeRecursively(MSHTML::IHTMLDOMNode* node) {
	if ( node )
	{
		addNode(node);

		MSHTML::IHTMLDOMNodePtr child = node->firstChild;
		while (child) {
			addNodeRecursively(child);
			child = child->nextSibling;
		}
	}
}

// Add all nodes within a window, starting with its document's body.
//
void JSHook::addStaticNodes(MSHTML::IHTMLDocument2Ptr doc) {
	MSHTML::IHTMLWindow2Ptr wnd = doc->parentWindow;
	if ( wcscmp(doc->url,L"about:blank") )
	{
		MSHTML::IHTMLDocument3Ptr document = doc;
		MSHTML::IHTMLDOMNodePtr documentElementNode = document->documentElement;
		addNodeRecursively(documentElementNode);
	}
}

void JSHook::unloadWindow(MSHTML::IHTMLDocument2Ptr doc)
{
	MSHTML::IHTMLWindow2Ptr wnd = doc->parentWindow;
	int docId = getDocumentId(doc);

	rescanForNodes(doc);
	if ( m_mainBrowserDlg->m_check_cycle_detection )
	{
		crossRefScan(doc,(CButton*)(m_mainBrowserDlg->GetDlgItem(IDC_CROSSREF_SCAN)));
	}

	std::map<IUnknown*,Elem*>::iterator it = m_nodes.begin();
	while (it != m_nodes.end())
	{
		std::map<IUnknown*,Elem*>::iterator next = it; next++; // Save the next one.
		Elem* elem = it->second;
		if ( elem->docId == docId )		{
			elem->running = false;
		}
		it = next;
	}

	IUnknown* unkWnd = NULL;
	wnd->QueryInterface(IID_IUnknown, (void**)&unkWnd);
	if ( unkWnd )
	{
		unkWnd->Release();
		std::map<IUnknown*,IUnknown*>::iterator it = m_hookedwindows.find(unkWnd);
		if ( it != m_hookedwindows.end())
		{
			m_hookedwindows.erase(it);
		}
	}	
}

int JSHook::getDocumentId(MSHTML::IHTMLDocument2Ptr doc)
{
	BSTR __sieve_documentId = NULL;
	int docId = 0;
	if ( doc )
	{
		MSHTML::IHTMLWindow2Ptr wnd = doc->parentWindow;
		if ( GetPropertyValueByName((CComQIPtr<IDispatchEx>)wnd, L"__sieve_documentId", &__sieve_documentId) )
		{
			docId = _wtoi(__sieve_documentId);
			SysFreeString(__sieve_documentId);
		}
	}
	return docId;
}

void JSHook::rescanForNodes(MSHTML::IHTMLDocument2Ptr doc)
{
	int docId = getDocumentId(doc);

	std::map<IUnknown*,Elem*>::iterator end = m_nodes.end();
	for (std::map<IUnknown*,Elem*>::iterator it = m_nodes.begin(); it != end; ++it) {
		IUnknown *unkNode = it->first;
		MSHTML::IHTMLElementPtr	node = unkNode;

		if ( node )
		{
			Elem* elem = it->second;

			// If 'doc' is NULL then Scan all nodes of all documents
			// Otherwise only scan the nodes of a specified doc;
			if ( (doc == NULL || elem->docId == docId) && elem->running )
			{
				unkNode->AddRef();
				int refCount = unkNode->Release();

				// Only rescan the nodes which are still in use (ie having references besides our own reference)
				if ( refCount > 1 )
				{	
					// Only scan the nodes without having a parent Element (Not parent Node; because they will have a parentNode (the documentElement node will have the document as parentNode)
					// Otherwise all we do way too much redundant scanning
					if ( node->parentElement == NULL )
					{
						MSHTML::IHTMLDOMNodePtr node = unkNode;
						addNodeRecursively(node);
					}
				}
				else
				{
						//AfxMessageBox(L"Found Refcount <= 1 ");
				}
			}

			// Search for "contentWindow" property; In that case it is a "iframe" or "frame" node
			MSHTML::IHTMLDocument2Ptr iFrameDoc = GetContentDocument((CComQIPtr<IDispatchEx>)node);
			if ( iFrameDoc )
			{
				// If it is an frame object then check if the document is already hooked;
				// The reason to check again for 'unhooked' documents is that some documents are loaded
				// in a synchroneous with document.write("..."); Thus not downloaded from a backend !
				// These documents will not fire the Event_NavigateComplete2Explorer

				int docId = getDocumentId(iFrameDoc);
				if ( docId == 0 )
				{
					/*
					{
						MSHTML::IHTMLWindow2Ptr		wnd = iFrameDoc ? iFrameDoc->parentWindow : NULL;
						BSTR url = NULL;
						if (wnd == NULL || ! GetLibraryURL((CComQIPtr<IDispatchEx>)wnd, &url) )  // Cordys Specific test
						{
							url = SysAllocString(iFrameDoc->url);
						}

						BSTR sValue = NULL;
						if ( GetPropertyValueByName((CComQIPtr<IDispatchEx>)node, L"id", &sValue) )
						{
						}

						CStringW x;
						x.Format(L"New IFRAME %s %s (#%d)",(LPCTSTR) url, (LPCTSTR) sValue, elem->seqNr);
						SysFreeString(sValue);
						m_mainBrowserDlg->logMessage(x);
						SysFreeString(url);
					}
					*/
					// Not yet hooked it is a new Document in an IFrame;
					hookNewPage(iFrameDoc);
				}
			}
		}
	}
}

size_t JSHook::getNodeCount() const {
	return m_nodes.size();
}

// Collect all leaked nodes, passing them to the specified leak dialog.
//
void JSHook::showLeaks(MSHTML::IHTMLWindow2Ptr wnd, CLeakDlg* dlg, bool showLeaks) {
	// Free non-leaked nodes
	//
	m_leakDlg = dlg;
	releaseExtraReferences(wnd);

	for (std::map<IUnknown*,Elem*>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it) {
		IUnknown *unkNode = it->first;
		Elem* elem = it->second;

		// For each node, AddRef() and Release() it.  The latter method will return
		//   the current ref count.
		//
		unkNode->AddRef();
		int refCount = unkNode->Release();
		elem->refCountSample = refCount -1; // Sample taken just before reporting in the leakDlg (excluding our own reference)

		// If any references (other than the one that we hold) are outstanding, then
		//   the node has been leaked.
		//

		if ( showLeaks )
		{
			// Only show all leaks (And thus also the detected cycles)
			if ( (!elem->running && refCount > 1) || elem->cycleDetected )
			{
				dlg->addElement(elem);
			}
		}
		else
		{
			// Show nodes in use together with leaks
			if ( (refCount - 1) > elem->reported ) elem->hide = false;
			if ( refCount > 1 && ! elem->hide )
				dlg->addElement(elem);
		}
	}
}

void JSHook::countNodes(MSHTML::IHTMLWindow2Ptr wnd, int& leakedItems, int& hiddenItems) {
	// Free non-leaked nodes
	//
	leakedItems = 0;
	hiddenItems = 0;
	releaseExtraReferences(wnd);

	for (std::map<IUnknown*,Elem*>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it) {
		IUnknown *unkNode = it->first;
		Elem* elem = it->second;

		unkNode->AddRef();
		int refCount = unkNode->Release();

		// If any references (other than the one that we hold) are outstanding, then
		//   the node has been leaked.
		// OR if we detected a cycle

		if ( (!elem->running && refCount > 1) || elem->cycleDetected )
		{
			leakedItems++;
		}
		if ( elem->hide || refCount <= 1 )
		{
			hiddenItems++;
		}
	}
}

// Hide all nodes for the UI (Show in Use Button clicked).
//
void JSHook::clearNodes() {
	std::map<IUnknown*,Elem*>::iterator it = m_nodes.begin();
	while (it != m_nodes.end())
	{
		std::map<IUnknown*,Elem*>::iterator next = it; next++; // Save the next one.
		IUnknown *unkNode = it->first;
		unkNode->AddRef();
		int refCount = unkNode->Release();

		Elem* elem = it->second;
		elem->hide = true;  // Hide the node for the user interface
		elem->reported = refCount - 1; // reported counter at the moment node is hidden. If recFount increases Node will be showed again
		it = next;
	}
}

// Free up any non-leaked nodes
//
void JSHook::releaseExtraReferences(MSHTML::IHTMLWindow2Ptr wnd) {
	// Make as many passes as necessary to clean up nodes. Some nodes such
	// as hidden table cells may not be cleaned up on the first pass.
	//
	size_t count = 0;
	while (count != m_nodes.size()) {
		count = m_nodes.size();

		// Ensure that all garbage collection is completed so that nodes will
		//   be released.
		//
		if ( wnd ) wnd->execScript(L"window.CollectGarbage()", L"javascript");

		// Release extra references for all nodes
		//
		m_itNextNode = m_nodes.begin();
		while (m_itNextNode != m_nodes.end())
			backgroundReleaseExtraReferences();
	}
}

void JSHook::backgroundReleaseExtraReferences() {
	if (m_itNextNode == m_nodes.end()) {
		if (m_itNextNode == m_nodes.begin()) {
			// There are no nodes to collect.
			//
			return;
		}
		else {
			// Restart the garbage collection
			//
			m_itNextNode = m_nodes.begin();
		}
	}

	IUnknown *unkNode = m_itNextNode->first;
	Elem* elem = m_itNextNode->second;

	unkNode->AddRef();
	int refCount = unkNode->Release();
	elem->refCountSample = refCount - 1;
	if (refCount == 1) {
		// If this is the only outstanding reference, free it.
		if ( m_leakDlg ) m_leakDlg->notifyElement(elem); // To update the dialog
		delete elem;
		VERIFY(unkNode->Release() == 0);
		m_itNextNode = m_nodes.erase(m_itNextNode);
	}
	else {
		if ( m_leakDlg && (!elem->running || elem->cycleDetected) && (!elem->leakReported || elem->reported != elem->refCountSample) )
		{
			// There is a new detected leak or cycle which is not yet reported.
			// It is necessary to inform the user about the leak ASAP.
			m_leakDlg->notifyElement(elem);
		}
		m_itNextNode++;
	}
}