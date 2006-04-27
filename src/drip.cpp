#include "stdafx.h"
#include "resource.h"
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

		// Create the main dialog and display it.  The application
		//   will end when this dialog is closed.
		//
		CMainBrowserDlg dlg;
		m_pMainWnd = &dlg;
		dlg.DoModal();

		return FALSE;
	}

	int ExitInstance() {
		OleUninitialize();
		return 0;
	}
} dripApp;
