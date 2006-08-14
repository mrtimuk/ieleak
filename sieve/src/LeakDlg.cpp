#include "stdafx.h"
#include "resource.h"
#include "JSHook.hpp"
#include "LeakDlg.hpp"
#include "MainBrowserDlg.hpp"
#include "PropDlg.hpp"
#include <afxtempl.h>

int g_sortOrder = 1;

CLeakDlg::CLeakDlg(CWnd* pParent) : CDialog(CLeakDlg::IDD, pParent), m_edit_items(0), m_lastSortedColumn(-1)
, m_check_show_all(FALSE)
{
	//{{AFX_DATA_INIT(CLeakDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CLeakDlg::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CLeakDlg)
	DDX_Control(pDX, IDC_LEAKLIST, m_leakList);
	//}}AFX_DATA_MAP
	//DDX_Text(pDX, IDC_EDIT_BYTES, m_edit_bytes);
	//DDX_Text(pDX, IDC_EDIT_KBYTES, m_edit_kbytes);
	DDX_Text(pDX, IDC_EDIT_ITEMS, m_edit_items);
	DDX_Text(pDX, IDC_EDIT_HIDDEN_ITEMS, m_edit_hidden_items);
	DDX_Check(pDX, IDC_CHECK_SHOW_ALL, m_check_show_all);
}

BEGIN_MESSAGE_MAP(CLeakDlg, CDialog)
	//{{AFX_MSG_MAP(CLeakDlg)
	ON_WM_PAINT()
	//ON_WM_NCHITTEST()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDOK, OnOk)
	ON_BN_CLICKED(IDC_PROPERTIES_BUTTON, OnViewProperties)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
	ON_NOTIFY(LVN_ITEMACTIVATE, IDC_LEAKLIST, OnLvnItemActivateLeaklist)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LEAKLIST, OnLvnItemChangedLeaklist)
	ON_BN_CLICKED(IDC_COPY, CopySelectedItems)
	ON_BN_CLICKED(IDC_CLEAR, clearInUse)
	ON_BN_CLICKED(IDC_REFRESH, showInUse)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LEAKLIST, OnLvnKeydownLeaklist)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LEAKLIST, OnCustomdrawLeakList)
	ON_NOTIFY(HDN_ITEMCLICK, 0, OnHdnItemclickLeaklist)
	ON_NOTIFY(WM_NCHITTEST, IDC_LEAKLIST, OnWmNcHitTest)
	ON_BN_CLICKED(IDC_CHECK_SHOW_ALL, OnBnClickedCheckShowAll)
	ON_BN_CLICKED(IDC_REFRESH_LEAKS, OnBnClickedRefreshLeaks)
END_MESSAGE_MAP()

//EASYSIZE(<control id>,left,top,right,bottom,options)
//l,t,r,b = ES_KEEPSIZE or ES_BORDER or <control id> (keep distance to border or <control id>)
//options = ES_HCENTER | ES_VCENTER or 0

BEGIN_EASYSIZE_MAP(CLeakDlg)
	//EASYSIZE(<control id>,		left,			top,			right,			bottom,		options)

	//Anchored to top/right
	EASYSIZE(IDC_PROPERTIES_BUTTON,	ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_COPY,				ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_CLEAR,				ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_REFRESH,			ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_REFRESH_LEAKS,		ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_EDIT_ITEMS,		ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_EDIT_HIDDEN_ITEMS,	ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_CHECK_SHOW_ALL,	ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_STATIC_ITEMS,		ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDC_STATIC_HIDDEN_ITEMS,ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)

	//Anchored to right + bottom
	EASYSIZE(IDOK,					ES_KEEPSIZE,	ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		0)
	EASYSIZE(IDC_STATIC_RED,		ES_KEEPSIZE,	ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		0)
	EASYSIZE(IDC_STATIC_BLUE,		ES_KEEPSIZE,	ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		0)
	EASYSIZE(IDC_STATIC_BLACK,		ES_KEEPSIZE,	ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		0)
	EASYSIZE(IDC_STATIC_GREEN,		ES_KEEPSIZE,	ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		0)

	//Anchored to left + top + right + bottom
	EASYSIZE(IDC_LEAKLIST,			ES_BORDER,		ES_BORDER,		ES_BORDER,		ES_BORDER,		0)

	//EASYSIZE(<control id>,		left,			top,			right,			bottom,		options)	
END_EASYSIZE_MAP

#define COL_NR			0
#define COL_DOC			1
#define COL_URL			2
#define COL_REFS		3
#define COL_TAG			4
#define COL_ID			5
#define COL_ORPHAN		6
#define COL_LEAK		7
#define COL_CYCLE		8
#define COL_OUTERHTML	9
#define COL_ADDRESS		10
#define COL_SIZE		11

BOOL CLeakDlg::OnInitDialog() {
	CDialog::OnInitDialog();

	// Full-row selection is more intuitive
	//
	m_leakList.SetExtendedStyle(m_leakList.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

	// Set up the columns for the leak list.
	//
	m_leakList.InsertColumn(COL_NR,	L"#", LVCFMT_LEFT, 35);
	m_leakList.InsertColumn(COL_DOC, L"doc", LVCFMT_LEFT, 37);
	m_leakList.InsertColumn(COL_URL, L"URL", LVCFMT_LEFT, 300);
	m_leakList.InsertColumn(COL_REFS, L"Refs", LVCFMT_LEFT, 40);
	m_leakList.InsertColumn(COL_TAG, L"Tag", LVCFMT_LEFT, 75);
	m_leakList.InsertColumn(COL_ID, L"ID", LVCFMT_LEFT, 80);
	m_leakList.InsertColumn(COL_ORPHAN, L"Orphan", LVCFMT_LEFT, 50);
	m_leakList.InsertColumn(COL_LEAK, L"Leak", LVCFMT_LEFT, 40);
	m_leakList.InsertColumn(COL_CYCLE, L"Cycle", LVCFMT_LEFT, 40);
	m_leakList.InsertColumn(COL_OUTERHTML, L"outerHTML", LVCFMT_LEFT, 800);
	m_leakList.InsertColumn(COL_ADDRESS, L"Address", LVCFMT_LEFT, 80);
	m_leakList.InsertColumn(COL_SIZE,L"Size", LVCFMT_LEFT, 50);

	// Set up resizing
	//
	//EnableToolTips(TRUE);

	//m_toolTip = new CToolTipCtrl();
	//m_toolTip->Create(this);
	//m_toolTip->AddTool(&m_leakList, LPSTR_TEXTCALLBACK);
	//m_toolTip->Activate(TRUE);

	m_brush.CreateSysColorBrush( COLOR_3DFACE  );
	INIT_EASYSIZE;
	return TRUE;
}

void CLeakDlg::OnPaint() {
	CDialog::OnPaint();
}

UINT CLeakDlg::OnNcHitTest(CPoint point)
{
	UINT ht = CDialog::OnNcHitTest(point);
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
void CLeakDlg::addElement(IUnknown* elem, Elem *hookElem, int refCount, BOOL fromCmallspy, ULONG size, int reported) {
	// Add a reference to the element, and allocate a copy of the URL string.
	//
	if ( fromCmallspy )
	{
		m_leaks.push_back(LeakEntry(elem, NULL, refCount, fromCmallspy, size, reported));
	}
	else
	{
		elem->AddRef();
		m_leaks.push_back(LeakEntry(elem, hookElem, refCount, fromCmallspy, size, reported));
	}
}

// Clear all leaks.
//
void CLeakDlg::clearLeaks() {
	for (std::vector<LeakEntry>::const_iterator it = m_leaks.begin(); it != m_leaks.end(); ++it) {
		// Release the leaked element and free its associated document URL.
		//   (of course, since this element is over-referenced, it won't actually get freed
		//   properly, but we're doing our part, at least!)
		//
		if ( it->fromCmallspy )
		{
		}
		else
		{
			it->elem->Release();
		}
	}
	
	m_edit_items = 0;
	m_edit_hidden_items = 0;
	//m_edit_bytes = 0;
	//m_edit_kbytes = 0;
	m_leaks.clear();
	UpdateData(FALSE); // From members to controls
}

BOOL getIsNodeOrphan(IUnknown* unk) { 
	MSHTML::IHTMLElementPtr	element = unk;
	return ( element && element->parentElement == NULL );
}

// Take all entries in m_leaks and populate the leak list control with them.
//
void CLeakDlg::populateLeaks() {
	if ( m_check_show_all )
	{
		SetWindowText(L"Leaks");
	}
	else
	{
		SetWindowText(L"Elements in use");
	}
	int idx = 0;
	this->LockWindowUpdate();
	m_leakList.DeleteAllItems();
	m_edit_items = 0;
	m_edit_hidden_items = 0;
	//m_edit_bytes = 0;
	//m_edit_kbytes = 0;
	for (std::vector<LeakEntry>::iterator it = m_leaks.begin(); it != m_leaks.end(); ++it, ++idx)
	{
		LeakEntry &entry = *it;

		if ( (entry.refCount > 0 && ! entry.hookElem->hide) || m_check_show_all )
		{

			MSHTML::IHTMLDOMNode2Ptr	element = entry.elem;
			MSHTML::IHTMLDocument2Ptr	document = entry.elem;;
			MSHTML::IHTMLWindow2Ptr		window = entry.elem;;

			wchar_t refCountText[64];
			wchar_t address[64];
			wchar_t size[64];
			wchar_t seqNr[12];
			wchar_t docId[12];

			_itow(entry.hookElem->seqNr, seqNr, 10);
			_itow(entry.hookElem->docElem->docId, docId, 10);
			_itow(entry.refCount, refCountText, 10);
			_ultow(entry.size, size, 10);
			wsprintf(address,L"0x%08x",entry.elem);

			m_leakList.InsertItem(idx,seqNr);
			m_leakList.SetItemText(idx, COL_DOC, docId);
			m_leakList.SetItemText(idx, COL_URL, entry.hookElem->docElem->url);
			m_leakList.SetItemText(idx, COL_REFS, refCountText);
			m_leakList.SetItemText(idx, COL_ADDRESS, address);
			m_leakList.SetItemText(idx, COL_SIZE, size);
			// Leak if:  !running && refcount > 0.
			m_leakList.SetItemText(idx, COL_ORPHAN, (getIsNodeOrphan(entry.elem)) ? L"Yes" : L"");
			m_leakList.SetItemText(idx, COL_LEAK, (!entry.hookElem->docElem->running && entry.refCount > 0) ? L"leak!" : L"");
			m_leakList.SetItemText(idx, COL_CYCLE, ( entry.hookElem->cycleDetected ) ? L"cycle!" : L"");
			m_leakList.SetItemData(idx,(DWORD_PTR) &entry);

			if ( element )
			{
				if ( ! entry.fromCmallspy )
				{
					BSTR sValue = NULL;
					m_leakList.SetItemText(idx, COL_TAG, entry.hookElem->nodeName);
		
					if ( GetPropertyValueByName((CComQIPtr<IDispatchEx>)element, L"id", &sValue) )
					{
						m_leakList.SetItemText(idx, COL_ID, sValue);
					}
					SysFreeString(sValue);
					sValue = NULL;

					if ( entry.hookElem->docElem->running )
					{
						if ( GetPropertyValueByName((CComQIPtr<IDispatchEx>)element, L"outerHTML", &sValue) )
						{
							wchar_t outerHTML[200];
							wcsncpy(outerHTML,sValue,200);
							m_leakList.SetItemText(idx, COL_OUTERHTML, outerHTML);
						}
						SysFreeString(sValue);
						sValue = NULL;
					}
				}
			}
			else if ( document )
			{
				m_leakList.SetItemText(idx, COL_TAG, entry.hookElem->nodeName);
			}
			else if ( window )
			{
				m_leakList.SetItemText(idx, COL_TAG, entry.hookElem->nodeName);
			}
			else
			{
				// Com or Lowlevel Normal Memory Allocation

				/*
				wchar_t text[32];
				IUnknown* unk = NULL;
				ULONG i = 0;
				while ( i < entry.size )
				{
					try
					{
						IUnknown* ptr2 = entry.elem + 1;
						HRESULT ok = ptr2->QueryInterface(IID_IUnknown, (void**)&unk);
						if ( ok && unk )
						{
							unk->Release();
						}
						break;
					}
					catch (...)
					{					
						//MessageBox(L"NOT a COM OBJECT");
					}
					i++;
				}
				if (i >= entry.size)
				{
					wsprintf(text,L"NOT a COM object");
				}
				else
				{
					wsprintf(text,L"A COM object at location base+%l",i);
				}
				m_leakList.SetItemText(idx, 8, text);
				*/
			}
			m_edit_items++;
			//m_edit_bytes += entry.size;
		}
		else
		{
			m_edit_hidden_items++;
		}
	}
	//m_edit_kbytes = m_edit_bytes >> 10;
	UpdateData(FALSE); // From members to controls
	updateButtons();
	sortOnDefaultColumn();
	this->UnlockWindowUpdate();
}

void CLeakDlg::clearInUse()
{
	((CMainBrowserDlg *)(this->GetParent()))->OnBnClickedClearInUse();
}

void CLeakDlg::showInUse()
{
	((CMainBrowserDlg *)(this->GetParent()))->OnBnClickedShowInUse();
}

void CLeakDlg::OnBnClickedRefreshLeaks()
{
	((CMainBrowserDlg *)(this->GetParent()))->OnBnClickedShowLeaks();
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

void CLeakDlg::OnCustomdrawLeakList ( NMHDR* pNMHDR, LRESULT* pResult )
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );

    // Take the default processing unless we set this to something else below.
    *pResult = CDRF_DODEFAULT;

    // First thing - check the draw stage. If it's the control's prepaint
    // stage, then tell Windows we want messages for every item.

    if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
    {
		*pResult = CDRF_NOTIFYITEMDRAW;
    }
    else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
        // This is the prepaint stage for an item. Here's where we set the
        // item's text color. Our return value will tell Windows to draw the
        // item itself, but it will use the new color we set here.
        // We'll cycle the colors through red, green, and light blue.

		LeakEntry *entry = (LeakEntry*) m_leakList.GetItemData((int)(pLVCD->nmcd.dwItemSpec));
		if ( entry->reported == 0 )
		{
			// Make the item red when item is reported the first time
 	        pLVCD->clrText = RGB(255,0,0);
		} 
		else if ( entry->reported < entry->refCount  )
		{
			// Make the item blue when the refcount is increased since last report
 	        pLVCD->clrText = RGB(0,0,255);
		}
		else if ( entry->reported > entry->refCount  )
		{
			// Make the item darkgreen when the refcount is decreased since last report
 	        pLVCD->clrText = RGB(0,140,0);
		}
		if ( ! entry->refCount )
		{
			// Make the item green when the memory item is already freed;
			pLVCD->clrText = RGB(0,200,0);
		}
		
        // Tell Windows to paint the control itself.
        *pResult = CDRF_DODEFAULT;
    }
}

HBRUSH CLeakDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr;
	switch ( pWnd->GetDlgCtrlID() )
	{
	case IDC_STATIC_RED:
		pDC->SetTextColor(RGB(255,0,0));
		pDC->SetBkMode(TRANSPARENT);
		hbr = (HBRUSH) m_brush; 
		break;
	case IDC_STATIC_BLUE:
		pDC->SetTextColor(RGB(0,0,255));
		pDC->SetBkMode(TRANSPARENT);
		hbr = (HBRUSH) m_brush; 
		break;
	case IDC_STATIC_GREEN:
		pDC->SetTextColor(RGB(0,140,0));
		pDC->SetBkMode(TRANSPARENT);
		hbr = (HBRUSH) m_brush; 
		break;
	default:
		hbr = CDialog::OnCtlColor(pDC,pWnd,nCtlColor);
	}
	return hbr;
}

void CLeakDlg::updateButtons()
{
	GetDlgItem(IDC_PROPERTIES_BUTTON)->EnableWindow(m_leakList.GetSelectedCount() == 1);
	((CButton*) GetDlgItem(IDC_PROPERTIES_BUTTON))->SetButtonStyle(BS_DEFPUSHBUTTON,1);

	if ( m_leakList.GetSelectedCount() == 1 )
	{
		GetDlgItem(IDC_PROPERTIES_BUTTON)->EnableWindow(TRUE);
		((CButton*) GetDlgItem(IDC_PROPERTIES_BUTTON))->SetButtonStyle(BS_DEFPUSHBUTTON,1);
		((CButton*) GetDlgItem(IDOK))->SetButtonStyle(BS_PUSHBUTTON,1);
	}
	else
	{
		GetDlgItem(IDC_PROPERTIES_BUTTON)->EnableWindow(FALSE);
		((CButton*) GetDlgItem(IDC_PROPERTIES_BUTTON))->SetButtonStyle(BS_PUSHBUTTON,1);
		((CButton*) GetDlgItem(IDOK))->SetButtonStyle(BS_DEFPUSHBUTTON,1);
	}
}

void CLeakDlg::showItemProperties(UINT nItem)
{
	// Get the IDispatch interface so that we can dynamically query the
	//   element's properties.
	//
	ASSERT(nItem != -1);

	LeakEntry *entry = (LeakEntry*) m_leakList.GetItemData(nItem);
	CComQIPtr<IDispatchEx> disp = entry->elem;

	CPropDlg propDlg(CStringW(L"Properties"), this);
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
		LeakEntry *entry = (LeakEntry*) m_leakList.GetItemData(iCurrentItem);
		GetObjectProperties((CComQIPtr<IDispatchEx>)entry->elem, aDispIDs, asNames, asValues);

		// Serialize object
		CStringW header;
		MSHTML::IHTMLDOMNode2Ptr	node= entry->elem;
		MSHTML::IHTMLDocument2Ptr	doc = node->ownerDocument;

		header.Format(_T("%s\t(%i reference%s)\r\n"), doc->url, entry->refCount, entry->refCount != 1 ? "s" : "");
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
	CDialog::OnSize(nType, cx, cy);
	UPDATE_EASYSIZE;
}

void CLeakDlg::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	// Using arbitrary figures here (dialog units would be better)
	//
	lpMMI->ptMinTrackSize.x = 400;
	lpMMI->ptMinTrackSize.y = 250;

	CWnd::OnGetMinMaxInfo(lpMMI);
}


int myStrCmp(TCHAR* s1, TCHAR* s2)
{
	// Be sure empty elements at end of list

	if ( ! s1 ) s1 = _T("ZZZZ");  
	if ( ! s2 ) s2 = _T("ZZZZ");  
	return _tcscmp(s1,s2);
}

int CALLBACK SortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	LeakEntry* entry1 = (LeakEntry*) lParam1;
	LeakEntry* entry2 = (LeakEntry*) lParam2;

	MSHTML::IHTMLElementPtr elem1 = entry1->fromCmallspy ? NULL : entry1->elem;
	MSHTML::IHTMLElementPtr elem2 = entry2->fromCmallspy ? NULL : entry2->elem;

	switch (lParamSort)
	{
	case COL_NR:
		return (entry1->hookElem->seqNr  < entry2->hookElem->seqNr ? -1 : ( entry1->hookElem->seqNr > entry2->hookElem->seqNr ? 1 : 0)) * g_sortOrder;

	case COL_DOC:
		return (entry1->hookElem->docElem->docId  < entry2->hookElem->docElem->docId ? -1 : ( entry1->hookElem->docElem->docId > entry2->hookElem->docElem->docId ? 1 : 0)) * g_sortOrder;

	case COL_URL:
		if (	entry1->hookElem && entry1->hookElem->docElem && entry1->hookElem->docElem->url && 
				entry2->hookElem && entry2->hookElem->docElem && entry2->hookElem->docElem->url )
		{
			return myStrCmp(entry1->hookElem->docElem->url,entry2->hookElem->docElem->url)  * g_sortOrder;
		}
		if ( elem1 ) return -1 * g_sortOrder;
		if ( elem2 ) return 1 * g_sortOrder;
		return 0; 

	case COL_REFS:
		return (entry1->refCount < entry2->refCount ? -1 : ( entry1->refCount > entry2->refCount ? 1 : 0)) * g_sortOrder;

	case COL_TAG:
		return myStrCmp(entry1->hookElem->nodeName,entry2->hookElem->nodeName) * g_sortOrder;

	case COL_ID:		if ( elem1 != NULL && elem2 != NULL )
		{
			return myStrCmp(elem1->id,elem2->id)  * g_sortOrder;
		}
		if ( elem1 ) return -1  * g_sortOrder;
		if ( elem2 ) return 1  * g_sortOrder;
		return 0; 

	case COL_SIZE:
		return (entry2->size < entry1->size ? -1 : ( entry2->size > entry1->size ? 1 : 0))  * g_sortOrder;

	case COL_LEAK:
		if ( entry1->hookElem->docElem->running == entry2->hookElem->docElem->running ) return 0;
		if ( entry1->hookElem->docElem->running ) return 1 * g_sortOrder;
		if ( entry2->hookElem->docElem->running ) return -1 * g_sortOrder;
		return 0;

	case COL_CYCLE:
		if ( entry1->hookElem->cycleDetected == entry2->hookElem->cycleDetected ) return 0;
		if ( entry1->hookElem->cycleDetected ) return 1 * g_sortOrder;
		if ( entry2->hookElem->cycleDetected ) return -1 * g_sortOrder;
		return 0;

	case COL_ORPHAN:
		{
			BOOL elem1_isOrphan = elem1 ? getIsNodeOrphan(elem1) : false;
			BOOL elem2_isOrphan = elem2 ? getIsNodeOrphan(elem2) : false;
			if ( elem1_isOrphan == elem2_isOrphan ) return 0;
			if ( elem1_isOrphan ) return 1 * g_sortOrder;
			if ( elem2_isOrphan ) return -1 * g_sortOrder;
		}
		return 0;

	default:
		// Don't sort other columns
		break;
	}
	return 0;
}


void CLeakDlg::sortOnDefaultColumn()
{
	m_lastSortedColumn = m_lastSortedColumn < 0 ?  COL_URL :  m_lastSortedColumn;  // Default the URL Column
	m_leakList.SortItems(SortFunc, m_lastSortedColumn);
}

void CLeakDlg::OnHdnItemclickLeaklist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	// TODO: Add your control notification handler code here

	if ( phdr->iItem != m_lastSortedColumn ) 
	{
		g_sortOrder = 1;
	}
	else
	{
		g_sortOrder *= -1;
	}
	m_lastSortedColumn = phdr->iItem;

	m_leakList.SortItems(SortFunc, phdr->iItem);
	*pResult = 0;
}

BOOL CLeakDlg::PreTranslateMessage(MSG* pMsg) 
{
    // TODO: Add your specialized code here and/or call the base class

    //if(m_toolTip != NULL)
    //   m_toolTip->RelayEvent(pMsg);

    return CDialog::PreTranslateMessage(pMsg);
}

//Notification handler
BOOL CLeakDlg::OnToolTipNotify(UINT id, NMHDR *pNMHDR,
   LRESULT *pResult)
{
   // need to handle both ANSI and UNICODE versions of the message
   TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
   TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
   CString strTipText;
   ULONGLONG nID = (UINT)(pNMHDR->idFrom);
   if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
      pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
   {
      // idFrom is actually the HWND of the tool
      nID = (::GetDlgCtrlID((HWND)nID));
   }

	//int xid = m_leakList.HitTest(m_point);

   _tcscpy(pTTTW->szText,L"HALLO HALLO HALLO");
/*
   if (nID != 0) // will be zero on a separator
      strTipText.Format("Control ID = %d", nID);

   if (pNMHDR->code == TTN_NEEDTEXTA)
      lstrcpyn(pTTTA->szText, strTipText, sizeof(pTTTA->szText));
   else
      ::MultiByteToWideChar( CP_ACP , 0, strTipText, -1, pTTTW->szText, sizeof(pTTTW->szText) );
*/  
  *pResult = 0;

   return TRUE;    // message was handled
}

//void CLeakDlg::OnNMHoverLeaklist(NMHDR *pNMHDR, LRESULT *pResult)
//{
//	// TODO: Add your control notification handler code here
//
//	AfxMessageBox(L"Hover over ");
//	*pResult = 0;
//}

void CLeakDlg::OnWmNcHitTest(NMHDR *pNMHDR, LRESULT *pResult)
{
	AfxMessageBox(L"HITHIT");
	// TODO: Add your control notification handler code here
	*pResult = 0;
}
void CLeakDlg::OnBnClickedCheckShowAll()
{
	m_check_show_all = ! m_check_show_all;
	populateLeaks();
}