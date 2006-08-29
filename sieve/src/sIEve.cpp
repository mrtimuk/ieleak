#include "stdafx.h"
#include "resource.h"
#include "JSHook.hpp"
#include "MainBrowserDlg.hpp"
#include "Cmallspy.h"
#include "MemLeakDetect.h"

ATL::CComModule _Module;

// Detect Memory Leaks or comment 
#ifdef _DEBUG
//CMemLeakDetect memLeakDetect;
#endif

// The main sIEve application.
//
class SIEveApp: public CWinApp {
public:
	CMallocSpy m_mallocspy;

	BOOL InitInstance() {
		OleInitialize(NULL);
		AfxEnableControlContainer();
		InitCommonControls();
		//CoRegisterMallocSpy( &m_mallocspy );   // Tijdelijk niet

		// Create the browser hook, which will be shared across all HTML documents.
		//
		CComObject<JSHook>* hook = NULL;
		CComObject<JSHook>::CreateInstance(&hook);
		hook->AddRef();
		// Create the main dialog and display it.  The application
		//   will end when this dialog is closed.
		//
		{
			// Scope the dialog so that its reference to the hook will be freed immediately
			//
			CMainBrowserDlg dlg(hook);
			dlg.m_mallocspy = &m_mallocspy;
			m_pMainWnd = &dlg;
			dlg.DoModal();
		}
		// JSHook::hookNewPage adds references when it calls __drip_initHook. However, it is unclear
		// when this reference is freed. At this point, simply release all references to the nodes
		// in case there are outstanding references to the hook.
		//
		hook->clearNodes();
		int refCnt = hook->Release();
		return FALSE;
	}

	int ExitInstance() {
		OleUninitialize();
		//CoRevokeMallocSpy();
		return 0;
	}
} sIEveApp;
