#ifndef _CMallocSpy_h
#define _CMallocSpy_h


struct MapEntry {
	MapEntry(BOOL inUse = false, void* pActual = 0, ULONG size = 0, int reported = 0 ) {
		this->pActual = pActual;
		this->size = size;
		this->inUse = inUse;
		this->reported = reported;
	}
	void*		pActual;
	ULONG		size;
	BOOL		inUse;
	int			reported;
};

class CLeakDlg;

class CMallocSpy : public IMallocSpy
{
public:
    CMallocSpy(void);
    ~CMallocSpy(void);

    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID *ppUnk);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);

    //
    // IMallocSpy methods
    //
    STDMETHOD_(ULONG, PreAlloc) (ULONG cbRequest);
    STDMETHOD_(void*, PostAlloc) (void* pActual);

    STDMETHOD_(void*, PreFree) (void* pRequest, BOOL fSpyed);
    STDMETHOD_(void, PostFree) (BOOL fSpyed);

    STDMETHOD_(ULONG, PreRealloc) (void* pRequest, ULONG cbRequest,
                                   void** ppNewRequest, BOOL fSpyed);
    STDMETHOD_(void*, PostRealloc) (void* pActual, BOOL fSpyed);

    STDMETHOD_(void*, PreGetSize) (void* pRequest, BOOL fSpyed);
    STDMETHOD_(ULONG, PostGetSize) (ULONG cbActual, BOOL fSpyed);

    STDMETHOD_(void*, PreDidAlloc) (void* pRequest, BOOL fSpyed);
    STDMETHOD_(BOOL, PostDidAlloc) (void* pRequest, BOOL fSpyed, BOOL fActual);

    STDMETHOD_(void, PreHeapMinimize) (void);
    STDMETHOD_(void, PostHeapMinimize) (void);

    //
    // Utilities ...
    //
    void Clear();
	void showLeaks(CLeakDlg* dlg);
    void Dump();
    void SetBreakAlloc(int allocNum);

protected:
    enum
    {
        HEADERSIZE = sizeof(int),
        MAX_ALLOCATIONS = 100000   // cannot handle more than max
    };

    ULONG   m_cRef;
    ULONG   m_cbRequest;
    int     m_counter;
    int     m_breakAlloc;

    char   *m_map;
    size_t  m_mapSize;

	MapEntry	*m_map2;
};


#endif   // _CMallocSpy_h

