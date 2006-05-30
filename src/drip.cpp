#include "stdafx.h"
#include "resource.h"
#include "JSHook.hpp"
#include "MainBrowserDlg.hpp"

#include "nanocppunit/test.h"

ATL::CComModule _Module;

// The main drip application.
//
class DripApp: public CWinApp {
public:
	BOOL InitInstance() {
		OleInitialize(NULL);
		AfxEnableControlContainer();
		InitCommonControls();

#ifdef TEST

		bool result (runAllTests());
		if (!result)
			return result;

#endif //#ifdef TEST

		// Create the browser hook, which will be shared across all HTML documents.
		//
		CComObject<JSHook>* hook = NULL;
		CComObject<JSHook>::CreateInstance(&hook);
		hook->AddRef();

		// Create the main dialog and display it.  The application
		//   will end when this dialog is closed.
		//
		CMainBrowserDlg dlg(hook);
		m_pMainWnd = &dlg;
		dlg.DoModal();

		// Release the browser hook.
		//
		hook->Release();

		return FALSE;
	}

	int ExitInstance() {
		OleUninitialize();
		return 0;
	}
} dripApp;
