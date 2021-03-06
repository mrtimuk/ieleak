#pragma once

class CDOMReportDlg;

// This structure is used to maintain a list of hooked nodes,
//   along with the URL of the document to which each belongs.
//
struct Node {
	Node(BSTR url) {
		this->url = url;
		this->lastUsageRefCount = 0;
		this->lastLeakRefCount = 0;
	}

	BSTR		url;
	int		lastUsageRefCount;
	int		lastLeakRefCount;
};

class __declspec(uuid("8340a7f2-413a-46dd-9f95-fbad5d455a90")) IJSHook: public IDispatch { };

// The JSHook class serves two purposes.  First, it implements a simple IDispatch interface
//   that is callable from JavaScript.  Its sole method, logNode(node), is called whenever
//   a node is created dynamically.  Each such node is stored in a set, along with all
//   of the static node.  When the user wants to detect leaks, this class goes through the
//   list, determining which nodes have a reference count greater than 1 (JSHook keeps one
//   reference on all nodes in order to keep them alive).
//
class JSHook: public IJSHook, public ATL::CComObjectRoot {
private:
	void addNode(MSHTML::IHTMLDOMNode* node, MSHTML::IHTMLDocument2* doc);
	void addNodeRecursively(MSHTML::IHTMLDOMNode* node, MSHTML::IHTMLDocument2* doc);

	void releaseExtraReferences(MSHTML::IHTMLWindow2Ptr wnd);

	CStringW m_js;
	std::map<IUnknown*,Node> m_nodes;
	std::map<IUnknown*,Node>::iterator m_itNextNode;

public:
	JSHook();
	virtual ~JSHook();

	enum DOMReportType
	{
		kUsage,
		kLeaks,
	};

	// Page loading functions
	void hookNewPage(MSHTML::IHTMLDocument2Ptr wnd);
	void addStaticNodes(MSHTML::IHTMLWindow2Ptr wnd);

	// Reporting functions
	size_t getNodeCount() const;
	void showDOMReport(MSHTML::IHTMLWindow2Ptr wnd, CDOMReportDlg* dlg, DOMReportType type);
	void backgroundReleaseExtraReferences();

	// Cleanup functions
	void clearNodes();

	BEGIN_COM_MAP(JSHook)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(IJSHook)
	END_COM_MAP()

	STDMETHOD(GetTypeInfoCount)(UINT *pctinfo);
	STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
	STDMETHOD(GetIDsOfNames)(REFIID iid, OLECHAR **names, UINT nameCount, LCID lcid, DISPID *dispIds);
	STDMETHOD(Invoke)(DISPID dispId, REFIID riid, LCID lcid, WORD flags, DISPPARAMS *dispParams, VARIANT *result,
		EXCEPINFO *exInfo, UINT *argErr);
};
