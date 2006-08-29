#pragma once

#include "resource.h"
#include "JSHook.hpp"

// The dialog box for displaying all leaked elements.
//
class CLeakDlg : public CDialog {
	DECLARE_EASYSIZE
private:
	enum { IDD = IDD_LEAKS };
	CPoint					m_point;
	int						m_lastSortedColumn;
	//CToolTipCtrl*			m_toolTip;
	CBrush m_brush;
	CComObject<JSHook>*		m_hook;
	void showItemProperties(UINT nItem);

public:
	CLeakDlg(CComObject<JSHook>* hook, CWnd* pParent = NULL);
	~CLeakDlg();
	CComObject<JSHook>* getHook() { return m_hook; }
	void addElement(Elem* hookElem);
	void notifyElement(Elem* hookElem);
	void sortOnDefaultColumn();
	void updateButtons();
	void clearLeaks();
	void clearInUse();
	void showInUse();
	void prepare(LPCTSTR title);
	void finish();
	CListCtrl				m_leakList;

	DECLARE_MESSAGE_MAP()

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg UINT OnNcHitTest(CPoint point);
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnOk();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLvnItemActivateLeaklist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnViewProperties();
	afx_msg void OnLvnItemChangedLeaklist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void CopySelectedItems();
	afx_msg void OnLvnKeydownLeaklist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCustomdrawLeakList (NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	//ULONG m_edit_bytes;
	//ULONG m_edit_kbytes;
	ULONG m_edit_items;
	ULONG m_edit_hidden_items;
	afx_msg void OnHdnItemclickLeaklist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL CLeakDlg::PreTranslateMessage(MSG* pMsg); 
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void CLeakDlg::OnWmNcHitTest(NMHDR *pNMHDR, LRESULT *pResult);
//	afx_msg void OnNMHoverLeaklist(NMHDR *pNMHDR, LRESULT *pResult);
//	afx_msg void OnNMClickLeaklist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedCheckShowAll();
	afx_msg void OnStnClickedStaticBlack();
	afx_msg void OnBnClickedRefreshLeaks();
};
