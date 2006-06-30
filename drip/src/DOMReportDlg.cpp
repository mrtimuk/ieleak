#include "stdafx.h"
#include "resource.h"
#include "DOMReportDlg.hpp"
#include "PropDlg.hpp"
#include <afxtempl.h>

#define COL_URL			0
#define COL_REFS			1
#define COL_NODETYPE		2
#define COL_ATTACHED		3
#define COL_ID				4
#define COL_OUTERHTML	5
#define NUM_COLS			6

int CALLBACK DOMReportSortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CDOMReportDlg* dlg = (CDOMReportDlg*)lParamSort;
	return dlg->compareEntries(lParam1, lParam2);
}

CDOMReportDlg::CDOMReportDlg(CStringW domReportType, CWnd* pParent) : CDialog(CDOMReportDlg::IDD, pParent) {
	//{{AFX_DATA_INIT(CDOMReportDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_isShowingRecentOnly = false;
	m_domReportType = domReportType;

	m_sortByColumn = COL_URL;
	m_reverseSort = false;
}

void CDOMReportDlg::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CDOMReportDlg)
	DDX_Control(pDX, IDC_LEAKLIST, m_leakList);
	DDX_Control(pDX, IDC_SHOW_ALL_RADIO, m_showAllRadio);
	DDX_Control(pDX, IDC_SHOW_RECENT_RADIO, m_showRecentRadio);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDOMReportDlg, CDialog)
	//{{AFX_MSG_MAP(CDOMReportDlg)
	ON_WM_PAINT()
	ON_WM_NCHITTEST()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDOK, OnOk)
	ON_BN_CLICKED(IDC_PROPERTIES_BUTTON, OnViewProperties)
	//}}AFX_MSG_MAP
	ON_NOTIFY(LVN_ITEMACTIVATE, IDC_LEAKLIST, OnLvnItemActivateLeaklist)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LEAKLIST, OnLvnItemChangedLeaklist)
	ON_BN_CLICKED(IDC_COPY, CopySelectedItems)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LEAKLIST, OnLvnKeydownLeaklist)
	ON_NOTIFY(HDN_ITEMCLICK, 0, OnHdnItemClickLeaklist)
	ON_BN_CLICKED(IDC_SHOW_ALL_RADIO, OnBnClickedAllRadio)
	ON_BN_CLICKED(IDC_SHOW_RECENT_RADIO, OnBnClickedRecentRadio)
END_MESSAGE_MAP()

BOOL CDOMReportDlg::OnInitDialog() {
	CDialog::OnInitDialog();

	// Show the report type in the title and radio buttons
	//
	replaceRptTypeInTitle(this);
	replaceRptTypeInTitle(&m_showAllRadio);
	replaceRptTypeInTitle(&m_showRecentRadio);

	// Full-row selection is more intuitive
	//
	m_leakList.SetExtendedStyle(m_leakList.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

	// Set up the columns for the leak list.
	//
	m_leakList.InsertColumn(COL_URL, L"URL", LVCFMT_LEFT, 128);
	m_leakList.InsertColumn(COL_REFS, L"Refs", LVCFMT_LEFT, 40);
	m_leakList.InsertColumn(COL_NODETYPE, L"Node Type", LVCFMT_LEFT, 72);
	m_leakList.InsertColumn(COL_ATTACHED, L"Attached?", LVCFMT_LEFT, 68);
	m_leakList.InsertColumn(COL_ID, L"ID", LVCFMT_LEFT, 128);
	m_leakList.InsertColumn(COL_OUTERHTML, L"Outer HTML", LVCFMT_LEFT, 256);

	// Populate the leak list control from the list of LeakEntry structures.
	//
	m_showAllRadio.SetCheck(0);
	m_showRecentRadio.SetCheck(1);
	populateLeaks(true);

	// Enable/disable button
	//
	updateButtons();

	// Set up resizing
	//
	m_resizeHelper.Init(m_hWnd);
	m_resizeHelper.Fix(IDC_SHOW_ALL_RADIO, DlgResizeHelper::kWidthLeft, DlgResizeHelper::kHeightTop);
	m_resizeHelper.Fix(IDC_SHOW_RECENT_RADIO, DlgResizeHelper::kWidthLeft, DlgResizeHelper::kHeightTop);
	m_resizeHelper.Fix(IDC_LEAKLIST, DlgResizeHelper::kLeftRight, DlgResizeHelper::kTopBottom);
	m_resizeHelper.Fix(IDC_PROPERTIES_BUTTON, DlgResizeHelper::kWidthRight, DlgResizeHelper::kHeightTop);
	m_resizeHelper.Fix(IDC_COPY, DlgResizeHelper::kWidthRight, DlgResizeHelper::kHeightTop);
	m_resizeHelper.Fix(IDOK, DlgResizeHelper::kWidthRight, DlgResizeHelper::kHeightTop);

	return TRUE;
}

void CDOMReportDlg::replaceRptTypeInTitle(CWnd* wnd) {
	CStringW text;
	wnd->GetWindowText(text);
	VERIFY(text.Replace(L"_____", m_domReportType) == 1);
	ASSERT(text.Find(L"_") == -1);
	wnd->SetWindowText(text);
}

void CDOMReportDlg::OnPaint() {
	CDialog::OnPaint();
	m_resizeHelper.OnGripperPaint();
}

NCHITTEST_RESULT CDOMReportDlg::OnNcHitTest(CPoint point)
{
	NCHITTEST_RESULT ht = CDialog::OnNcHitTest(point);
	m_resizeHelper.OnGripperNcHitTest(point, ht);
	return ht;
}

void CDOMReportDlg::OnClose() {
	EndDialog(0);
}

void CDOMReportDlg::OnOk() {
	EndDialog(0);
}

void CDOMReportDlg::OnDestroy() {
	// Free the leak list when the dialog is destroyed.
	//
	clearLeaks();
}

// Adds an node to the leak list, including its document URL and ref count.
//
void CDOMReportDlg::addNode(IUnknown* node, BSTR url, int refCount, bool isRecent) {
	// Add a reference to the node, and allocate a copy of the URL string.
	//
	node->AddRef();
	m_leaks.push_back(LeakEntry(node, SysAllocString(url), refCount, isRecent));
}

// Clear all leaks.
//
void CDOMReportDlg::clearLeaks() {
	for (std::vector<LeakEntry>::const_iterator it = m_leaks.begin(); it != m_leaks.end(); ++it) {
		// Release the leaked node and free its associated document URL.
		//   (of course, since this node is over-referenced, it won't actually get freed
		//   properly, but we're doing our part, at least!)
		//
		it->node->Release();
		SysFreeString(it->url);
	}
	m_leaks.clear();
}

CStringW CDOMReportDlg::getIsNodeAttached(IUnknown* unk) {
	// Compare node.ownserDocument (MSDN: "document object that is used to create new nodes")
	// with the node.document (the document or document fragment to which the node is 
	// currently attached) to determine whether the element is attached to the document.
	VARIANT ownerDocument, document;
	CStringW retVal = L"";

	if (!getPropertyValue((CComQIPtr<IDispatchEx>)unk, L"ownerDocument", ownerDocument) ||
		!getPropertyValue((CComQIPtr<IDispatchEx>)unk, L"document", document)) {
		retVal = L"";
	}
	else if (ownerDocument.vt != VT_DISPATCH || document.vt != VT_DISPATCH) {
		retVal = L"";
	}
	else if (ownerDocument.pdispVal == document.pdispVal) {
		retVal = L"Yes";
	}
	else {
		retVal = L"No";
	}
	VariantClear(&ownerDocument);
	VariantClear(&document);
	return retVal;
}

CStringW CDOMReportDlg::getColumnText(const LeakEntry& entry, int column) {
	MSHTML::IHTMLDOMNodePtr node = entry.node;
	MSHTML::IHTMLElementPtr elem = entry.node;

	CStringW tmp;

	switch (column) {
		case COL_URL:
			return entry.url;

		case COL_REFS:
			tmp.Format(L"%d", entry.refCount);
			return tmp;

		case COL_NODETYPE:
			return (LPCTSTR)node->nodeName;

		case COL_ATTACHED:
			return getIsNodeAttached(entry.node);

		case COL_ID:
			if (elem)
				return (LPCTSTR)elem->id;
			return L"";

		case COL_OUTERHTML:
			if (elem)
				return (LPCTSTR)elem->outerHTML;
			return L"";

		default:
			ASSERT(false);
			return L"";
	}
}

// Take all entries in m_leaks and populate the leak list control with them.
//
void CDOMReportDlg::populateLeaks(bool showRecentOnly) {
	// Don't reload if the dialog is already showing the correct items
	//
	if (m_isShowingRecentOnly == showRecentOnly)
		return;
	m_isShowingRecentOnly = showRecentOnly;

	// Reduce flicker
	//
	LockWindowUpdate();

	m_leakList.DeleteAllItems();

	for (size_t data_idx = 0; data_idx < m_leaks.size(); data_idx++) {
		LeakEntry const& entry = m_leaks.at(data_idx);
		if (showRecentOnly && !entry.isRecent)
			continue;

		int display_idx = m_leakList.GetItemCount();
		m_leakList.InsertItem(display_idx, L"");
		m_leakList.SetItemData(display_idx, data_idx);

		for (int column = 0; column < NUM_COLS; column++)
			m_leakList.SetItemText(display_idx, column, getColumnText(entry, column));
	}
	if (m_leakList.GetItemCount() > 0)
		m_leakList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);

	m_leakList.SortItems(DOMReportSortFunc, (DWORD_PTR)this);
	UnlockWindowUpdate();
}

// When a leaked element is selected, display its properties.
//
afx_msg void CDOMReportDlg::OnLvnItemActivateLeaklist(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	showItemProperties(pNMLV->iItem);
}

void CDOMReportDlg::OnLvnItemChangedLeaklist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	
	*pResult = 0;
	updateButtons();
}

void CDOMReportDlg::OnLvnKeydownLeaklist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	if (GetKeyState(VK_CONTROL) < 0 && pLVKeyDown->wVKey == 'C')
		CopySelectedItems();

	*pResult = 0;
}

void CDOMReportDlg::updateButtons()
{
	GetDlgItem(IDC_PROPERTIES_BUTTON)->EnableWindow(m_leakList.GetSelectedCount() == 1);
}

void CDOMReportDlg::showItemProperties(UINT nItem)
{
	// Get the IDispatch interface so that we can dynamically query the
	//   element's properties.
	//
	ASSERT(nItem != -1);

	size_t data_idx = m_leakList.GetItemData(nItem);
	LeakEntry& entry = m_leaks.at(data_idx);
	CComQIPtr<IDispatchEx> disp = entry.node;

	CPropDlg propDlg(CStringW(L"Memory Leak in ") + entry.url, this);
	propDlg.setObject(disp);
	propDlg.DoModal();
}

afx_msg void CDOMReportDlg::OnViewProperties()
{
	if (m_leakList.GetSelectedCount() == 1)
		showItemProperties(m_leakList.GetNextItem(-1, LVNI_SELECTED));
}

bool CopyToClipboard(HWND hOwner, CStringW text)
{
	LPWSTR  lptstrCopy; 
	HGLOBAL hglbCopy; 

	if (!OpenClipboard(hOwner))
		return false;

	// Allocate a global memory object for the text. 
	hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (text.GetLength() + 1) * sizeof(WCHAR)); 
	if (hglbCopy == NULL) 
	{ 
		CloseClipboard(); 
		return false; 
	} 

	// Lock the handle and copy the text to the buffer. 
	lptstrCopy = (LPWSTR)GlobalLock(hglbCopy); 
	memcpy(lptstrCopy, text, text.GetLength() * sizeof(WCHAR)); 
	lptstrCopy[text.GetLength()] = (WCHAR) 0; // null character 
	GlobalUnlock(hglbCopy); 

	// Place the handle on the clipboard. 
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, hglbCopy);
	CloseClipboard();
	return true;
}

void CDOMReportDlg::CopySelectedItems()
{
	CStringW text;

	int iCurrentItem = -1;
	while ((iCurrentItem = m_leakList.GetNextItem(iCurrentItem, LVNI_SELECTED)) != -1) {
		CArray<DISPID> aDispIDs;
		CArray<CStringW> asNames, asValues;
		
		// Load object properties
		size_t data_idx = m_leakList.GetItemData(iCurrentItem);
		LeakEntry& entry = m_leaks.at(data_idx);
		GetObjectProperties((CComQIPtr<IDispatchEx>)entry.node, aDispIDs, asNames, asValues);

		// Serialize object
		CStringW header;
		header.Format(_T("%s\t(%i reference%s)\r\n"), entry.url, entry.refCount, entry.refCount != 1 ? "s" : "");
		text += header;
		for (int iPropCnt = 0; iPropCnt < aDispIDs.GetSize(); iPropCnt++)
			text += "\t" + asNames[iPropCnt] + "\t" + asValues[iPropCnt] + "\r\n";
		text += "\r\n\r\n";
	}

	if (!CopyToClipboard(m_hWnd, text)) {
		AfxMessageBox(_T("The information could not be copied to the clipboard."));
	}
}

void CDOMReportDlg::OnSize(UINT nType, int cx, int cy) {
	m_resizeHelper.OnSize();
}

void CDOMReportDlg::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	// Using arbitrary figures here (dialog units would be better)
	//
	lpMMI->ptMinTrackSize.x = 400;
	lpMMI->ptMinTrackSize.y = 250;

	CWnd::OnGetMinMaxInfo(lpMMI);
}

void CDOMReportDlg::OnBnClickedAllRadio()
{
	populateLeaks(false);
}

void CDOMReportDlg::OnBnClickedRecentRadio()
{
	populateLeaks(true);
}

int CDOMReportDlg::compareEntries(size_t left_idx, size_t right_idx) {
	const LeakEntry& left = m_leaks.at(left_idx);
	const LeakEntry& right = m_leaks.at(right_idx);

	// primary sort by user-selected column
	int result = compareColumns(left, right, m_sortByColumn);

	// secondary sort by URL
	if (result == 0 && m_sortByColumn != COL_URL)
		result = compareColumns(left, right, COL_URL);

	if (m_reverseSort)
		result *= -1;
	return result;
}

int CDOMReportDlg::compareColumns(const LeakEntry& left, const LeakEntry& right, int column) {
	if (column == COL_REFS) {
		// must compare refCount numerically
		if (left.refCount < right.refCount)
			return -1;
		else if (left.refCount == right.refCount)
			return 0;
		else
			return 1;
	}

	// compare text only
	CStringW leftText = getColumnText(left, column);
	CStringW rightText = getColumnText(right, column);

	int result = leftText.Compare(rightText);

	// reverse sort blank items
	if (leftText == "" || rightText == "")
		result *= -1;

	return result;
}

void CDOMReportDlg::OnHdnItemClickLeaklist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);

	if (phdr->iItem == m_sortByColumn) 
		m_reverseSort = !m_reverseSort;
	else
		m_reverseSort = false;
	m_sortByColumn = phdr->iItem;

	m_leakList.SortItems(DOMReportSortFunc, (DWORD_PTR)this);

	*pResult = 0;
}
