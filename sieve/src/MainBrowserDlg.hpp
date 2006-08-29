#pragma once

#include "resource.h"
#include "BrowserHostDlg.hpp"
#include "JSHook.hpp"
#include "LeakDlg.hpp"
#include "Cmallspy.h"
#include "VORegistry.h"
#include "MemoryGraphCtrl.h"
#include "afxwin.h"

// Forward declaration
class CBrowserPopupDlg;
class JSHook;

// The main sIEve dialog box, containing the browser control.
//
class CMainBrowserDlg : public CBrowserHostDlg {
	DECLARE_EASYSIZE

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
	virtual bool isHookActive() { return !m_waitingForBlankDoc /*&& !m_autoRefreshMode*/;}

public:
	enum { IDD = IDD_BROWSER_DIALOG };
	CMallocSpy *m_mallocspy;
	CListCtrl m_memsamples;
	CButton	m_radioMemoryUsage;
	CButton	m_radioDOMUsage;
	CButton	m_radioHigh;
	CButton	m_radioNormal;
	CButton	m_radioLow;
	CButton m_radioPaused;
	bool m_check_cycle_detection;
	CMemoryGraphCtrl m_memGraph;

	CMainBrowserDlg(CComObject<JSHook>* hook, CWnd* pParent = NULL);
	virtual ~CMainBrowserDlg();
	static size_t GetMemoryUsage();

	void BeginWaitCursor();
	void EndWaitCursor();
	int updateStatistics();
	afx_msg void OnBnClickedShowInUse();
	afx_msg void OnBnClickedClearInUse();

	DECLARE_MESSAGE_MAP()
#ifdef NEVER // Separate browser POC-code
	DECLARE_DISPATCH_MAP()
#endif

	LRESULT WindowProc(UINT message, WPARAM wparam, LPARAM lparam);
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void doNothing();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM lCount);
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
	afx_msg void OnBnClickedCheckCycleDetection();
	afx_msg void OnBnClickedRadioMemoryUsage();
	afx_msg void OnBnClickedRadioDOMUsage();
	afx_msg void OnBnClickedRadioHigh();
	afx_msg void OnBnClickedRadioNormal();
	afx_msg void OnBnClickedRadioLow();
	afx_msg void OnBnClickedRadioPaused();
	afx_msg void OnBnClickedCrossrefScan();
};
