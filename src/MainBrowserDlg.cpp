#include "stdafx.h"
#include "MainBrowserDlg.hpp"
#include "BrowserPopupDlg.hpp"
#include "DlgResizeHelper.h"
#include "JSHook.hpp"
#include "LeakDlg.hpp"
#include "resource.h"

#define TIMER_AUTO_REFRESH		0
#define TIMER_MONITOR_MEMORY	1
#define TIMER_CHECK_LEAKS	2

void GetMemoryUsage(wchar_t *pszUsage)
{
	PROCESS_MEMORY_COUNTERS procMem;
	memset(&procMem, 0, sizeof(PROCESS_MEMORY_COUNTERS));
	procMem.cb = sizeof(PROCESS_MEMORY_COUNTERS);
	GetProcessMemoryInfo(GetCurrentProcess(), &procMem, procMem.cb);

	_itow((int)procMem.WorkingSetSize, pszUsage, 10);
}

BEGIN_MESSAGE_MAP(CMainBrowserDlg, CBrowserHostDlg)
	//{{AFX_MSG_MAP(CMainBrowserDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDOK, doNothing)
	ON_BN_CLICKED(IDCANCEL, doNothing)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_GO, OnBnClickedGo)
	ON_BN_CLICKED(IDC_BACK, OnBnClickedBack)
	ON_BN_CLICKED(IDC_FORWARD, OnBnClickedForward)
	ON_BN_CLICKED(IDC_CHECKLEAKS, OnBnClickedCheckLeaks)
	ON_BN_CLICKED(IDC_AUTOREFRESH, OnBnClickedAutoRefresh)
END_MESSAGE_MAP()

CMainBrowserDlg::CMainBrowserDlg(CWnd* pParent)	: CBrowserHostDlg(IDC_EXPLORER, CMainBrowserDlg::IDD, pParent),
	m_waitingForDoc(false), m_waitingForBlankDoc(false), m_autoRefreshMode(false), m_checkLeakDoc(NULL)
{
	//{{AFX_DATA_INIT(CMainBrowserDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);

	// Create the browser hook, which will be shared across all HTML documents.
	//
	CComObject<JSHook>::CreateInstance(&m_hook);
	m_hook->AddRef();
}

CMainBrowserDlg::~CMainBrowserDlg() {
	// Release the browser hook.
	//
	m_hook->Release();
}

void CMainBrowserDlg::DoDataExchange(CDataExchange* pDX) {
	CBrowserHostDlg::DoDataExchange(pDX);
}

BOOL CMainBrowserDlg::OnInitDialog() {
	CBrowserHostDlg::OnInitDialog();

	// Set the application icon.
	//
	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	// Disable the leak check button.
	//
	GetDlgItem(IDC_CHECKLEAKS)->EnableWindow(FALSE);

	// Focus the URL edit control.
	//
	GetDlgItem(IDC_EDITURL)->SetFocus();

	// Enable memory monitoring
	//
	SetTimer(TIMER_MONITOR_MEMORY, 100, NULL);

	// Set up resizing
	m_resizeHelper.Init(m_hWnd);
	m_resizeHelper.Fix(IDC_AUTOREFRESH, DlgResizeHelper::kWidthRight, DlgResizeHelper::kHeightTop);
	m_resizeHelper.Fix(IDC_CHECKLEAKS, DlgResizeHelper::kWidthRight, DlgResizeHelper::kHeightTop);
	m_resizeHelper.Fix(IDC_GO, DlgResizeHelper::kWidthRight, DlgResizeHelper::kHeightTop);
	m_resizeHelper.Fix(IDC_EDITURL, DlgResizeHelper::kLeftRight, DlgResizeHelper::kHeightTop);
	m_resizeHelper.Fix(IDC_ADDRESS_STATIC, DlgResizeHelper::kWidthLeft, DlgResizeHelper::kHeightTop);
	m_resizeHelper.Fix(IDC_FORWARD, DlgResizeHelper::kWidthLeft, DlgResizeHelper::kHeightTop);
	m_resizeHelper.Fix(IDC_BACK, DlgResizeHelper::kWidthLeft, DlgResizeHelper::kHeightTop);
	m_resizeHelper.Fix(IDC_TOTALMEMLABEL, DlgResizeHelper::kWidthRight, DlgResizeHelper::kHeightTop);
	m_resizeHelper.Fix(IDC_CURRENT_MEMORY_EDIT, DlgResizeHelper::kWidthRight, DlgResizeHelper::kHeightTop);
	m_resizeHelper.Fix(IDC_MEMLABEL, DlgResizeHelper::kWidthRight, DlgResizeHelper::kHeightTop);
	m_resizeHelper.Fix(IDC_MEMLIST, DlgResizeHelper::kWidthRight, DlgResizeHelper::kTopBottom);
	m_resizeHelper.Fix(IDC_EXPLORER, DlgResizeHelper::kLeftRight, DlgResizeHelper::kTopBottom);
	m_resizeHelper.Fix(getExplorerHwnd(), DlgResizeHelper::kLeftRight, DlgResizeHelper::kTopBottom);

	// Navigate to a page so that the border appears in the browser
	//
	go();

	// Must return false when focusing a control.
	//
	return FALSE;
}

void CMainBrowserDlg::OnPaint() {
	if (IsIconic()) {
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle.
		//
		CRect rect;
		GetClientRect(&rect);
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon.
		//
		dc.DrawIcon(x, y, m_hIcon);
	}
	else {
		CBrowserHostDlg::OnPaint();
	}
}

HCURSOR CMainBrowserDlg::OnQueryDragIcon() {
	return (HCURSOR) m_hIcon;
}

void CMainBrowserDlg::OnSize(UINT nType, int cx, int cy) {
	m_resizeHelper.OnSize();
}

void CMainBrowserDlg::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	// Using arbitrary figures here (dialog units would be better)
	//
	lpMMI->ptMinTrackSize.x = 500;
	lpMMI->ptMinTrackSize.y = 300;

	CWnd::OnGetMinMaxInfo(lpMMI);
}

void CMainBrowserDlg::OnClose() {
	DestroyWindow();
}

void CMainBrowserDlg::OnDestroy() {
	// Destroy the popups without navigating to blank page
	//
	for (std::vector<CBrowserPopupDlg*>::const_iterator it = m_popups.begin(); it != m_popups.end(); ++it) {
		CBrowserPopupDlg *dlg = *it;
		dlg->DestroyWindow();
		delete dlg;
	}
	m_popups.clear();

	// When the main dialog is destroyed, quit the application.
	//
	PostQuitMessage(0);
}

void CMainBrowserDlg::doNothing() {
}

LRESULT CMainBrowserDlg::WindowProc(UINT message, WPARAM wparam, LPARAM lparam) {
	return CBrowserHostDlg::WindowProc(message, wparam, lparam);
}

void CMainBrowserDlg::OnTimer(UINT_PTR nIDEvent) {
	switch (nIDEvent) {
		case TIMER_AUTO_REFRESH:
			KillTimer(nIDEvent);
			go();
			break;

		case TIMER_MONITOR_MEMORY:
			wchar_t memText[32];
			GetMemoryUsage(memText);
			GetDlgItem(IDC_CURRENT_MEMORY_EDIT)->SendMessage(WM_SETTEXT, 0, (LPARAM)memText);
			break;

		case TIMER_CHECK_LEAKS:
			if (m_checkLeakDoc == NULL) {
				KillTimer(nIDEvent);
				ASSERT(false);
				break;
			}

			// Display the leak dialog if all the popups are finished
			//
			destroyFinishedPopups();
			if (m_popups.begin() == m_popups.end()) {
				KillTimer(nIDEvent);

				CLeakDlg leakDlg(this);
				m_hook->showLeaks(m_checkLeakDoc->parentWindow, &leakDlg);
				GetDlgItem(IDC_CHECKLEAKS)->EnableWindow(FALSE);
				leakDlg.DoModal();
			}
			break;
	}
}

// Gets the text in the url edit control.  The caller must free it.
//
wchar_t* CMainBrowserDlg::getUrlText() {
	CWnd* item = GetDlgItem(IDC_EDITURL);
	int len = (int)item->SendMessage(WM_GETTEXTLENGTH, 0, 0);
	
	// Navigating to a blank URL will cause the browser not to show a document at all.
	// Either use about:blank or bulletproof this dialog against NULL document pointers.
	//
	if (!len) {
		wchar_t about_blank[] = L"about:blank";
		wchar_t* text = new wchar_t[sizeof(about_blank)];
		memcpy(text, about_blank, sizeof(about_blank));
		return text;
	}

	wchar_t* text = new wchar_t[len + 1];
	item->SendMessage(WM_GETTEXT, (WPARAM)len + 1, (LPARAM)text);
	return text;
}

// Loads the document specified by the url edit control.
//
void CMainBrowserDlg::go() {
	wchar_t* url = getUrlText();
	Navigate(url);
	delete[] url;
}

void CMainBrowserDlg::OnBnClickedBack() {
	GoBack();
}

void CMainBrowserDlg::OnBnClickedForward() {
	GoForward();
}

void CMainBrowserDlg::OnBnClickedGo() {
	if (m_waitingForDoc) {
		// (The button says 'stop')
		//   If waiting for a document, cancel it, set the button back to
		//   'go', and enable the leak test and auto-refresh buttons.
		//
		m_waitingForDoc = false;
		GetDlgItem(IDC_GO)->SendMessage(WM_SETTEXT, 0, (LPARAM)L"Go");
		GetDlgItem(IDC_DRIP)->EnableWindow(TRUE);
		GetDlgItem(IDC_AUTOREFRESH)->EnableWindow(TRUE);
		Stop();
	}
	else {
		// (The button says 'go')
		//   Load the specified document, set the button to 'stop', and
		//   disable the leak test and auto-refresh buttons.
		//
		m_waitingForDoc = true;
		GetDlgItem(IDC_GO)->SendMessage(WM_SETTEXT, 0, (LPARAM)L"Stop");
		GetDlgItem(IDC_DRIP)->EnableWindow(FALSE);
		GetDlgItem(IDC_AUTOREFRESH)->EnableWindow(FALSE);
		go();
	}
}

void CMainBrowserDlg::OnBnClickedCheckLeaks() {
	// When the leak test button is pressed, navigate to the blank document
	//   so that the browser will release all of its elements.  Set
	//   m_waitingForBlankDoc to true so that the DocumentComplete event
	//   handler will know to check for leaks when the blank document
	//   finishes loading.
	//
	requestClosePopups();
	Navigate(L"about:blank");
	m_waitingForBlankDoc = true;
}

void CMainBrowserDlg::OnBnClickedAutoRefresh() {
	if (!m_autoRefreshMode) {
		// (The button says 'auto refresh')
		//   Start automatically refreshing ("blowing memory"), change the button to 'stop', and disable
		//   the go and leak test buttons.
		//
		m_autoRefreshMode = true;
		GetDlgItem(IDC_AUTOREFRESH)->SendMessage(WM_SETTEXT, 0, (LPARAM)L"Stop");
		GetDlgItem(IDC_GO)->EnableWindow(FALSE);
		GetDlgItem(IDC_DRIP)->EnableWindow(FALSE);
		GetDlgItem(IDC_MEMLIST)->SendMessage(LB_RESETCONTENT, 0, 0);

		// Load the specified document, which will start the auto-refresh cycle.
		//
		go();
	}
	else {
		// (The button says 'stop')
		//   Stop auto-refresh, change the button back to 'auto refresh', and
		//   re-enable the go and leak test buttons.
		//
		m_autoRefreshMode = false;
		GetDlgItem(IDC_AUTOREFRESH)->SendMessage(WM_SETTEXT, 0, (LPARAM)L"Auto-Refresh");
		GetDlgItem(IDC_GO)->EnableWindow(TRUE);
		GetDlgItem(IDC_DRIP)->EnableWindow(TRUE);
	}
}

void CMainBrowserDlg::requestClosePopups()
{
	for (std::vector<CBrowserPopupDlg*>::const_iterator it = m_popups.begin(); it != m_popups.end(); ++it) {
		CBrowserPopupDlg *dlg = *it;
		dlg->requestClose();
	}
}

void CMainBrowserDlg::destroyFinishedPopups()
{
	for (std::vector<CBrowserPopupDlg*>::iterator it = m_popups.begin(); it != m_popups.end(); ++it) {
		CBrowserPopupDlg *dlg = *it;
		if (dlg->isFinished()) {
			dlg->DestroyWindow();
			delete dlg;
			m_popups.erase(it);
			it--;
		}
	}
}

void CMainBrowserDlg::onTitleChange(LPCTSTR lpszText) {
	CStringW title(lpszText);
	if (title.IsEmpty())
		SetWindowText(CStringW(L"Drip"));
	else
		SetWindowText(CStringW(L"Drip - ") + CStringW(lpszText));
}

void CMainBrowserDlg::onOuterDocumentLoad(MSHTML::IHTMLDocument2Ptr doc)
{
	CBrowserHostDlg::onOuterDocumentLoad(doc);

	// If we're in 'auto-refresh' mode, display the current memory usage
	//   and refresh the document.
	//
	if (m_autoRefreshMode) {
		// Make sure that the browser's GC pass is done so that we don't report
		//   as-yet-uncollected memory.
		//
		doc->parentWindow->execScript(L"window.CollectGarbage()", L"javascript");

		// Get the process' memory usage and add it to the list.
		//
		wchar_t memText[32];
		GetMemoryUsage(memText);
		GetDlgItem(IDC_MEMLIST)->SendMessage(LB_INSERTSTRING, 0, (LPARAM)memText);

		// Reload the document in 500 ms.
		//
		SetTimer(0, 500, NULL);
	}

	// If we're waiting for a blank document (that is, we're waiting for the
	//   current document to fully unload so that we can check for leaks),
	//   then check for leaks and display the leak dialog.
	//
	else if (m_waitingForBlankDoc) {
		m_waitingForBlankDoc = false;
		SetTimer(TIMER_CHECK_LEAKS, 100, NULL);
		m_checkLeakDoc = doc;
	}

	// If we're simply waiting on the page to load, re-enable the drip, leak
	//   test, and auto-refresh buttons, and set the 'stop' button back to 'go'.
	//
	else {
		GetDlgItem(IDC_DRIP)->EnableWindow(TRUE);
		GetDlgItem(IDC_AUTOREFRESH)->EnableWindow(TRUE);
		GetDlgItem(IDC_GO)->SendMessage(WM_SETTEXT, 0, (LPARAM)L"Go");
		GetDlgItem(IDC_CHECKLEAKS)->EnableWindow(TRUE);
		m_waitingForDoc = false;
	}
}

void CMainBrowserDlg::onNewWindow(CBrowserHostDlg** ppDlg)
{
	CBrowserPopupDlg *dlg = new CBrowserPopupDlg(m_hook,&m_popups,this);
	dlg->Create(CBrowserPopupDlg::IDD);
	dlg->ShowWindow(SW_SHOW);
	m_popups.push_back(dlg);

	*ppDlg = dlg;
}
