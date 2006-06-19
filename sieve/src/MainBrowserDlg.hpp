#pragma once

#include "resource.h"
#include "DlgResizeHelper.h"
#include "BrowserHostDlg.hpp"
#include "JSHook.hpp"
#include "LeakDlg.hpp"
#include "Cmallspy.h"
#include "VORegistry.h"
#include "Graph.h"
#include "afxwin.h"

#ifdef NEVER // Separate browser POC-code
#include <exdisp.h> //For IWebBrowser2* and others
#include <exdispid.h>
#endif

// Forward declaration
class CBrowserPopupDlg;
class JSHook;

// The main sIEve dialog box, containing the browser control.
//
class CMainBrowserDlg : public CBrowserHostDlg {
#ifdef NEVER // Separate browser POC-code
	DECLARE_DYNCREATE(CMainBrowserDlg)
#endif

private:
	wchar_t* getUrlText();
	void go();

	std::vector<CBrowserPopupDlg*>	m_popups;

	bool						m_waitingForDoc;
	bool						m_waitingForBlankDoc;
	bool						m_autoRefreshMode;
	MSHTML::IHTMLDocument2Ptr	m_checkLeakDoc;
	MSHTML::IHTMLDocument2Ptr	m_loadedDoc;

	HICON						m_hIcon;
	CVORegistry					m_reg;
	CLeakDlg*					m_leakDlg;
	CBrush						m_brush;

	DlgResizeHelper				m_resizeHelper;
	virtual bool isHookActive() { return !m_waitingForBlankDoc && !m_autoRefreshMode; }

public:
	enum { IDD = IDD_BROWSER_DIALOG };
	CMallocSpy *m_mallocspy;
	CListCtrl m_memsamples;
	CButton	m_radioFast;
	CButton	m_radioSlow;
	CButton m_radioPaused;
	int m_timerMemoryMonitor;
	bool m_check_auto_cleanup;
	CGraphCtrl m_memGraph;

	CMainBrowserDlg(CComObject<JSHook>* hook, CWnd* pParent = NULL);
	virtual ~CMainBrowserDlg();
	afx_msg void OnBnClickedShowInUse();
	afx_msg void OnBnClickedClearInUse();
	//void rescans();

	DECLARE_MESSAGE_MAP()
#ifdef NEVER // Separate browser POC-code
	DECLARE_DISPATCH_MAP()
#endif

	LRESULT WindowProc(UINT message, WPARAM wparam, LPARAM lparam);
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HBRUSH OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void doNothing();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM lCount);
	afx_msg void CMainBrowserDlg::autoCleanup();

	void updateStatistics();
	void requestClosePopups();
	void destroyFinishedPopups();

public:
	virtual void onTitleChange(LPCTSTR lpszText);
	virtual void onOuterDocumentLoad(MSHTML::IHTMLDocument2Ptr doc);
	virtual void onNewWindow(CBrowserHostDlg** ppDlg);
	afx_msg void OnStnClickedMemlabel();
	afx_msg void OnNMCustomdrawMemsamples(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedGo();
	afx_msg void OnBnClickedBack();
	afx_msg void OnBnClickedForward();
	afx_msg void OnBnClickedAboutBlank();
	afx_msg void OnBnClickedAutoRefresh();
	afx_msg void OnBnClickedShowLeaks();
	afx_msg void OnBnClickedLogDefect();
	afx_msg void OnBnClickedShowHelp();
	afx_msg void OnBnClickedCheckAutoCleanup();


#ifdef NEVER // Separate browser POC-code
	BOOL UnAdviseSink();

	// Fires before a navigation occurs in the given object 
	afx_msg void BeforeNavigate2(LPDISPATCH pDisp, VARIANT FAR *url, VARIANT FAR *Flags, VARIANT FAR *TargetFrameName, VARIANT FAR *PostData, VARIANT FAR *Headers, VARIANT_BOOL* Cancel);
	// Fires when the document that is being navigated to reaches the READYSTATE_COMPLETE state
	afx_msg void DocumentComplete(IDispatch *pDisp,VARIANT *URL);
	//
	afx_msg void NavigateComplete2(LPDISPATCH pDisp, VARIANT* URL);
	// Fires when a navigation operation is beginning.
	afx_msg void DownloadBegin();
	// Fires when a navigation operation finishes, is halted, or fails.
	afx_msg void DownloadEnd();
	// browser is quiting so kill events
	afx_msg void OnQuit();
#endif
public:
	afx_msg void OnBnClickedRadioSlow();
public:
	afx_msg void OnBnClickedRadioFast();
public:
	afx_msg void OnBnClickedRadioPaused();
};
