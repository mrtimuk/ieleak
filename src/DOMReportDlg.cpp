#include "stdafx.h"
#include "resource.h"
#include "DOMReportDlg.hpp"
#include "PropDlg.hpp"
#include <afxtempl.h>

CDOMReportDlg::CDOMReportDlg(CStringW domReportType, CWnd* pParent) : CDialog(CDOMReportDlg::IDD, pParent) {
	//{{AFX_DATA_INIT(CDOMReportDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_isShowingRecentOnly = false;
	m_domReportType = domReportType;
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
	m_leakList.InsertColumn(0, L"URL", LVCFMT_LEFT, 128);
	m_leakList.InsertColumn(1, L"Refs", LVCFMT_LEFT, 40);
	m_leakList.InsertColumn(2, L"Node Type", LVCFMT_LEFT, 72);
	m_leakList.InsertColumn(3, L"ID", LVCFMT_LEFT, 128);
	m_leakList.InsertColumn(4, L"Class", LVCFMT_LEFT, 256);

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

	for (std::vector<LeakEntry>::const_iterator it = m_leaks.begin(); it != m_leaks.end(); ++it) {
		LeakEntry const& entry = *it;
		if (showRecentOnly && !entry.isRecent)
			continue;

		MSHTML::IHTMLDOMNodePtr node = entry.node;
		MSHTML::IHTMLElementPtr elem = entry.node;

		wchar_t refCountText[32];
		wsprintf(refCountText, L"%d", entry.refCount);

		int idx = m_leakList.GetItemCount();
		m_leakList.InsertItem(idx, entry.url);
		m_leakList.SetItemText(idx, 1, refCountText);
		m_leakList.SetItemText(idx, 2, node->nodeName);

		if (elem) {
			m_leakList.SetItemText(idx, 3, elem->id);
			m_leakList.SetItemText(idx, 4, elem->innerHTML);
		}
		else {
			m_leakList.SetItemText(idx, 3, L"");
			m_leakList.SetItemText(idx, 4, L"");
		}
	}
	if (m_leakList.GetItemCount() > 0)
		m_leakList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);

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

	LeakEntry& entry = m_leaks.at(nItem);
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
		LeakEntry& entry = m_leaks.at(iCurrentItem);
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
