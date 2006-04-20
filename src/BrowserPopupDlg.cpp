// BrowserPopupDlg.cpp : implementation file
//

#include "stdafx.h"
#include "BrowserPopupDlg.hpp"
#include "JSHook.hpp"



// CBrowserPopupDlg dialog

IMPLEMENT_DYNAMIC(CBrowserPopupDlg, CBrowserHostDlg)
CBrowserPopupDlg::CBrowserPopupDlg(CComObject<JSHook>* hook, std::vector<CBrowserPopupDlg*> *popups, CWnd* pParent /*=NULL*/)
	: CBrowserHostDlg(IDC_EXPLORER, CBrowserPopupDlg::IDD, pParent),
	m_hook(hook), m_popups(popups), m_waitingForBlankDoc(false), m_isFinished(false) {
}

CBrowserPopupDlg::~CBrowserPopupDlg() {
}

void CBrowserPopupDlg::DoDataExchange(CDataExchange* pDX) {
	CBrowserHostDlg::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CBrowserPopupDlg, CBrowserHostDlg)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CBrowserPopupDlg message handlers

BOOL CBrowserPopupDlg::OnInitDialog() {
	CBrowserHostDlg::OnInitDialog();

	repositionExplorer();
	return TRUE;
}

void CBrowserPopupDlg::OnSize(UINT nType, int cx, int cy) {
	CBrowserHostDlg::OnSize(nType, cx, cy);
	repositionExplorer();
}

void CBrowserPopupDlg::repositionExplorer() {
	CRect rect;
	GetClientRect(&rect);
	::MoveWindow(getExplorerHwnd(), 0, 0, rect.Width(), rect.Height(), true);
}

bool CBrowserPopupDlg::isFinished() {
	return m_isFinished;
}

void CBrowserPopupDlg::requestClose() {
	if (m_waitingForBlankDoc) return;
	Navigate(L"about:blank");
	ShowWindow(SW_HIDE);
	m_waitingForBlankDoc = true;
}

CComObject<JSHook>* CBrowserPopupDlg::getHook() {
	return m_hook;
}

bool CBrowserPopupDlg::isHookActive() {
	return !m_waitingForBlankDoc;
}

void CBrowserPopupDlg::onTitleChange(LPCTSTR lpszText) {
	CStringW title(lpszText);
	if (title.IsEmpty())
		SetWindowText(CStringW(L"Drip"));
	else
		SetWindowText(CStringW(L"Drip - ") + CStringW(lpszText));
}

void CBrowserPopupDlg::onWindowSetHeight(long height) {
	CRect clientRect, windowRect;
	GetClientRect(&clientRect);
	GetWindowRect(&windowRect);

	windowRect.bottom += height - clientRect.Height();
	MoveWindow(&windowRect);
}

void CBrowserPopupDlg::onWindowSetWidth(long width) {
	CRect clientRect, windowRect;
	GetClientRect(&clientRect);
	GetWindowRect(&windowRect);

	windowRect.right += width - clientRect.Width();
	MoveWindow(&windowRect);
}

void CBrowserPopupDlg::onOuterDocumentLoad(MSHTML::IHTMLDocument2Ptr doc) {
	if (m_waitingForBlankDoc)
		m_isFinished = true;

	// For some reason, the control does not seem to be resizeable until after the page has loaded
	//
	repositionExplorer();
}

void CBrowserPopupDlg::onNewWindow(CBrowserHostDlg** ppDlg) {
	CBrowserPopupDlg *dlg = new CBrowserPopupDlg(m_hook,m_popups,this);
	dlg->Create(CBrowserPopupDlg::IDD);
	dlg->ShowWindow(SW_SHOW);
	m_popups->push_back(dlg);

	*ppDlg = dlg;
}

void CBrowserPopupDlg::onClosing() {
	requestClose();
}
