#pragma once

#include "resource.h"
#include "DlgResizeHelper.h"

// This structure is used to keep track of leaked nodes, the URL of their
//  documents, and the number of references left outstanding.
//
struct LeakEntry {
	LeakEntry(IUnknown* node, BSTR url, int refCount, bool isRecent) {
		this->node = node;
		this->url = url;
		this->refCount = refCount;
		this->isRecent = isRecent;
	}

	IUnknown*	node;
	BSTR			url;
	int			refCount;
	bool			isRecent;
};

// The dialog box for displaying all leaked nodes.
//
class CDOMReportDlg : public CDialog {
private:
	enum { IDD = IDD_LEAKS };
	CListCtrl				m_leakList;
	CButton					m_showAllRadio;
	CButton					m_showRecentRadio;

	bool						m_isShowingRecentOnly;
	int						m_sortByColumn;
	bool						m_reverseSort;

	std::vector<LeakEntry>	m_leaks;
	DlgResizeHelper			m_resizeHelper;
	CStringW						m_domReportType;

	void replaceRptTypeInTitle(CWnd* wnd);
	void clearLeaks();
	static CStringW getIsNodeAttached(IUnknown* unk);
	static CStringW getColumnText(const LeakEntry& entry, int column);
	void populateLeaks(bool showRecentOnly);
	void updateButtons();
	void showItemProperties(UINT nItem);

public:
	CDOMReportDlg(CStringW domReportType, CWnd* pParent = NULL);
	void addNode(IUnknown* node, BSTR url, int refCount, bool isRecent);

	// for sorting
	int compareEntries(size_t left_idx, size_t right_idx);
	static int compareColumns(const LeakEntry& left, const LeakEntry& right, int column);

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
	afx_msg void OnBnClickedAllRadio();
	afx_msg void OnBnClickedRecentRadio();
	afx_msg void OnHdnItemClickLeaklist(NMHDR *pNMHDR, LRESULT *pResult);
};
