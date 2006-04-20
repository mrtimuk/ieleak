#include "stdafx.h"
#include "resource.h"
#include "LeakDlg.hpp"
#include "PropDlg.hpp"
#include ".\leakdlg.hpp"
#include <afxtempl.h>

CLeakDlg::CLeakDlg(CWnd* pParent) : CDialog(CLeakDlg::IDD, pParent) {
	//{{AFX_DATA_INIT(CLeakDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CLeakDlg::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CLeakDlg)
	DDX_Control(pDX, IDC_LEAKLIST, m_leakList);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CLeakDlg, CDialog)
	//{{AFX_MSG_MAP(CLeakDlg)
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
END_MESSAGE_MAP()

BOOL CLeakDlg::OnInitDialog() {
	CDialog::OnInitDialog();

	// Full-row selection is more intuitive
	//
	m_leakList.SetExtendedStyle(m_leakList.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

	// Set up the columns for the leak list.
	//
	m_leakList.InsertColumn(0, L"URL", LVCFMT_LEFT, 128);
	m_leakList.InsertColumn(1, L"Refs", LVCFMT_LEFT, 40);
	m_leakList.InsertColumn(2, L"Tag", LVCFMT_LEFT, 64);
	m_leakList.InsertColumn(3, L"ID", LVCFMT_LEFT, 128);
	m_leakList.InsertColumn(4, L"Class", LVCFMT_LEFT, 256);

	// Populate the leak list control from the list of LeakEntry structures.
	//
	populateLeaks();

	// Enable/disable button
	//
	updateButtons();

	// Set up resizing
	//
	m_resizeHelper.Init(m_hWnd);
	m_resizeHelper.Fix(IDC_LEAKLIST, DlgResizeHelper::kLeftRight, DlgResizeHelper::kTopBottom);
	m_resizeHelper.Fix(IDC_PROPERTIES_BUTTON, DlgResizeHelper::kWidthRight, DlgResizeHelper::kHeightTop);
	m_resizeHelper.Fix(IDC_COPY, DlgResizeHelper::kWidthRight, DlgResizeHelper::kHeightTop);
	m_resizeHelper.Fix(IDOK, DlgResizeHelper::kWidthRight, DlgResizeHelper::kHeightTop);

	return TRUE;
}

void CLeakDlg::OnPaint() {
	CDialog::OnPaint();
	m_resizeHelper.OnGripperPaint();
}

UINT CLeakDlg::OnNcHitTest(CPoint point)
{
	UINT ht = CDialog::OnNcHitTest(point);
	m_resizeHelper.OnGripperNcHitTest(point, ht);
	return ht;
}

void CLeakDlg::OnClose() {
	EndDialog(0);
}

void CLeakDlg::OnOk() {
	EndDialog(0);
}

void CLeakDlg::OnDestroy() {
	// Free the leak list when the dialog is destroyed.
	//
	clearLeaks();
}

// Adds an element to the leak list, including its document URL and ref count.
//
void CLeakDlg::addElement(IUnknown* elem, BSTR url, int refCount) {
	// Add a reference to the element, and allocate a copy of the URL string.
	//
	elem->AddRef();
	m_leaks.push_back(LeakEntry(elem, SysAllocString(url), refCount));
}

// Clear all leaks.
//
void CLeakDlg::clearLeaks() {
	for (std::vector<LeakEntry>::const_iterator it = m_leaks.begin(); it != m_leaks.end(); ++it) {
		// Release the leaked element and free its associated document URL.
		//   (of course, since this element is over-referenced, it won't actually get freed
		//   properly, but we're doing our part, at least!)
		//
		it->elem->Release();
		SysFreeString(it->url);
	}
	m_leaks.clear();
}

// Take all entries in m_leaks and populate the leak list control with them.
//
void CLeakDlg::populateLeaks() {
	int idx = 0;
	for (std::vector<LeakEntry>::const_iterator it = m_leaks.begin(); it != m_leaks.end(); ++it, ++idx) {
		LeakEntry const& entry = *it;
		MSHTML::IHTMLElementPtr elem = entry.elem;

		wchar_t refCountText[32];
		_itow(entry.refCount, refCountText, 10);

		m_leakList.InsertItem(idx, entry.url);
		m_leakList.SetItemText(idx, 1, refCountText);
		m_leakList.SetItemText(idx, 2, elem->tagName);
		m_leakList.SetItemText(idx, 3, elem->id);
		m_leakList.SetItemText(idx, 4, elem->innerHTML);
		IHTMLElementPtr parentElement = elem->parentElement;
	}
	if (m_leakList.GetItemCount() > 0)
		m_leakList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
}

// When a leaked element is selected, display its properties.
//
afx_msg void CLeakDlg::OnLvnItemActivateLeaklist(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	showItemProperties(pNMLV->iItem);
}

void CLeakDlg::OnLvnItemChangedLeaklist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	
	*pResult = 0;
	updateButtons();
}

void CLeakDlg::OnLvnKeydownLeaklist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	if (GetKeyState(VK_CONTROL) < 0 && pLVKeyDown->wVKey == 'C')
		CopySelectedItems();

	*pResult = 0;
}

void CLeakDlg::updateButtons()
{
	GetDlgItem(IDC_PROPERTIES_BUTTON)->EnableWindow(m_leakList.GetSelectedCount() == 1);
}

void CLeakDlg::showItemProperties(UINT nItem)
{
	// Get the IDispatch interface so that we can dynamically query the
	//   element's properties.
	//
	ASSERT(nItem != -1);

	LeakEntry& entry = m_leaks.at(nItem);
	CComQIPtr<IDispatchEx> disp = entry.elem;

	CPropDlg propDlg(CStringW(L"Memory Leak in ") + entry.url, this);
	propDlg.setObject(disp);
	propDlg.DoModal();
}

afx_msg void CLeakDlg::OnViewProperties()
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
    lptstrCopy[text.GetLength()] = (WCHAR) 0;    // null character 
    GlobalUnlock(hglbCopy); 

    // Place the handle on the clipboard. 
	EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hglbCopy);
	CloseClipboard();
	return true;
}

void CLeakDlg::CopySelectedItems()
{
	CStringW text;

	int iCurrentItem = -1;
	while ((iCurrentItem = m_leakList.GetNextItem(iCurrentItem, LVNI_SELECTED)) != -1) {
		CArray<DISPID> aDispIDs;
		CArray<CStringW> asNames, asValues;
		
		// Load object properties
		LeakEntry& entry = m_leaks.at(iCurrentItem);
		GetObjectProperties((CComQIPtr<IDispatchEx>)entry.elem, aDispIDs, asNames, asValues);

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

void CLeakDlg::OnSize(UINT nType, int cx, int cy) {
	m_resizeHelper.OnSize();
}

void CLeakDlg::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	// Using arbitrary figures here (dialog units would be better)
	//
	lpMMI->ptMinTrackSize.x = 400;
	lpMMI->ptMinTrackSize.y = 250;

	CWnd::OnGetMinMaxInfo(lpMMI);
}
