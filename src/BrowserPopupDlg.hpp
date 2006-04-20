#pragma once

#include "resource.h"
#include "BrowserHostDlg.hpp"

// forward declaration
class JSHook;

// CBrowserPopupDlg dialog

class CBrowserPopupDlg : public CBrowserHostDlg
{
private:
	CComObject<JSHook>*			m_hook;
	void repositionExplorer();

	DECLARE_DYNAMIC(CBrowserPopupDlg)

public:
	CBrowserPopupDlg(CComObject<JSHook>* hook, std::vector<CBrowserPopupDlg*> *popups, CWnd* pParent = NULL);   // standard constructor
	virtual ~CBrowserPopupDlg();

	void createPopup(LPDISPATCH* ppDisp);

	bool isFinished();
	void requestClose();

// Dialog Data
	enum { IDD = IDD_BROWSER_POPUP_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	virtual CComObject<JSHook>* getHook();
	virtual bool isHookActive();

	virtual void onTitleChange(LPCTSTR lpszText);
	virtual void onWindowSetHeight(long height);
	virtual void onWindowSetWidth(long width);
	virtual void onOuterDocumentLoad(MSHTML::IHTMLDocument2Ptr doc);
	virtual void onNewWindow(CBrowserHostDlg** ppDlg);
	virtual void onClosing();

	std::vector<CBrowserPopupDlg*> *m_popups;
	bool m_waitingForBlankDoc, m_isFinished;
};
