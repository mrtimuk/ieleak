#include "stdafx.h"
#include "MainBrowserDlg.hpp"
#include "BrowserPopupDlg.hpp"
#include "JSHook.hpp"
#include "LeakDlg.hpp"
#include "resource.h"
#include "Cmallspy.h"
#include "VORegistry.h"
#include "mainbrowserdlg.hpp"
#include <afxpriv.h>

#define TIMER_AUTO_REFRESH			0
#define TIMER_MONITOR_MEMORY		1
#define TIMER_ABOUTBLANK			2

#define TIMER_MONITOR_MEMORY_SLOW	5000
#define TIMER_MONITOR_MEMORY_FAST	1000
#define TIMER_MONITOR_MEMORY_PAUSED	0

LONG memUsage = 0;
LONG nrMemSamples = 0;
LONG autoFreedRefs = 0;

void showMemoryUsageAndAvarageGrowth(CListCtrl &m_memsamples, int items, int itemsLeaked, int hiddenItems)
{
	TCHAR memUsageText[32];
	TCHAR memDeltaText[32];
	TCHAR itemsInUseText[32];
	TCHAR itemsLeakedText[32];
	//TCHAR autoFreedRefsText[32];
	LONG currentMemUsage;


	PROCESS_MEMORY_COUNTERS procMem;
	memset(&procMem, 0, sizeof(PROCESS_MEMORY_COUNTERS));
	procMem.cb = sizeof(PROCESS_MEMORY_COUNTERS);
	GetProcessMemoryInfo(GetCurrentProcess(), &procMem, procMem.cb);
	currentMemUsage = (LONG) (procMem.WorkingSetSize);

	if ( ! memUsage ) memUsage = currentMemUsage; //First 
	LONG incr =  currentMemUsage - memUsage;
	memUsage = currentMemUsage;
	wsprintf(memUsageText,L"%d",memUsage >> 10);
	wsprintf(memDeltaText,L"%d",incr >> 10);
	wsprintf(itemsInUseText,L"%d",items - hiddenItems);
	wsprintf(itemsLeakedText,L"%d",itemsLeaked);

	nrMemSamples++;

	m_memsamples.InsertItem(0,L"");
	m_memsamples.SetItemText(0, 1, memUsageText);
	m_memsamples.SetItemText(0, 2, memDeltaText);
	m_memsamples.SetItemText(0, 3, itemsInUseText);
	m_memsamples.SetItemText(0, 4, itemsLeakedText);
	m_memsamples.SetItemData(0,(DWORD_PTR)incr);

	if ( nrMemSamples > 200 ) m_memsamples.DeleteItem(200);  // Show maximum 200 samples to avoid leaking in this tool :-)

}

void initializeMemorySamples(CMainBrowserDlg* dlg)
{
	memUsage = 0;
	nrMemSamples = 0;
	autoFreedRefs = 0;
	dlg->m_memsamples.DeleteAllItems();
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
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_BN_CLICKED(IDOK, doNothing)
	ON_BN_CLICKED(IDCANCEL, doNothing)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_GO, OnBnClickedGo)
	ON_BN_CLICKED(IDC_BACK, OnBnClickedBack)
	ON_BN_CLICKED(IDC_FORWARD, OnBnClickedForward)
	ON_BN_CLICKED(IDC_ABOUTBLANK, OnBnClickedAboutBlank)
	ON_BN_CLICKED(IDC_AUTOREFRESH, OnBnClickedAutoRefresh)
	ON_BN_CLICKED(IDC_SHOW_IN_USE, OnBnClickedShowInUse)
	ON_BN_CLICKED(IDC_CLEAR_IN_USE, OnBnClickedClearInUse)
	ON_BN_CLICKED(IDC_SHOW_LEAKS, OnBnClickedShowLeaks)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_MEMSAMPLES, OnNMCustomdrawMemsamples)
	ON_BN_CLICKED(IDC_CHECK_AUTO_CLEANUP, &CMainBrowserDlg::OnBnClickedCheckAutoCleanup)
	ON_BN_CLICKED(IDC_CHECK_CYCLE_DETECTION, &CMainBrowserDlg::OnBnClickedCheckCycleDetection)
	ON_BN_CLICKED(IDC_LOG_DEFECT, &CMainBrowserDlg::OnBnClickedLogDefect)
	ON_BN_CLICKED(IDC_SHOW_HELP, &CMainBrowserDlg::OnBnClickedShowHelp)
	ON_BN_CLICKED(IDC_RADIO_SLOW, &CMainBrowserDlg::OnBnClickedRadioSlow)
	ON_BN_CLICKED(IDC_RADIO_FAST, &CMainBrowserDlg::OnBnClickedRadioFast)
	ON_BN_CLICKED(IDC_RADIO_PAUSED, &CMainBrowserDlg::OnBnClickedRadioPaused)
	ON_BN_CLICKED(IDC_CROSSREF_SCAN, &CMainBrowserDlg::OnBnClickedCrossrefScan)
END_MESSAGE_MAP()

//EASYSIZE(<control id>,left,top,right,bottom,options)
//l,t,r,b = ES_KEEPSIZE or ES_BORDER or <control id> (keep distance to border or <control id>)
//options = ES_HCENTER | ES_VCENTER or 0

BEGIN_EASYSIZE_MAP(CMainBrowserDlg)
	//EASYSIZE(<control id>,		left,			top,			right,			bottom,		options)

	//Anchored to top/right
	EASYSIZE(IDC_AUTOREFRESH,		ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_ABOUTBLANK,		ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_CLEAR_IN_USE,		ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_SHOW_IN_USE,		ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_SHOW_LEAKS,		ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_SHOW_HELP,			ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_LOG_DEFECT,		ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_CHECK_AUTO_CLEANUP,ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_CROSSREF_SCAN,		ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_CHECK_CYCLE_DETECTION,ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_GO,				ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_MEMLABEL,			ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)

	//Anchored to right + bottom
	EASYSIZE(IDC_RADIO_FAST,		ES_KEEPSIZE,	ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		0)
	EASYSIZE(IDC_RADIO_SLOW,		ES_KEEPSIZE,	ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		0)
	EASYSIZE(IDC_RADIO_PAUSED,		ES_KEEPSIZE,	ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		0)

	//Anchored to top + right + bottom
	EASYSIZE(IDC_MEMSAMPLES,		ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_BORDER,		0)

	//Anchored to left + top + right
	EASYSIZE(IDC_EDITURL,			ES_BORDER,		ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)

	//Anchored to left + bottom
	EASYSIZE(IDC_STATIC_MIN,		ES_BORDER,		ES_KEEPSIZE,	ES_KEEPSIZE,	ES_BORDER,		0)
	EASYSIZE(IDC_STATIC_MAX,		ES_BORDER,		ES_KEEPSIZE,	ES_KEEPSIZE,	ES_BORDER,		0)

	//Anchored to left + right + bottom
	EASYSIZE(IDC_STATIC_GRAPH,		ES_BORDER,		ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		0)
	EASYSIZE(IDC_STATIC_HISTORY,	ES_BORDER,		ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		0)

	//Anchored to left + top + right + bottom
	EASYSIZE(IDC_EXPLORER,			ES_BORDER,		ES_BORDER,		ES_BORDER,		ES_BORDER,		0)

	//EASYSIZE(<control id>,		left,			top,			right,			bottom,		options)	
END_EASYSIZE_MAP


CMainBrowserDlg::CMainBrowserDlg(CComObject<JSHook>* hook, CWnd* pParent)	: CBrowserHostDlg(hook, IDC_EXPLORER, CMainBrowserDlg::IDD, pParent),
	m_waitingForDoc(false), m_waitingForBlankDoc(false), m_autoRefreshMode(false), m_checkLeakDoc(NULL), m_reg(HKEY_LOCAL_MACHINE, TEXT("Software\\IE Sieve"))
	, m_check_auto_cleanup(false), m_check_cycle_detection(false)
{
	//{{AFX_DATA_INIT(CMainBrowserDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON2);
	m_leakDlg = NULL;

	getHook()->setMainBrowserDlg(this);
}

void showHelp()
{
	IWebBrowser2* m_pBrowser;
	HRESULT hr1 ;
    COleVariant null; 

    //Create an Instance of web browser
    hr1 = CoCreateInstance (CLSID_InternetExplorer, NULL, 
        CLSCTX_LOCAL_SERVER, 
        IID_IWebBrowser2, (LPVOID *)&m_pBrowser); 
    if(hr1==S_OK)
    {
        VARIANT_BOOL pBool=true; //The browser is invisible
		//COleVariant vaURL(L"http://cwiki/wiki/index.php/SIEve") ; 
		COleVariant vaURL(L"http://sourceforge.net/projects/ieleak") ; 

		m_pBrowser->put_MenuBar(true);
		m_pBrowser->put_ToolBar(true);
        m_pBrowser->Navigate2(vaURL,null,null,null,null) ; 
	    m_pBrowser->put_Visible( pBool ) ; 
	}
}


void logDefect()
{
	IWebBrowser2* m_pBrowser;
	HRESULT hr1 ;
    COleVariant null; 

    //Create an Instance of web browser
    hr1 = CoCreateInstance (CLSID_InternetExplorer, NULL, 
        CLSCTX_LOCAL_SERVER, 
        IID_IWebBrowser2, (LPVOID *)&m_pBrowser); 
    if(hr1==S_OK)
    {
        VARIANT_BOOL pBool=true; //The browser is invisible
  		//COleVariant vaURL(L"http://scrat.vanenburg.com/components/sieve/newticket") ; 
 		COleVariant vaURL(L"http://sourceforge.net/tracker/?func=add&group_id=165799&atid=837173") ; 

		m_pBrowser->put_MenuBar(true);
		m_pBrowser->put_ToolBar(true);
        m_pBrowser->Navigate2(vaURL,null,null,null,null) ; 
	    m_pBrowser->put_Visible( pBool ) ; 
	}
}

CMainBrowserDlg::~CMainBrowserDlg() {
	delete m_leakDlg;
}

void CMainBrowserDlg::DoDataExchange(CDataExchange* pDX) {
	CBrowserHostDlg::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MEMSAMPLES, m_memsamples);
	DDX_Control(pDX, IDC_RADIO_SLOW, m_radioSlow);
	DDX_Control(pDX, IDC_RADIO_FAST, m_radioFast);
	DDX_Control(pDX, IDC_RADIO_PAUSED, m_radioPaused);
}

BOOL CMainBrowserDlg::OnInitDialog() {
	CBrowserHostDlg::OnInitDialog();

	m_memGraph.SubclassDlgItem(IDC_STATIC_GRAPH, this);
	//m_memGraph.AddPoint(0);
	m_radioSlow.SetCheck(0);
	m_radioFast.SetCheck(0);
	m_radioPaused.SetCheck(1);
	m_timerMemoryMonitor = TIMER_MONITOR_MEMORY_FAST; // Default slow


	// Set the application icon.
	//
	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	// Disable the about:blank button.
	//
	GetDlgItem(IDC_ABOUTBLANK)->EnableWindow(FALSE);

	m_memsamples.InsertColumn(0, L" ", LVCFMT_LEFT, 1);
	m_memsamples.InsertColumn(1, L"usage", LVCFMT_RIGHT, 55);
	m_memsamples.InsertColumn(2, L"delta", LVCFMT_RIGHT, 50);
	m_memsamples.InsertColumn(3, L"#inUse", LVCFMT_RIGHT, 48);
	m_memsamples.InsertColumn(4, L"#leaks", LVCFMT_RIGHT, 45);

	// Navigate to a page so that the border appears in the browser
	//
	go();

	// Focus the URL edit control and set default URL value.
	//
	GetDlgItem(IDC_EDITURL)->SetFocus();

	CVOString m_lastusedUrl = m_reg.ReadString(L"MRU_Url");
	GetDlgItem(IDC_EDITURL)->SetWindowText(m_lastusedUrl);
	GetDlgItem(IDC_EDITURL)->SendMessage(EM_SETSEL,0,-1);

	m_brush.CreateSysColorBrush( COLOR_3DFACE  );
	INIT_EASYSIZE;
	// Must return false when focusing a control.
	//
	return FALSE;
}

/*
int rescansCount = 0;
void CMainBrowserDlg::rescans()
{
	rescansCount++;
	TCHAR rs[10];
	wsprintf(rs,L"%d",rescansCount);
	GetDlgItem(IDC_EDIT_RESCANS)->SetWindowText(rs);
}
*/

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

HBRUSH CMainBrowserDlg::OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor)
{
	HBRUSH hbr;

    switch ( pWnd->GetDlgCtrlID() )
    {
		case IDC_STATIC_MAX:
		case IDC_STATIC_MIN:
			pDC->SetTextColor(RGB(0,255,0));
			pDC->SetBkColor(RGB(0,0,0));
			hbr = (HBRUSH) m_brush;
			break;
		default:
			hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
			break;
    } 
	return hbr;
}

HCURSOR CMainBrowserDlg::OnQueryDragIcon() {
	return (HCURSOR) m_hIcon;
}

void CMainBrowserDlg::OnSize(UINT nType, int cx, int cy) {
	CDialog::OnSize(nType, cx, cy);
	UPDATE_EASYSIZE;
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

LRESULT CMainBrowserDlg::OnKickIdle(WPARAM, LPARAM lCount) {
	getHook()->backgroundReleaseExtraReferences();
	return (lCount < (LPARAM)getHook()->getNodeCount());
}

void CMainBrowserDlg::OnTimer(UINT_PTR nIDEvent)
{	
	switch (nIDEvent) {
		case TIMER_AUTO_REFRESH:
			KillTimer(nIDEvent);
			go();
			break;

		case TIMER_MONITOR_MEMORY:
			updateStatistics();
			break;

		case TIMER_ABOUTBLANK:
			if (m_checkLeakDoc == NULL) {
				KillTimer(nIDEvent);
				break;
			}

			// Display the leak dialog if all the popups are finished
			//
			destroyFinishedPopups();
			if (m_popups.begin() == m_popups.end()) {
				KillTimer(nIDEvent);
			}
			if ( m_check_auto_cleanup ) autoCleanup();
			if ( m_leakDlg ) OnBnClickedShowLeaks();
			break;
	}
}

void CMainBrowserDlg::updateStatistics()
{
	TCHAR min_text[32]; TCHAR max_text[32];
	int min, max;
	int items;
	int leakedItems;
	int hiddenItems;

	items = (int) (getHook()->m_elements.size());
	items += (int) (getHook()->m_runningDocs.size());
	getHook()->countElements(m_loadedDoc->parentWindow,leakedItems, hiddenItems);
	showMemoryUsageAndAvarageGrowth(m_memsamples, items, leakedItems, hiddenItems);
	m_memGraph.AddPoint((int)memUsage >> 10);

	m_memGraph.GetMinMax(min,max);
	wsprintf(min_text,L"%d (MB)      ",(min-1024) >> 10);
	wsprintf(max_text,L"%d (MB)      ",(max+1024) >> 10);
	GetDlgItem(IDC_STATIC_MIN)->SetWindowText(min_text);
	GetDlgItem(IDC_STATIC_MAX)->SetWindowText(max_text);
}

void CMainBrowserDlg::autoCleanup()
{
	for (std::map<IUnknown*,Elem>::iterator it = getHook()->m_elements.begin(); it != getHook()->m_elements.end(); ++it) {
		IUnknown *unk = it->first;
		Elem &elem = it->second;

		// For each element, AddRef() and Release() it.  The latter method will return
		//   the current ref count.
		//
		unk->AddRef();
		int refCount = unk->Release();

		if ( refCount > 1 && ! elem.docElem->running )
		{
			if ( wcscmp(elem.nodeName,L"#window") )
			{
				for ( int i = 0; i < refCount-1 ; i++)
				{
					autoFreedRefs++;
					unk->Release();
				}
			}
			else
			{
				AfxMessageBox(L"Don't free #window");
			}
		}
	}

	for (std::map<IUnknown*,Elem>::iterator it = getHook()->m_runningDocs.begin(); it != getHook()->m_runningDocs.end(); ++it) {
		IUnknown *unk = it->first;
		Elem &elem = it->second;

		// For each element, AddRef() and Release() it.  The latter method will return
		//   the current ref count.
		//
		unk->AddRef();
		int refCount = unk->Release();

		if ( refCount > 1 && ! elem.docElem->running )
		{
			for ( int i = 0; i < refCount-1 ; i++)
			{
				autoFreedRefs++;
				unk->Release();
			}
		}
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
	if ( m_check_auto_cleanup ) autoCleanup();
	wchar_t* url = getUrlText();
	Navigate(url);
	//Save MRU_Url
	if ( wcscmp(url,L"about:blank") ) 
		m_reg.WriteString(L"MRU_Url",url);
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
		GetDlgItem(IDC_SIEVE)->EnableWindow(TRUE);
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
		GetDlgItem(IDC_SIEVE)->EnableWindow(FALSE);
		GetDlgItem(IDC_AUTOREFRESH)->EnableWindow(FALSE);
		if ( m_timerMemoryMonitor )	SetTimer(TIMER_MONITOR_MEMORY, m_timerMemoryMonitor, NULL);
		m_radioSlow.SetCheck(0);
		m_radioFast.SetCheck(0);
		m_radioPaused.SetCheck(0);
		if ( m_timerMemoryMonitor == TIMER_MONITOR_MEMORY_FAST ) m_radioFast.SetCheck(1);
		if ( m_timerMemoryMonitor == TIMER_MONITOR_MEMORY_SLOW ) m_radioSlow.SetCheck(1);
		if ( m_timerMemoryMonitor == TIMER_MONITOR_MEMORY_PAUSED ) m_radioPaused.SetCheck(1);		
		go();
	}
}

void CMainBrowserDlg::OnBnClickedAboutBlank() {
	// When the leak test button is pressed, navigate to the blank document
	//   so that the browser will release all of its elements.  Set
	//   m_waitingForBlankDoc to true so that the DocumentComplete event
	//   handler will know to check for leaks when the blank document
	//   finishes loading.
	//
	requestClosePopups();
	Navigate(L"about:blank");
	m_waitingForBlankDoc = true;
	GetDlgItem(IDC_ABOUTBLANK)->EnableWindow(FALSE);
}

void CMainBrowserDlg::OnBnClickedAutoRefresh() {
	if (!m_autoRefreshMode) {
		// (The button says 'auto refresh')
		//   Start automatically refreshing ("blowing memory"), change the button to 'stop', and disable
		//   the go and leak test buttons.
		//
		m_autoRefreshMode = true;
		initializeMemorySamples(this);
		KillTimer(TIMER_MONITOR_MEMORY);	// Not during sampling
		GetDlgItem(IDC_AUTOREFRESH)->SendMessage(WM_SETTEXT, 0, (LPARAM)L"Stop");
		GetDlgItem(IDC_GO)->EnableWindow(FALSE);
		GetDlgItem(IDC_SIEVE)->EnableWindow(FALSE);
		m_radioFast.EnableWindow(FALSE);
		m_radioSlow.EnableWindow(FALSE);
		m_radioPaused.EnableWindow(FALSE);
		m_radioSlow.SetCheck(0);
		m_radioFast.SetCheck(1);
		m_radioPaused.SetCheck(0);

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
		GetDlgItem(IDC_SIEVE)->EnableWindow(TRUE);
		m_radioFast.EnableWindow(TRUE);
		m_radioSlow.EnableWindow(TRUE);
		m_radioPaused.EnableWindow(TRUE);
		m_radioSlow.SetCheck(0);
		m_radioFast.SetCheck(0);
		m_radioPaused.SetCheck(1);
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

void CMainBrowserDlg::BeginWaitCursor()
{
	CDialog::BeginWaitCursor();
}

void CMainBrowserDlg::EndWaitCursor()
{
	CDialog::EndWaitCursor();
}

void CMainBrowserDlg::onTitleChange(LPCTSTR lpszText) {
	CStringW title(lpszText);
	if (title.IsEmpty())
		SetWindowText(CStringW(L"sIEve"));
	else
		SetWindowText(CStringW(L"sIEve - ") + CStringW(lpszText));
}

void CMainBrowserDlg::onOuterDocumentLoad(MSHTML::IHTMLDocument2Ptr doc)
{
	CBrowserHostDlg::onOuterDocumentLoad(doc);
	m_loadedDoc = doc;

	// If we're in 'auto-refresh' mode, display the current memory usage
	//   and refresh the document.
	//
	if (m_autoRefreshMode) {
		// Make sure that the browser's GC pass is done so that we don't report
		//   as-yet-uncollected memory.
		//
		doc->parentWindow->execScript(L"window.CollectGarbage()", L"javascript");

		//JSR
		//IMalloc *malloc;
		//CoGetMalloc(1,&malloc);
		//malloc->HeapMinimize();

		this->updateStatistics();
		// Reload the document in 500 ms.
		SetTimer(TIMER_AUTO_REFRESH, 500, NULL);
	}

	// If we're waiting for a blank document (that is, we're waiting for the
	//   current document to fully unload so that we can check for leaks),
	//   then check for leaks and display the leak dialog.
	//
	else if (m_waitingForBlankDoc) {
		m_waitingForBlankDoc = false;
		SetTimer(TIMER_ABOUTBLANK, 100, NULL);
		m_checkLeakDoc = doc;
	}
	// If we're simply waiting on the page to load, re-enable the sIEve, leak
	//   test, and auto-refresh buttons, and set the 'stop' button back to 'go'.
	//
	else {
		GetDlgItem(IDC_SIEVE)->EnableWindow(TRUE);
		GetDlgItem(IDC_AUTOREFRESH)->EnableWindow(TRUE);
		GetDlgItem(IDC_GO)->SendMessage(WM_SETTEXT, 0, (LPARAM)L"Go");
		GetDlgItem(IDC_ABOUTBLANK)->EnableWindow(TRUE);
		m_waitingForDoc = NULL;
	}
}

void CMainBrowserDlg::onNewWindow(CBrowserHostDlg** ppDlg)
{
	AfxMessageBox(L"New Window");
	CBrowserPopupDlg *dlg = new CBrowserPopupDlg(getHook(),&m_popups,this);
	dlg->Create(CBrowserPopupDlg::IDD);
	dlg->ShowWindow(SW_SHOW);
	m_popups.push_back(dlg);

	*ppDlg = dlg;
}

void CMainBrowserDlg::OnBnClickedShowInUse()
{
	if ( ! m_leakDlg ) 
	{
		m_leakDlg = new CLeakDlg(this);
		m_leakDlg->Create(IDD_LEAKS);
	}
	m_leakDlg->clearLeaks();
	getHook()->rescanForElements(NULL);
	getHook()->showLeaks(m_loadedDoc->parentWindow, m_leakDlg,false);
	m_mallocspy->showLeaks(m_leakDlg);
	m_leakDlg->populateLeaks();
	m_leakDlg->ShowWindow(SW_SHOW);
}

void CMainBrowserDlg::OnBnClickedShowLeaks()
{
	if ( ! m_leakDlg ) 
	{
		m_leakDlg = new CLeakDlg(this);
		m_leakDlg->Create(IDD_LEAKS);
	}
	m_leakDlg->clearLeaks();
	getHook()->rescanForElements(NULL);
	getHook()->showLeaks(m_loadedDoc->parentWindow, m_leakDlg,true);
	m_mallocspy->showLeaks(m_leakDlg);
	m_leakDlg->m_check_show_all = true;
	m_leakDlg->populateLeaks();
	m_leakDlg->m_check_show_all = false;
	m_leakDlg->ShowWindow(SW_SHOW);
}

void CMainBrowserDlg::OnBnClickedClearInUse()
{
	if ( m_leakDlg )
	{
		m_leakDlg->clearLeaks();
		m_leakDlg->populateLeaks();
	}


	getHook()->clearElements();
	m_mallocspy->Clear();
	if ( m_leakDlg && m_leakDlg->IsWindowVisible() )
	{
		OnBnClickedShowInUse();
	}
}

void CMainBrowserDlg::OnNMCustomdrawMemsamples(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );

    // Take the default processing unless we set this to something else below.
    *pResult = CDRF_DODEFAULT;

    // First thing - check the draw stage. If it's the control's prepaint
    // stage, then tell Windows we want messages for every item.

    if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
    {
		*pResult = CDRF_NOTIFYITEMDRAW;
    }
    else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
        // This is the prepaint stage for an item. Here's where we set the
        // item's text color. Our return value will tell Windows to draw the
        // item itself, but it will use the new color we set here.
        // We'll cycle the colors through red, green, and light blue.
		
		LONG incr = (LONG) m_memsamples.GetItemData((int)(pLVCD->nmcd.dwItemSpec));
		if (incr > 0  ) // The reported boolean
		{
			// Make the item red when the memory item is reported for the first time
 	        pLVCD->clrText = RGB(255,0,0);
		}
		if ( incr < 0 )
		{
			// Make the item green when the memory item is already freed;
			pLVCD->clrText = RGB(0,175,0);
		}
		
        // Tell Windows to paint the control itself.
        *pResult = CDRF_DODEFAULT;
    }

}

void CMainBrowserDlg::OnBnClickedCheckCycleDetection()
{
	m_check_cycle_detection = ! m_check_cycle_detection;
}

void CMainBrowserDlg::OnBnClickedCheckAutoCleanup()
{
	m_check_auto_cleanup = ! m_check_auto_cleanup;
	if ( m_check_auto_cleanup ) autoCleanup();
}

void CMainBrowserDlg::OnBnClickedLogDefect()
{
	logDefect();
}

void CMainBrowserDlg::OnBnClickedShowHelp()
{
	showHelp();
}

void CMainBrowserDlg::OnBnClickedRadioSlow()
{
	m_timerMemoryMonitor = TIMER_MONITOR_MEMORY_SLOW;
	KillTimer(TIMER_MONITOR_MEMORY);
	SetTimer(TIMER_MONITOR_MEMORY, m_timerMemoryMonitor, NULL);
}

void CMainBrowserDlg::OnBnClickedRadioFast()
{
	m_timerMemoryMonitor = TIMER_MONITOR_MEMORY_FAST;
	KillTimer(TIMER_MONITOR_MEMORY);
	SetTimer(TIMER_MONITOR_MEMORY, m_timerMemoryMonitor, NULL);
}

void CMainBrowserDlg::OnBnClickedRadioPaused()
{
	m_timerMemoryMonitor = TIMER_MONITOR_MEMORY_PAUSED;
	KillTimer(TIMER_MONITOR_MEMORY);
}

void CMainBrowserDlg::OnBnClickedCrossrefScan()
{
	BeginWaitCursor();
	getHook()->crossRefScan(NULL, (CButton *)GetDlgItem(IDC_CROSSREF_SCAN));
	EndWaitCursor();
}