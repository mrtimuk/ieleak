#pragma once

class CLeakDlg;
class CMainBrowserDlg;

// This structure is used to maintain a list of hooked elements,
//   along with the URL of the document to which each belongs.
//

static int ElemSeqNr = 0;
struct Elem {
	Elem(BSTR url, BSTR nodeName) {
		this->seqNr = ElemSeqNr++;
		this->url = url;
		this->nodeName = nodeName;
		this->reported = 0; // To support red coloring of new reported elements
		this->leakReported = 0; // To support red coloring of new reported elements
		this->running = true; // To indicate a real leaked element
		this->hide = false;  // To support clear in use functionality
		this->docId = 0;
		this->docElem = NULL;
		this->documentHasLeaks = false;
	}
	int			seqNr;
	BSTR		url;
	BSTR		nodeName;
	int			reported;
	int			leakReported;
	BOOL		running;
	BOOL		hide;
	int			docId;
	Elem*		docElem;
	BOOL		documentHasLeaks;
};

class __declspec(uuid("8340a7f2-413a-46dd-9f95-fbad5d455a90")) IJSHook: public IDispatch { };

// The JSHook class serves two purposes.  First, it implements a simple IDispatch interface
//   that is callable from JavaScript.  Its sole method, logElement(elem), is called whenever
//   an element is created dynamically.  Each such element is stored in a set, along with all
//   of the static elements.  When the user wants to detect leaks, this class goes through the
//   list, determining which elements have a reference count greater than 1 (JSHook keeps one
//   reference on all elements in order to keep them alive).
//
class JSHook: public IJSHook, public ATL::CComObjectRoot {
private:
	void addElement(MSHTML::IHTMLDOMNode* elem);
	void addElementRecursively(MSHTML::IHTMLDOMNode* elem);
	void releaseExtraReferences(MSHTML::IHTMLWindow2Ptr wnd);
	Elem* addDocument(MSHTML::IHTMLDocument2* doc, MSHTML::IHTMLWindow2* wnd);
	Elem* getDocument(MSHTML::IHTMLDocument2* doc);
	Elem* addWindow(MSHTML::IHTMLDocument2* doc, MSHTML::IHTMLWindow2* wnd);
	Elem* getWindow(MSHTML::IHTMLWindow2* wnd);
	CStringW m_js;

public:	
	std::map<IUnknown*,Elem> m_elements;
	std::map<IUnknown*,Elem> m_runningDocs;
	std::map<IUnknown*,Elem>::iterator m_itNextNode;

	CMainBrowserDlg* m_mainBrowserDlg;
	JSHook();
	virtual ~JSHook();

	void clearElements();
	void hookNewPage(MSHTML::IHTMLDocument2Ptr doc);
	void hookNewElement(MSHTML::IHTMLDOMNodePtr elem, MSHTML::IHTMLDocument2Ptr doc );
	void addStaticElements(MSHTML::IHTMLDocument2Ptr doc);
	size_t getNodeCount() const;

	void showLeaks(MSHTML::IHTMLWindow2Ptr wnd, CLeakDlg* dlg, bool showLeaks);
	void rescanForElements(MSHTML::IHTMLDocument2Ptr doc);
	void countElements(MSHTML::IHTMLWindow2Ptr wnd,int& leakedItems, int& hiddenItems);
	void setMainBrowserDlg(CMainBrowserDlg* dlg);
	void backgroundReleaseExtraReferences();
	void unloadWindow(MSHTML::IHTMLDocument2Ptr doc);
	void crossRefScan(MSHTML::IHTMLDocument2Ptr doc, CButton* button);
	void crossRefScanElement(MSHTML::IHTMLDOMNode2Ptr elem);
	
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
