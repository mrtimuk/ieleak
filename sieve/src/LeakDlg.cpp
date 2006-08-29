#include "stdafx.h"
#include "resource.h"
#include "JSHook.hpp"
#include "LeakDlg.hpp"
#include "MainBrowserDlg.hpp"
#include "PropDlg.hpp"
#include <afxtempl.h>

int g_sortOrder = 1;

CLeakDlg::CLeakDlg(CComObject<JSHook>* hook,CWnd* pParent) : CDialog(CLeakDlg::IDD, pParent), m_edit_items(0), m_lastSortedColumn(-1)
{
	m_hook = hook;
	m_hook->AddRef();
	//{{AFX_DATA_INIT(CLeakDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CLeakDlg::~CLeakDlg()
{
	m_hook->Release();
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
	m_leakList.InsertColumn(COL_DOC, L"doc", LVCFMT_LEFT, 60);
	m_leakList.InsertColumn(COL_URL, L"URL", LVCFMT_LEFT, 300);
	m_leakList.InsertColumn(COL_REFS, L"Refs", LVCFMT_LEFT, 40);
	m_leakList.InsertColumn(COL_TAG, L"Tag", LVCFMT_LEFT, 75);
	m_leakList.InsertColumn(COL_ID, L"ID", LVCFMT_LEFT, 80);
	m_leakList.InsertColumn(COL_ORPHAN, L"Orphan", LVCFMT_LEFT, 50);
	m_leakList.InsertColumn(COL_LEAK, L"Leak", LVCFMT_LEFT, 40);
	m_leakList.InsertColumn(COL_CYCLE, L"Cycle", LVCFMT_LEFT, 40);
	m_leakList.InsertColumn(COL_OUTERHTML, L"outerHTML", LVCFMT_LEFT, 800);
	//m_leakList.InsertColumn(COL_ADDRESS, L"Address", LVCFMT_LEFT, 80);
	//m_leakList.InsertColumn(COL_SIZE,L"Size", LVCFMT_LEFT, 50);

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

void CLeakDlg::prepare(LPCTSTR title)
{
	this->LockWindowUpdate();
	this->clearLeaks();
	m_edit_items = 0;
	m_edit_hidden_items = 0;
	SetWindowText(title);
	//m_edit_bytes = 0;
	//m_edit_kbytes = 0;
}

void CLeakDlg::finish()
{
	UpdateData(FALSE); // From members to controls
	updateButtons();
	sortOnDefaultColumn();
	this->UnlockWindowUpdate();
	this->ShowWindow(SW_SHOW);
}

BOOL getIsNodeOrphan(IUnknown* unk) { 
	MSHTML::IHTMLElementPtr	node = unk;
	return ( node && node->parentElement == NULL );
}

// Adds an element to the leak list, including its document URL and ref count.
//
void CLeakDlg::addElement(Elem *hookElem) 
{
	if ( hookElem && hookElem->unkNode )
	{
		int idx = m_edit_items++;
		Elem* elem = new Elem(hookElem); // Make a Copy;
		hookElem->reported = hookElem->refCountSample;
		MSHTML::IHTMLDOMNode2Ptr node = elem->unkNode;
		elem->isOrphan = getIsNodeOrphan(elem->unkNode);

		wchar_t refCountText[64];
		//wchar_t addressText[64];
		//wchar_t sizeText[64];
		wchar_t seqNrText[12];
		wchar_t docIdText[12];

		_itow(elem->seqNr, seqNrText, 10);
		_itow(elem->docId, docIdText, 10);
		_itow(elem->refCountSample, refCountText, 10);
		//_ultow(elem->size, size, 10);
		//wsprintf(addressText,L"0x%08x",elem->unkNode);

		m_leakList.InsertItem(idx,seqNrText);
		m_leakList.SetItemText(idx, COL_DOC, docIdText);
		m_leakList.SetItemText(idx, COL_URL, elem->url);
		m_leakList.SetItemText(idx, COL_REFS, refCountText);
		m_leakList.SetItemText(idx, COL_ORPHAN, (elem->isOrphan) ? L"Yes" : L"");
		if ( !elem->running && elem->refCountSample > 0)
		{
			m_leakList.SetItemText(idx, COL_LEAK, L"leak!");
			hookElem->leakReported = 1;
		}
		if ( elem->cycleDetected )
		{
			m_leakList.SetItemText(idx, COL_CYCLE, L"cycle!");
			hookElem->leakReported = 1;
		}
		m_leakList.SetItemData(idx,(DWORD_PTR) elem);
		//m_leakList.SetItemText(idx, COL_ADDRESS, addressText);
		//m_leakList.SetItemText(idx, COL_SIZE, sizeText);
		m_leakList.SetItemText(idx, COL_TAG, elem->nodeName);

		BSTR sValue = NULL;
		if ( GetPropertyValueByName((CComQIPtr<IDispatchEx>)node, L"id", &sValue) )
		{
			elem->id = sValue; // Will be saved for sorting; Will be freed by ~Elem
			m_leakList.SetItemText(idx, COL_ID, sValue);
		}
		sValue = NULL;

		if ( elem->running )
		{
			if ( GetPropertyValueByName((CComQIPtr<IDispatchEx>)node, L"outerHTML", &sValue) )
			{
				wchar_t outerHTML[200];
				wcsncpy(outerHTML,sValue,200);
				m_leakList.SetItemText(idx, COL_OUTERHTML, outerHTML);
			}
			SysFreeString(sValue);  // Value not saved
			sValue = NULL;
		}
		//m_edit_bytes += elem->size;
	}
	else
	{
		m_edit_hidden_items++;
	}
}

void CLeakDlg::notifyElement(Elem* hookElem)
{
	// Search hookElem in leakList
	int itemCount = m_leakList.GetItemCount();
	int seqNr = hookElem->seqNr;
	for ( int idx = 0; idx < itemCount; idx++ )
	{
		Elem *elem = (Elem*) m_leakList.GetItemData(idx);
		if ( elem->seqNr == seqNr )
		{
			hookElem->reported = hookElem->refCountSample;

			// Update important values in the element to notify;
			elem->refCountSample = hookElem->refCountSample;
			elem->running = hookElem->running;
			elem->cycleDetected = hookElem->cycleDetected;
			wchar_t refCountText[64];
			_itow(elem->refCountSample, refCountText, 10);
			m_leakList.SetItemText(idx, COL_REFS, refCountText);
			if ( elem->cycleDetected )
			{
				m_leakList.SetItemText(idx, COL_CYCLE, L"cycle!");
				hookElem->leakReported = 1;
			}
			if ( !elem->running && elem->refCountSample > 0 )
			{
				m_leakList.SetItemText(idx, COL_LEAK, L"leak!");
				hookElem->leakReported = 1;
			}
			else if ( elem->refCountSample == 0 )
			{
				m_leakList.SetItemText(idx, COL_LEAK, L"freed");
			}
			m_leakList.RedrawItems(idx,idx);
			break;
		}
	}
}

// Clear all leaks.
//
void CLeakDlg::clearLeaks()
{
	int itemCount = m_leakList.GetItemCount();
	for ( int i = 0; i < itemCount; i++ )
	{
		Elem *elem = (Elem*) m_leakList.GetItemData(i);
		delete elem; // Free saved data;
	}
	m_leakList.DeleteAllItems();
	m_edit_items = 0;
	m_edit_hidden_items = 0;
	UpdateData(FALSE); // From members to controls
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

		Elem *elem = (Elem*) m_leakList.GetItemData((int)(pLVCD->nmcd.dwItemSpec));
		if ( elem->reported == 0 )
		{
			// Make the item red when item is reported the first time
 	        pLVCD->clrText = RGB(255,0,0);
		} 
		else if ( elem->reported < elem->refCountSample  )
		{
			// Make the item blue when the refcount is increased since last report
 	        pLVCD->clrText = RGB(0,0,255);
		}
		else if ( elem->reported > elem->refCountSample  )
		{
			// Make the item darkgreen when the refcount is decreased since last report
 	        pLVCD->clrText = RGB(0,140,0);
		}
		if ( elem->refCountSample == 0 )
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
		Elem* elem = (Elem*)  m_leakList.GetItemData(m_leakList.GetNextItem(-1, LVNI_SELECTED));
		GetDlgItem(IDC_PROPERTIES_BUTTON)->EnableWindow(elem->refCountSample == 0 ? FALSE : TRUE);
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

	Elem *elem = (Elem*) m_leakList.GetItemData(nItem);
	if (getHook()->m_nodes.find(elem->unkNode) != getHook()->m_nodes.end())
	{
		CComQIPtr<IDispatchEx> disp = elem->unkNode;
		CPropDlg propDlg(CStringW(L"Properties"), this);
		propDlg.setObject(disp);
		propDlg.DoModal();
	}
	else
	{
		AfxMessageBox(L"Invalid node; most likely already released");
	}
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
		
		Elem *elem = (Elem*) m_leakList.GetItemData(iCurrentItem);
		// Serialize object
		CStringW header;

		header.Format(_T("%s\t(%i reference%s)\r\n"), elem->url, elem->refCountSample, elem->refCountSample != 1 ? "s" : "");
		text += header;

		// Load object properties
		try
		{
			GetObjectProperties((CComQIPtr<IDispatchEx>)elem->unkNode, aDispIDs, asNames, asValues);

			for (int iPropCnt = 0; iPropCnt < aDispIDs.GetSize(); iPropCnt++)
				text += "\t" + asNames[iPropCnt] + "\t" + asValues[iPropCnt] + "\r\n";
			text += "\r\n\r\n";
		}
		catch (...)
		{
			CStringW props;
			props.Format(_T("\ttagName\t%s\n\tid\t%s\r\n"),elem->nodeName,elem->id);
			text += props;
		}
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
	Elem* entry1 = (Elem*) lParam1;
	Elem* entry2 = (Elem*) lParam2;

	switch (lParamSort)
	{
	case COL_NR:
		return (entry1->seqNr  < entry2->seqNr ? -1 : ( entry1->seqNr > entry2->seqNr ? 1 : 0)) * g_sortOrder;

	case COL_DOC:
		return (entry1->docId  < entry2->docId ? -1 : ( entry1->docId > entry2->docId ? 1 : 0)) * g_sortOrder;

	case COL_URL:
		if ( entry1->url && entry2->url )
		{
			return myStrCmp(entry1->url,entry2->url)  * g_sortOrder;
		}
		if ( entry1->url ) return -1 * g_sortOrder;
		if ( entry2->url ) return 1 * g_sortOrder;
		return 0; 

	case COL_REFS:
		return (entry1->refCountSample < entry2->refCountSample ? -1 : ( entry1->refCountSample > entry2->refCountSample ? 1 : 0)) * g_sortOrder;

	case COL_TAG:
		return myStrCmp(entry1->nodeName,entry2->nodeName) * g_sortOrder;

	case COL_ID:
		if ( entry1->id != NULL && entry2->id != NULL )
		{
			return myStrCmp(entry1->id,entry2->id)  * g_sortOrder;
		}
		if ( entry1->id ) return -1  * g_sortOrder;
		if ( entry2->id ) return 1  * g_sortOrder;
		return 0; 

//	case COL_SIZE:
//		return (entry2->size < entry1->size ? -1 : ( entry2->size > entry1->size ? 1 : 0))  * g_sortOrder;

	case COL_LEAK:
		if ( entry1->running == entry2->running ) return 0;
		if ( entry1->running ) return 1 * g_sortOrder;
		if ( entry2->running ) return -1 * g_sortOrder;
		return 0;

	case COL_CYCLE:
		if ( entry1->cycleDetected == entry2->cycleDetected ) return 0;
		if ( entry1->cycleDetected ) return 1 * g_sortOrder;
		if ( entry2->cycleDetected ) return -1 * g_sortOrder;
		return 0;

	case COL_ORPHAN:
		{
			if ( entry1->isOrphan == entry2->isOrphan ) return 0;
			if ( entry1->isOrphan ) return 1 * g_sortOrder;
			if ( entry2->isOrphan ) return -1 * g_sortOrder;
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
	m_leakList.SortItems(SortFunc, COL_DOC);  // primary sort on docId;
	m_lastSortedColumn = m_lastSortedColumn < 0 ?  COL_URL :  m_lastSortedColumn;  // Default the URL Column
	m_leakList.SortItems(SortFunc, m_lastSortedColumn);  // secondary sort on specified column;
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