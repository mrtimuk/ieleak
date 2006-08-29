#pragma once

class CLeakDlg;
class CMainBrowserDlg;

// This structure is used to maintain a list of hooked nodes,
//   along with the URL of the document to which each belongs.
//

static int ElemSeqNr = 0;
struct Elem {
	Elem(IUnknown* unkNode, BSTR url, BSTR nodeName) {
		this->unkNode = unkNode;
		this->seqNr = ElemSeqNr++;
		this->url = url;
		this->nodeName = nodeName;
		this->id = NULL;
		this->reported = 0; // To support red coloring of new reported elements
		this->refCountSample = 0; // To support red coloring of new reported elements
		this->leakReported = 0; // To support red coloring of new reported elements
		this->running = true; // To indicate a real leaked element
		this->hide = false;  // To support clear in use functionality
		this->docId = 0;
		this->cycleDetected = false;
		this->isOrphan = false;
	}
	Elem(Elem *org)
	{
		this->unkNode = org->unkNode;
		this->seqNr = org->seqNr;
		this->url = SysAllocString(org->url);
		this->nodeName = SysAllocString(org->nodeName);
		this->id = NULL;
		this->reported = org->reported;
		this->refCountSample = org->refCountSample;
		this->leakReported = org->leakReported;
		this->running = org->running;
		this->hide = org->hide;
		this->docId = org->docId;
		this->cycleDetected = org->cycleDetected;		
		this->isOrphan = org->isOrphan;
	}
	~Elem()
	{
		if ( this->url) SysFreeString(this->url);
		if ( this->nodeName) SysFreeString(this->nodeName);
		if ( this->id) SysFreeString(this->id);
	}
	IUnknown*	unkNode;
	int			seqNr;
	BSTR		url;
	BSTR		nodeName;
	BSTR		id;
	int			refCountSample;
	int			reported;
	int			leakReported;
	int			reportedItemIndex;
	BOOL		running;
	BOOL		hide;
	int			docId;
	BOOL		cycleDetected;
	BOOL		isOrphan;
};

class __declspec(uuid("8340a7f2-413a-46dd-9f95-fbad5d455a90")) IJSHook: public IDispatch { };

// The JSHook class serves two purposes.  First, it implements a simple IDispatch interface
//   that is callable from JavaScript.  Its sole method, logNode(node), is called whenever
//   an node is created dynamically.  Each such node is stored in a set, along with all
//   of the static nodes.  When the user wants to detect leaks, this class goes through the
//   list, determining which nodes have a reference count greater than 1 (JSHook keeps one
//   reference on all nodes in order to keep them alive).
//
class JSHook: public IJSHook, public ATL::CComObjectRoot {
private:
	void addNode(MSHTML::IHTMLDOMNode* node);
	Elem* getElement(MSHTML::IHTMLDOMNodePtr node);
	void addNodeRecursively(MSHTML::IHTMLDOMNode* node);
	void releaseExtraReferences(MSHTML::IHTMLWindow2Ptr wnd);
	CStringW m_js;
	CLeakDlg* m_leakDlg;

public:	
	std::map<IUnknown*,Elem*> m_nodes;
	std::map<IUnknown*,Elem*>::iterator m_itNextNode;

	CMainBrowserDlg* m_mainBrowserDlg;
	JSHook();
	virtual ~JSHook();

	void clearNodes();
	void hookNewPage(MSHTML::IHTMLDocument2Ptr doc);
	void hookNewNode(MSHTML::IHTMLDOMNodePtr node, MSHTML::IHTMLDocument2Ptr doc );
	void addStaticNodes(MSHTML::IHTMLDocument2Ptr doc);
	size_t getNodeCount() const;

	void showLeaks(MSHTML::IHTMLWindow2Ptr wnd, CLeakDlg* dlg, bool showLeaks);
	void rescanForNodes(MSHTML::IHTMLDocument2Ptr doc);
	void countNodes(MSHTML::IHTMLWindow2Ptr wnd,int& leakedItems, int& hiddenItems);
	void setMainBrowserDlg(CMainBrowserDlg* dlg);
	void backgroundReleaseExtraReferences();
	void unloadWindow(MSHTML::IHTMLDocument2Ptr doc);
	void crossRefScan(MSHTML::IHTMLDocument2Ptr doc, CButton* button);
	void crossRefScanNode(MSHTML::IHTMLDOMNode2Ptr node);
	int getDocumentId(MSHTML::IHTMLDocument2Ptr doc);
	
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