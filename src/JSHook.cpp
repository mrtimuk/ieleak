#include "stdafx.h"
#include "JSHook.hpp"
#include "DOMReportDlg.hpp"
#include "HtmlResource.h"

JSHook::~JSHook() {
	// When the hook is destroyed, make sure all nodes are released.
	//
	clearNodes();
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
	// The only member is 'logNode'.
	//
	bool failed = false;
	for (int i = 0; i < (int)nameCount; ++i) {
		if (!wcscmp(names[i], L"logNode"))
			dispIds[i] = 0;
		else {
			dispIds[i] = -1;
			failed = true;
		}
	}

	if (failed)
		return DISP_E_MEMBERNOTFOUND;
	return S_OK;
}

STDMETHODIMP JSHook::Invoke(DISPID dispId, REFIID riid, LCID lcid, WORD flags, DISPPARAMS *dispParams, VARIANT *result, EXCEPINFO *exInfo, UINT *argErr) {
	if (dispId == -1)
		return DISP_E_MEMBERNOTFOUND;

	// logNode() takes three arguments, node, doc, and recurse.
	//
	if (dispParams->cArgs != 3)
		return DISP_E_BADPARAMCOUNT;

	if (dispParams->rgvarg[0].vt != VT_BOOL ||
		dispParams->rgvarg[1].vt != VT_DISPATCH ||
		dispParams->rgvarg[2].vt != VT_DISPATCH)
		return DISP_E_BADVARTYPE;

	// Get the document and node, and add the node to the list.
	//
	BOOL recurse = dispParams->rgvarg[0].boolVal;
	MSHTML::IHTMLDocument2Ptr doc = dispParams->rgvarg[1].pdispVal;
	MSHTML::IHTMLDOMNodePtr node = dispParams->rgvarg[2].pdispVal;

	if (node) {
		if (recurse)
			addNodeRecursively(node, doc);
		else
			addNode(node, doc);
	}
	return S_OK;
}

// Add an node to the hook's list.  The node's URL will be retrieved from
//  the specified document.
//
void JSHook::addNode(MSHTML::IHTMLDOMNode* node, MSHTML::IHTMLDocument2* doc) {
	// In order to ensure that we maintain a durable reference to the node (as
	//   opposed to a tear-off interface), we have to query for IUnknown.
	//
	IUnknown* unk = NULL;
	node->QueryInterface(IID_IUnknown, (void**)&unk);

	// Do not register the node twice; doing so will cause it to be incorrectly reported as a leak.
	// This happens when an node is created by createElement and is added to the DOM before the DOM
	// is traversed. (Note that the createElement hook must be in place immediately in case the caller
	// saves a references to the element and adds it to the DOM after the page is loaded.) 
	//
	if (m_nodes.find(unk) == m_nodes.end()) {
		Node cachedNode(SysAllocString(doc->url));
		m_nodes.insert(std::pair<IUnknown*,Node>(unk,cachedNode));

		// Create a temporary parameter list to pass the node to the script, 
		// in order to so attach events and override functions
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
		OLECHAR *name = SysAllocString(L"__drip_hookEvents");
		scriptObj->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_SYSTEM_DEFAULT, &dispId);
		SysFreeString(name);

		scriptObj->Invoke(dispId, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, NULL, NULL, NULL);

		VariantClear(&vHook);
	}
	else {
		unk->Release();
	}
}

// This method is called when a document is finished loading.  It will hook the
//   document's createElement() method, passing all dynamically-created elements
//   to the hook as they are created.
//
void JSHook::hookNewPage(MSHTML::IHTMLDocument2Ptr doc) {
	MSHTML::IHTMLWindow2Ptr wnd = doc->parentWindow;

	if (!m_js.GetLength())
		VERIFY(GetHTML(IDR_DRIP_JS, m_js));

	// Free up extra references to avoid a perpetual increase in memory usage
	//
	releaseExtraReferences(wnd);

	// Create a temporary function to hook createElement() and cloneNode
	// Also, set up functions to hook onPropertyChange.
	//
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
	CComBSTR name("__drip_initHook");
	scriptObj->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_SYSTEM_DEFAULT, &dispId);

	scriptObj->Invoke(dispId, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, NULL, NULL, NULL);

	VariantClear(&vHook);
}

// Add all elements recursively from a given root element.
//
void JSHook::addNodeRecursively(MSHTML::IHTMLDOMNode* node, MSHTML::IHTMLDocument2* doc) {
	addNode(node, doc);

	MSHTML::IHTMLDOMNodePtr child = node->firstChild;
	while (child) {
		addNodeRecursively(child, doc);
		child = child->nextSibling;
	}
}

// Add all elements within a window, starting with its document's body.
//
void JSHook::addStaticNodes(MSHTML::IHTMLWindow2Ptr wnd) {
   MSHTML::IHTMLDOMNodePtr docNode = wnd->document;
	addNodeRecursively(docNode, wnd->document);
}

// Collect all leaked elements, passing them to the specified leak dialog.
//
void JSHook::showDOMReport(MSHTML::IHTMLWindow2Ptr wnd, CDOMReportDlg* dlg, DOMReportType type) {
	ASSERT(type == kLeaks);

	// Free non-leaked nodes
	//
	releaseExtraReferences(wnd);

	// The remaining nodes have been leaked
	//
	for (std::map<IUnknown*,Node>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it) {
		IUnknown *unk = it->first;
		Node& node = it->second;

		// For each node, AddRef() and Release() it.  The latter method will return
		//   the current ref count.
		//
		unk->AddRef();
		int refCount = unk->Release();

		// If any references (other than the one that we hold) are outstanding, then
		//   the node has been leaked. (All non-leaked elements should already have been released.)
		//
		if (refCount > 1) {
			dlg->addNode(unk, node.url, refCount - 1, node.lastLeakRefCount != refCount);
			node.lastLeakRefCount = refCount;
		}
		else
			ASSERT(false);
	}
}

// Returns true if the hook contains any nodes.
//
bool JSHook::hasNodes() {
	return (m_nodes.size() > 0);
}

// Clear all nodes in the hook.
//
void JSHook::clearNodes() {
	for (std::map<IUnknown*,Node>::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it) {
		IUnknown *unk = it->first;
		Node const& node = it->second;

		// Release the URL string and the node.
		//
		SysFreeString(node.url);
		unk->Release();
	}

	m_nodes.clear();
}

// Free up any non-leaked elements
//
void JSHook::releaseExtraReferences(MSHTML::IHTMLWindow2Ptr wnd) {
	// Make as many passes as necessary to clean up elements. Some elements such
	// as hidden table cells may not be cleaned up on the first pass.
	//
	size_t count = 0;
	while (count != m_nodes.size()) {
		count = m_nodes.size();

		// Ensure that all garbage collection is completed so that elements will
		//   be released.
		//
		wnd->execScript(L"window.CollectGarbage()", L"javascript");

		for (std::map<IUnknown*,Node>::iterator it = m_nodes.begin(); it != m_nodes.end(); ) {
			IUnknown *unk = it->first;
			Node const& node = it->second;

			unk->AddRef();
			int i = unk->Release();
			if (i == 1) {
				// If this is the only outstanding reference, free it.
				SysFreeString(node.url);
				VERIFY(unk->Release() == 0);
				it = m_nodes.erase(it);
			}
			else
				it++;
		}
	}
}
