#pragma once

#include "resource.h"
#include "DlgResizeHelper.h"
#include "BrowserHostDlg.hpp"

// Forward declaration
class CBrowserPopupDlg;
class JSHook;

// The main drip dialog box, containing the browser control.
//
class CMainBrowserDlg : public CBrowserHostDlg {
private:
	wchar_t* getUrlText();
	void go();

	CComObject<JSHook>*			m_hook;

	std::vector<CBrowserPopupDlg*>	m_popups;

	bool						m_waitingForDoc;
	bool						m_waitingForBlankDoc;
	bool						m_autoRefreshMode;
	MSHTML::IHTMLDocument2Ptr	m_checkLeakDoc;

	HICON						m_hIcon;

	DlgResizeHelper				m_resizeHelper;

	virtual CComObject<JSHook>* getHook() { return m_hook; }
	virtual bool isHookActive() { return m_waitingForDoc && !m_autoRefreshMode; }

public:
	enum { IDD = IDD_BROWSER_DIALOG };

	CMainBrowserDlg(CWnd* pParent = NULL);
	virtual ~CMainBrowserDlg();

	DECLARE_MESSAGE_MAP()

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
	afx_msg void OnBnClickedGo();
	afx_msg void OnBnClickedBack();
	afx_msg void OnBnClickedForward();
	afx_msg void OnBnClickedCheckLeaks();
	afx_msg void OnBnClickedAutoRefresh();

	void requestClosePopups();
	void destroyFinishedPopups();

	virtual void onTitleChange(LPCTSTR lpszText);
	virtual void onOuterDocumentLoad(MSHTML::IHTMLDocument2Ptr doc);
	virtual void onNewWindow(CBrowserHostDlg** ppDlg);
};
