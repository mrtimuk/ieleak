#include "stdafx.h"
#include "cmallspy.h"
#include "stdafx.h"
#include "resource.h"
#include "LeakDlg.hpp"
#include "PropDlg.hpp"

extern int dumpstacktrace();

// ******************************************************************
// ******************************************************************
// Constructor/Destructor
// ******************************************************************
// ******************************************************************


CMallocSpy::CMallocSpy(void)
{
    m_cRef = 0;
    m_counter = 0;
    m_mapSize = MAX_ALLOCATIONS;
    m_map = new char[m_mapSize];
	m_map2 = new MapEntry[m_mapSize];
    memset(m_map, 0, m_mapSize);
    memset(m_map2, 0, m_mapSize * sizeof(MapEntry));
}


CMallocSpy::~CMallocSpy(void)
{
    delete [] m_map;
	delete [] m_map2;
}


// ******************************************************************
// ******************************************************************
// IUnknown support ...
// ******************************************************************
// ******************************************************************


HRESULT CMallocSpy::QueryInterface(REFIID riid, LPVOID *ppUnk)
{
    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppUnk = (IUnknown *) this;
    }
    else if (IsEqualIID(riid, IID_IMallocSpy))
    {
        *ppUnk =  (IMalloc *) this;
    }
    else
    {
        *ppUnk = NULL;
        hr =  E_NOINTERFACE;
    }
    AddRef();
    return hr;
}


ULONG CMallocSpy::AddRef(void)
{
    return ++m_cRef;
}


ULONG CMallocSpy::Release(void)
{
    ULONG cRef;

    cRef = --m_cRef;
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}


// ******************************************************************
// ******************************************************************
// Utilities ...
// ******************************************************************
// ******************************************************************


void CMallocSpy::SetBreakAlloc(int allocNum)
{
    m_breakAlloc = allocNum;
}


void CMallocSpy::Clear()
{
	m_counter = 0;
    memset(m_map, 0, m_mapSize);
    memset(m_map2, 0, m_mapSize * sizeof(MapEntry));
}


void CMallocSpy::Dump()
{
    //::OutputDebugString("CMallocSpy dump ->\n");

    for (int i=1; i <= m_counter; i++)
    {
        if (m_map[i] != 0)
        {
            fprintf(stderr, " IMalloc memory leak at [%d]\n", i);
            //::OutputDebugString(buff);
        }
    }
//    ::OutputDebugString("CMallocSpy dump complete.\n");
}

void CMallocSpy::showLeaks(CLeakDlg* dlg)
{
    for (int i=1; i <= m_counter; i++)
    {
		if ( m_map2[i].inUse )
		{
			//TODO: Create an Elem() structure
//			dlg->addElement((IUnknown *)(m_map2[i].pActual), NULL,  m_map2[i].inUse, true, m_map2[i].size, m_map2[i].reported);
			m_map2[i].reported = 1;
		}
	}

	return;

	// Temp code to show freed elements; (for debug porposes)
    for (int i=1; i <= m_counter; i++)
    {
		if ( ! m_map2[i].inUse )
		{
			//TODO: Create an Elem() structure
//			dlg->addElement((IUnknown *)(m_map2[i].pActual), NULL, m_map2[i].inUse, true, m_map2[i].size, m_map2[i].reported);
			m_map2[i].reported = 1;
		}
    }
}


// ******************************************************************
// ******************************************************************
// IMallocSpy methods ...
// ******************************************************************
// ******************************************************************


ULONG CMallocSpy::PreAlloc(ULONG cbRequest)
{
    m_cbRequest = cbRequest;
    return cbRequest + HEADERSIZE;
}


void *CMallocSpy::PostAlloc(void *pActual)
{
    m_counter++;

 //   if (m_breakAlloc == m_counter)
 //       ::DebugBreak();

    //
    // Store the allocation counter and note that this allocation
    // is active in the map.
    //
    memcpy(pActual, &m_counter, 4);
    m_map[m_counter] = 1;
	m_map2[m_counter].inUse = true;
	m_map2[m_counter].pActual = pActual;
	m_map2[m_counter].size = m_cbRequest;

    return (void*)((BYTE*)pActual + HEADERSIZE);
}


void *CMallocSpy::PreFree(void *pRequest, BOOL fSpyed)
{
    if (pRequest == NULL)
    {
        return NULL;
    }

    if (fSpyed)
    {
        //
        // Mark the allocation as inactive in the map.
        //
        int counter;
		// wchar_t	buf[50];

		pRequest = (void*)(((BYTE*)pRequest) - HEADERSIZE);

		memcpy(&counter, pRequest, 4);
		//if ( counter > m_counter )
		//{
		//	wsprintf(buf,L"Free element: %d > last Element %d",counter,m_counter);
		//	AfxMessageBox (buf,MB_ICONINFORMATION|MB_OK);
		//}

        m_map[counter] = 0;
		m_map2[counter].inUse = false;
// 		m_map2[counter].pActual = 0;
//		m_map2[counter].size = 0;

        return pRequest;
    }
    else
    {
        return pRequest;
    }
}


void CMallocSpy::PostFree(BOOL fSpyed)
{
    return;
}


ULONG CMallocSpy::PreRealloc(void *pRequest,
                             ULONG cbRequest,
                             void **ppNewRequest,
                             BOOL fSpyed)
{
    if (fSpyed  &&  pRequest != NULL)
    {
        //
        // Mark the allocation as inactive in the map since IMalloc::Realloc()
        // frees the originally allocated block.
        //
		m_cbRequest = cbRequest;
        int counter;
        BYTE* actual = (BYTE*)pRequest - HEADERSIZE;
        memcpy(&counter, actual, 4);
        m_map[counter] = 0;
		m_map2[counter].inUse = false;
//		m_map2[counter].pActual = 0;
//		m_map2[counter].size = 0;

        *ppNewRequest = (void*)(((BYTE*)pRequest) - HEADERSIZE);
        return cbRequest + HEADERSIZE;
    }
    else
    {
        *ppNewRequest = pRequest;
        return cbRequest;
    }
}


void *CMallocSpy::PostRealloc(void *pActual, BOOL fSpyed)
{
    if (fSpyed)
    {
        m_counter++;

        if (m_breakAlloc == m_counter)
            ::DebugBreak();

        //
        // Store the allocation counter and note that this allocation
        // is active in the map.
        //
        memcpy(pActual, &m_counter, 4);
		m_map[m_counter] = 1;
		m_map2[m_counter].inUse = true;
		m_map2[m_counter].pActual = pActual;
		m_map2[m_counter].size = m_cbRequest;

        return (void*)((BYTE*)pActual + HEADERSIZE);
    }
    else
    {
        return pActual;
    }
}


void *CMallocSpy::PreGetSize(void *pRequest, BOOL fSpyed)
{
    if (fSpyed)
        return (void *) (((BYTE *) pRequest) - HEADERSIZE);
    else
        return pRequest;
}


ULONG CMallocSpy::PostGetSize(ULONG cbActual, BOOL fSpyed)
{
    if (fSpyed)
        return cbActual - HEADERSIZE;
    else
        return cbActual;
}


void *CMallocSpy::PreDidAlloc(void *pRequest, BOOL fSpyed)
{
    if (fSpyed)
        return (void *) (((BYTE *) pRequest) - HEADERSIZE);
    else
        return pRequest;
}


BOOL CMallocSpy::PostDidAlloc(void *pRequest, BOOL fSpyed, BOOL fActual)
{
    return fActual;
}


void CMallocSpy::PreHeapMinimize(void)
{
    return;
}


void CMallocSpy::PostHeapMinimize(void)
{
    return;
}