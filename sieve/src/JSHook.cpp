#include "stdafx.h"
#include "JSHook.hpp"
#include "LeakDlg.hpp"
#include "PropDlg.hpp"
#include "HtmlResource.h"
#include "MainBrowserDlg.hpp"

JSHook::JSHook() {
	m_itNextNode = m_elements.begin();
}

JSHook::~JSHook() {
	// When the hook is destroyed, make sure all elements are released.
	//
	clearElements();
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
	// members are 'logElement' and 'unloadWindow' and 'rescanForElements'
	//
	bool failed = false;
	for (int i = 0; i < (int)nameCount; ++i) {
		if (!wcscmp(names[i], L"logElement"))
			dispIds[i] = 0;
		else if (!wcscmp(names[i], L"unloadWindow"))
			dispIds[i] = 1;
		else if (!wcscmp(names[i], L"rescanForElements"))
			dispIds[i] = 2;
		else if (!wcscmp(names[i], L"logDetectedCycle"))
			dispIds[i] = 3;
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
		// logElement() takes one argument, elem
		//
		if (dispParams->cArgs != 1)
			return DISP_E_BADPARAMCOUNT;

		if (dispParams->rgvarg[0].vt != VT_DISPATCH)
			return DISP_E_BADVARTYPE;

		// Get the element, and add the element to the list.
		//
		MSHTML::IHTMLDOMNodePtr elem = dispParams->rgvarg[0].pdispVal;
		addElement(elem);
		return S_OK;
	}
	else if ( dispId == 1 )
	{
		// unloadWindow
		MSHTML::IHTMLDocument2Ptr doc = dispParams->rgvarg[0].pdispVal;
		Elem* docElem = getDocument(doc);
		if ( docElem )
		{
			unloadWindow(doc);
		}
		else
		{
			IUnknown *unkDoc;
			doc->QueryInterface(IID_IUnknown, (void**)&unkDoc);
			unkDoc->Release();

			TCHAR msg[128];
			wsprintf(msg,L"Document not found %s unkptr = 0x%x trid=0x%x",doc->url,unkDoc, GetCurrentThreadId());
			AfxMessageBox(msg);
		}
		return S_OK;
	}
	else if ( dispId == 2 )
	{
		// rescanForElements
		if (dispParams->cArgs == 1)
		{
			MSHTML::IHTMLDocument2Ptr doc = dispParams->rgvarg[0].pdispVal;
			rescanForElements(doc);
		}
		else
		{
			rescanForElements(NULL);
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

		// Get the element, and add the element to the list.
		//
		MSHTML::IHTMLDOMNodePtr elem = dispParams->rgvarg[0].pdispVal;
		Elem* cachedElem = getElement(elem);
		if ( cachedElem )
		{
			cachedElem->cycleDetected = true;
		}
		return S_OK;
	}
	else 
		return DISP_E_MEMBERNOTFOUND;
}

static int docId = 0;

Elem* JSHook::addDocument(MSHTML::IHTMLDocument2* doc, MSHTML::IHTMLWindow2* wnd)
{
	Elem* docElem = getDocument(doc);
	if ( doc && ! docElem )
	{	
		BSTR nodeName = NULL;
		BSTR url = NULL;//SysAllocString(doc->url);
		IUnknown *unkDoc;

		doc->QueryInterface(IID_IUnknown, (void**)&unkDoc);
		GetPropertyValueByName((CComQIPtr<IDispatchEx>)doc, L"nodeName", &nodeName);	
		if (!wnd || ! GetLibraryURL((CComQIPtr<IDispatchEx>)wnd, &url) )  // Cordys Specific test
		{
			url = SysAllocString(doc->url);
		}
		Elem cachedElem(url, nodeName);
		cachedElem.docId = docId++;
		m_runningDocs.insert(std::pair<IUnknown*,Elem>(unkDoc,cachedElem));
		// Don't unkDoc->Release() because reference is inserted in map;
		
		docElem = getDocument(doc);
		docElem->docElem = docElem; // Point to itself;
	}
	return docElem;
}

Elem* JSHook::getDocument(MSHTML::IHTMLDocument2* doc)
{
	if ( doc )
	{
		IUnknown *unkDoc;
		doc->QueryInterface(IID_IUnknown, (void**)&unkDoc);		
		std::map<IUnknown*,Elem>::iterator pair = m_runningDocs.find(unkDoc);
		unkDoc->Release();

		if ( pair == m_runningDocs.end() ) return NULL;
		Elem& docElem =  pair->second;
		return &docElem;
	}
	return NULL;
}

Elem* JSHook::addWindow(MSHTML::IHTMLDocument2* doc, MSHTML::IHTMLWindow2* wnd)
{
	Elem* docElem = getDocument(doc);
	Elem* wndElem = getWindow(wnd);
	if ( wnd && ! wndElem )
	{
		IUnknown *unkWnd;

		wnd->QueryInterface(IID_IUnknown, (void**)&unkWnd);
		BSTR nodeName = SysAllocString(L"#window");

		Elem cachedElem(NULL, nodeName);
		if ( docElem ) 
		{
			cachedElem.docElem = docElem;
		}
		m_elements.insert(std::pair<IUnknown*,Elem>(unkWnd,cachedElem));
		// Don't unkWnd->Release() because reference is inserted in map;
		wndElem = getWindow(wnd);
	}
	if ( wndElem )
	{
		if ( wndElem->docElem != docElem )
		{
			// 'window' objects are recycled; so the document 'ie docElem' can change !
			// Just reset the properties and deal with this window object as being a new object
			TCHAR msg[256];
			wsprintf(msg,L"window doc changed: %s -> %s", wndElem->docElem->url, docElem->url);
//			AfxMessageBox(msg);
			wndElem->docElem = docElem;
			wndElem->reported = 0;
			wndElem->hide = false;
		}
	}
	return wndElem;
}


Elem* JSHook::getWindow(MSHTML::IHTMLWindow2* wnd)
{
	if ( wnd )
	{
		IUnknown *unkWnd;
		wnd->QueryInterface(IID_IUnknown, (void**)&unkWnd);		
		std::map<IUnknown*,Elem>::iterator pair = m_elements.find(unkWnd);
		unkWnd->Release();

		if ( pair == m_elements.end() ) return NULL;
		Elem& wndElem =  pair->second;
		return &wndElem;
	}
	return NULL;
}

// Add an element to the hook's list.  The element's URL will be retrieved from
//  its current document.
//
void JSHook::addElement(MSHTML::IHTMLDOMNode* elem) {
	if ( elem->nodeType == NODE_TEXT )
		return; // No need to register NODE_TEXT; They never leak;
	MSHTML::IHTMLDOMNode2Ptr	node= elem;
	MSHTML::IHTMLDocument2Ptr	doc = node->ownerDocument;
	MSHTML::IHTMLWindow2Ptr		wnd = doc ? doc->parentWindow : NULL;

	// In order to ensure that we maintain a durable reference to the element (as
	//   opposed to a tear-off interface), we have to query for IUnknown.
	//
	IUnknown* unkElem = NULL;
	Elem* docElem = NULL;

	// Do not register the element twice; doing so will cause it to be incorrectly reported as a leak.
	// This happens when an element is created by createElement and is added to the DOM before the DOM
	// is traversed. (Note that the createElement hook must be in place immediately in case the caller
	// saves a references to the element and adds it to the DOM after the page is loaded.) 
	//
	if ( elem ) elem->QueryInterface(IID_IUnknown, (void**)&unkElem);
	if ( doc )
	{
		docElem = addDocument(doc,wnd);
	}

	if ( wnd )
	{
		addWindow(doc,wnd);
	}

	if ( unkElem )
	{
		BSTR nodeName = NULL;
		if (m_elements.find(unkElem) == m_elements.end())
		{
			GetPropertyValueByName((CComQIPtr<IDispatchEx>)elem, L"nodeName", &nodeName);	
			if ( doc )
			{
				Elem cachedElem(NULL, nodeName);
				if ( docElem ) 
				{
					cachedElem.docElem = docElem;
				}
				m_elements.insert(std::pair<IUnknown*,Elem>(unkElem,cachedElem));
				hookNewElement(elem,doc);
			}
			else
			{
				Elem cachedElem(SysAllocString(L"N/A"), nodeName);
				m_elements.insert(std::pair<IUnknown*,Elem>(unkElem,cachedElem));
			}
		}
		else
		{
			unkElem->Release();
		}
	}
}

Elem* JSHook::getElement(MSHTML::IHTMLDOMNodePtr elem)
{
	if ( elem )
	{
		IUnknown *unkElem;
		elem->QueryInterface(IID_IUnknown, (void**)&unkElem);		
		std::map<IUnknown*,Elem>::iterator pair = m_elements.find(unkElem);
		unkElem->Release();

		if ( pair == m_elements.end() ) return NULL;
		Elem& cachedElem =  pair->second;
		return &cachedElem;
	}
	return NULL;
}

void JSHook::hookNewElement(MSHTML::IHTMLDOMNodePtr elem, MSHTML::IHTMLDocument2Ptr doc ) {

	if ( elem->nodeType != NODE_TEXT )
	{
		// Create a parameter list containing the hook, then invoke the
		//   temporary function to attach it to the document.
		//
		VARIANT vHook;
		VariantInit(&vHook);
		vHook.vt = VT_DISPATCH;
		vHook.pdispVal = elem;
		elem->AddRef();

		DISPPARAMS params;
		memset(&params, 0, sizeof(DISPPARAMS));
		params.cArgs = 1;
		params.rgvarg = &vHook;

		CComPtr<IDispatch> scriptObj = doc->Script;

		DISPID dispId;
		OLECHAR *name = SysAllocString(L"__sIEve_overloadCloneNode");
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
	if ( button ) button->GetWindowTextW(oldButtonTxt,50);
	Elem* docElem = getDocument(doc); // Search for registered document
	for (std::map<IUnknown*,Elem>::iterator it = m_elements.begin(); it != m_elements.end(); ++it) {
		IUnknown *unk = it->first;
		Elem& elem = it->second;


		MSHTML::IHTMLDOMNode2Ptr element = unk;
		MSHTML::IHTMLDOMNodePtr node = unk;

		if ( element )
		{
			// If 'doc' is NULL then Scan all elements of all documents
			// Otherwise only scan the elements of a specified doc;
			if ( (doc == NULL || elem.docElem == docElem) && elem.docElem->running && node->nodeType != NODE_TEXT)
			{
				if ( button )
				{
					wsprintf(buttonTxt,L"Scan: %d",++i);
					button->SetWindowTextW(buttonTxt);
				}
				crossRefScanElement(node);
			}
		}
	}
	if ( button ) button->SetWindowTextW(oldButtonTxt);
}

void JSHook::crossRefScanElement(MSHTML::IHTMLDOMNode2Ptr elem)
{
	// Create a parameter list containing the hook, then invoke the
	//   temporary function to attach it to the document.
	//
	VARIANT vHook;
	VariantInit(&vHook);
	vHook.vt = VT_DISPATCH;
	vHook.pdispVal = elem;
	elem->AddRef();

	DISPPARAMS params;
	memset(&params, 0, sizeof(DISPPARAMS));
	params.cArgs = 1;
	params.rgvarg = &vHook;


	MSHTML::IHTMLDocument2Ptr doc = elem->ownerDocument;
	CComPtr<IDispatch> scriptObj = doc->Script;

	DISPID dispId;
	OLECHAR *name = SysAllocString(L"___sIEve_crossRefScan");
	scriptObj->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_SYSTEM_DEFAULT, &dispId);
	SysFreeString(name);
	scriptObj->Invoke(dispId, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, NULL, NULL, NULL);

	VariantClear(&vHook);
}

// This method is called when a document is finished loading.  It will hook the
//   document's createElement() method, passing all dynamically-created elements
//   to the hook as they are created.
//
void JSHook::hookNewPage(MSHTML::IHTMLDocument2Ptr doc) {
	if ( wcscmp(doc->url,L"about:blank") )
	{
		MSHTML::IHTMLWindow2Ptr wnd = doc->parentWindow;

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
	}
}

// Add all elements recursively from a given root element.
//
void JSHook::addElementRecursively(MSHTML::IHTMLDOMNode* elem) {
	addElement(elem);

	MSHTML::IHTMLDOMNodePtr node = elem;
	MSHTML::IHTMLDOMNodePtr child = node->firstChild;
	while (child) {
		addElementRecursively(child);
		child = child->nextSibling;
	}
}

// Add all elements within a window, starting with its document's body.
//
void JSHook::addStaticElements(MSHTML::IHTMLDocument2Ptr doc) {

	MSHTML::IHTMLWindow2Ptr wnd = doc->parentWindow;
	if ( wcscmp(doc->url,L"about:blank") )
	{
		MSHTML::IHTMLDocument3Ptr document = doc;
		MSHTML::IHTMLDOMNodePtr documentElementNode = document->documentElement;
		addElementRecursively(documentElementNode);
	}
}

void JSHook::unloadWindow(MSHTML::IHTMLDocument2Ptr doc)
{
	Elem* docElem = getDocument(doc);
	if ( docElem )
	{
		rescanForElements(doc);
		if ( m_mainBrowserDlg->m_check_cycle_detection )
		{
			crossRefScan(doc,(CButton*)(m_mainBrowserDlg->GetDlgItem(IDC_CROSSREF_SCAN)));
		}
		docElem->running = false; // Invalidate the running state
	}
}

void JSHook::rescanForElements(MSHTML::IHTMLDocument2Ptr doc)
{
	Elem* docElem = getDocument(doc); // Search for registered document
	std::map<IUnknown*,Elem>::iterator end = m_elements.end();
	for (std::map<IUnknown*,Elem>::iterator it = m_elements.begin(); it != end; ++it) {
		IUnknown *unk = it->first;
		MSHTML::IHTMLElementPtr	element = unk;

		if ( element )
		{
			Elem& elem = it->second;

			// If 'doc' is NULL then Scan all elements of all documents
			// Otherwise only scan the elements of a specified doc;
			if ( (doc == NULL || elem.docElem == docElem) && elem.docElem->running )
			{
				unk->AddRef();
				int refCount = unk->Release();

				// Only rescan the elements which are still in use (ie having references besides our own reference)
				if ( refCount > 1 )
				{	
					// Only scan the elements without having a parent Element
					// Otherwise all we do way too much redundant scanning
					if ( element->parentElement == NULL )
					{
						MSHTML::IHTMLDOMNodePtr node = unk;
						addElementRecursively(node);
					}
				}
				else
				{
						//AfxMessageBox(L"Found Refcount <= 1 ");
				}
			}

			MSHTML::IHTMLDocument2Ptr iFrameDoc = GetContentDocument((CComQIPtr<IDispatchEx>)element);
			if ( iFrameDoc )
			{
				MSHTML::IHTMLWindow2Ptr		iFrameWnd = iFrameDoc ? iFrameDoc->parentWindow : NULL;
				if ( getDocument(iFrameDoc) == NULL )
				{
//					AfxMessageBox(L"new IframeDoc found");
//					AfxMessageBox(iFrameDoc->url);
					addDocument(iFrameDoc,iFrameWnd);
					addStaticElements(iFrameDoc);
					hookNewPage(iFrameDoc);
				}
			}
		}
		else
		{
			// Document or window type
		}
	}
}

size_t JSHook::getNodeCount() const {
	return m_elements.size();
}

// Collect all leaked elements, passing them to the specified leak dialog.
//
void JSHook::showLeaks(MSHTML::IHTMLWindow2Ptr wnd, CLeakDlg* dlg, bool showLeaks) {
	// Free non-leaked nodes
	//
	releaseExtraReferences(wnd);

	for (std::map<IUnknown*,Elem>::iterator it = m_elements.begin(); it != m_elements.end(); ++it) {
		IUnknown *unk = it->first;
		Elem &elem = it->second;

		// For each element, AddRef() and Release() it.  The latter method will return
		//   the current ref count.
		//
		unk->AddRef();
		int refCount = unk->Release();

		// If any references (other than the one that we hold) are outstanding, then
		//   the element has been leaked.
		//

		if ( showLeaks )
		{
			// Only show all leaks (And thus also the detected cycles)
			if ( (!elem.docElem->running && refCount > 1) || elem.cycleDetected )
			{
				dlg->addElement(unk, &elem, refCount - 1, false, 0, elem.leakReported );
				elem.leakReported = refCount - 1;
			}
		}
		else
		{
			// Show elements in use together with leaks
			if ( (refCount - 1) > elem.reported ) elem.hide = false;
			if ( refCount > 1 && ! elem.hide )
				dlg->addElement(unk, &elem, refCount - 1, false, 0, elem.reported );
			elem.reported = refCount - 1;
		}
	}

	for (std::map<IUnknown*,Elem>::iterator it = m_runningDocs.begin(); it != m_runningDocs.end(); ++it) {
		IUnknown *unk = it->first;
		Elem &elem = it->second;

		// For each element, AddRef() and Release() it.  The latter method will return
		//   the current ref count.
		//
		unk->AddRef();
		int refCount = unk->Release();

		// If any references (other than the one that we hold) are outstanding, then
		//   the element has been leaked.
		//

		if ( showLeaks )
		{
			// Only show all leaks
			if ( !elem.docElem->running && refCount > 1)
			{
				dlg->addElement(unk, &elem, refCount - 1, false, 0, elem.leakReported );
				elem.leakReported = refCount - 1;
			}
		}
		else
		{
			if ( (refCount - 1) > elem.reported ) elem.hide = false;
			if ( refCount > 1 && ! elem.hide )
				dlg->addElement(unk, &elem, refCount - 1, false, 0, elem.reported );
			elem.reported = refCount - 1;
		}
	}
}

void JSHook::countElements(MSHTML::IHTMLWindow2Ptr wnd, int& leakedItems, int& hiddenItems) {
	// Free non-leaked nodes
	//
	leakedItems = 0;
	hiddenItems = 0;
	releaseExtraReferences(wnd);

	for (std::map<IUnknown*,Elem>::iterator it = m_elements.begin(); it != m_elements.end(); ++it) {
		IUnknown *unk = it->first;
		Elem &elem = it->second;

		unk->AddRef();
		int refCount = unk->Release();

		// If any references (other than the one that we hold) are outstanding, then
		//   the element has been leaked.
		// OR if we detected a cycle

		if ( (!elem.docElem->running && refCount > 1) || elem.cycleDetected )
		{
			leakedItems++;
		}
		if ( elem.hide || refCount <= 1 )
		{
			hiddenItems++;
		}
	}

	for (std::map<IUnknown*,Elem>::iterator it = m_runningDocs.begin(); it != m_runningDocs.end(); ++it) {
		IUnknown *unk = it->first;
		Elem &elem = it->second;

		unk->AddRef();
		int refCount = unk->Release();

		// If any references (other than the one that we hold) are outstanding, then
		//   the element has been leaked.
		if ( !elem.docElem->running && refCount > 1)
		{
			leakedItems++;
		}
		if ( elem.hide || refCount <= 1 )
		{
			hiddenItems++;
		}
	}
}

// Clear all unused elements and documents in the hook.
//
void JSHook::clearElements() {
	// Clear unused elements

	std::map<IUnknown*,Elem>::iterator it = m_elements.begin();
	while (it != m_elements.end())
	{
		std::map<IUnknown*,Elem>::iterator next = it; next++; // Save the next one.
		IUnknown *unk = it->first;
		unk->AddRef();
		int refCount = unk->Release();

		Elem& elem = it->second;
		elem.hide = true;  // Hide the element for the user interface
		elem.reported = refCount - 1; // reported counter at the moment element is hidden. If recFount increases Element will be showed again
		if ( ! elem.docElem->running )
		{
			// Only erase the elements which are not longer in use (ie having references besides our own reference)
			if ( refCount == 1 )
			{
				if ( elem.url ) SysFreeString(elem.url);
				if ( elem.nodeName ) SysFreeString(elem.nodeName);
				m_elements.erase(it);  
				unk->Release();  // Relase our own reference
			}
			else
			{
				elem.docElem->documentHasLeaks = true;  // Protect docElem from normal cleanUp;
			}
		}
		it = next;
	}

	// Clear unused documents
	it = m_runningDocs.begin(); 
	while (it != m_runningDocs.end())
	{
		std::map<IUnknown*,Elem>::iterator next = it; next++; // Save the next one.
		IUnknown *unk = it->first;
		unk->AddRef();
		int refCount = unk->Release();

		Elem& elem = it->second;
		elem.hide = true;  // Hide the element for the user interface
		elem.reported = refCount - 1; // reported counter at the moment element is hidden. If recFount increases Element will be showed again
		if ( ! elem.docElem->running && ! elem.docElem->documentHasLeaks )
		{
			// Only erase the elements which are not longer in use (ie having references besides our own reference)
			if ( refCount == 1 )
			{
				if ( elem.url ) SysFreeString(elem.url);
				if ( elem.nodeName ) SysFreeString(elem.nodeName);
				m_runningDocs.erase(it->first);
				unk->Release();
			}
		}
		it = next;
	}

	// It is very important to update m_itNextNode whenever items are removed from the nodes list
	//
	m_itNextNode = m_elements.begin();
}

// Free up any non-leaked elements
//
void JSHook::releaseExtraReferences(MSHTML::IHTMLWindow2Ptr wnd) {
	// Make as many passes as necessary to clean up elements. Some elements such
	// as hidden table cells may not be cleaned up on the first pass.
	//
	size_t count = 0;
	while (count != m_elements.size()) {
		count = m_elements.size();

		// Ensure that all garbage collection is completed so that elements will
		//   be released.
		//
		if ( wnd ) wnd->execScript(L"window.CollectGarbage()", L"javascript");

		// Release extra references for all elements
		//
		m_itNextNode = m_elements.begin();
		while (m_itNextNode != m_elements.end())
			backgroundReleaseExtraReferences();
	}
}

void JSHook::backgroundReleaseExtraReferences() {
	if (m_itNextNode == m_elements.end()) {
		if (m_itNextNode == m_elements.begin()) {
			// There are no nodes to collect.
			//
			return;
		}
		else {
			// Restart the garbage collection
			//
			m_itNextNode = m_elements.begin();
		}
	}

	IUnknown *unk = m_itNextNode->first;
	Elem const& node = m_itNextNode->second;

	unk->AddRef();
	int i = unk->Release();
	if (i == 1) {
		// If this is the only outstanding reference, free it.
		if ( node.url ) SysFreeString(node.url);
		if ( node.nodeName ) SysFreeString(node.nodeName);
		VERIFY(unk->Release() == 0);
		m_itNextNode = m_elements.erase(m_itNextNode);
	}
	else {
		m_itNextNode++;
	}
}