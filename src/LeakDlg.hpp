#pragma once

#include "resource.h"
#include "DlgResizeHelper.h"

// This structure is used to keep track of leaked nodes, the URL of their
//  documents, and the number of references left outstanding.
//
struct LeakEntry {
	LeakEntry(IUnknown* node, BSTR url, int refCount) {
		this->node = node;
		this->url = url;
		this->refCount = refCount;
	}

	IUnknown*	node;
	BSTR		url;
	int			refCount;
};

// The dialog box for displaying all leaked nodes.
//
class CLeakDlg : public CDialog {
private:
	enum { IDD = IDD_LEAKS };
	CListCtrl				m_leakList;
	std::vector<LeakEntry>	m_leaks;
	DlgResizeHelper			m_resizeHelper;

	void clearLeaks();
	void populateLeaks();
	void updateButtons();
	void showItemProperties(UINT nItem);

public:
	CLeakDlg(CWnd* pParent = NULL);
	void addNode(IUnknown* node, BSTR url, int refCount);

	DECLARE_MESSAGE_MAP()

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg NCHITTEST_RESULT OnNcHitTest(CPoint point);
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnOk();
	afx_msg void OnLvnItemActivateLeaklist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnViewProperties();
	afx_msg void OnLvnItemChangedLeaklist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void CopySelectedItems();
	afx_msg void OnLvnKeydownLeaklist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
};
