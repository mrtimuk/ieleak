#include "stdafx.h"
#include "resource.h"
#include "PropDlg.hpp"
#include ".\propdlg.hpp"
#include <afxtempl.h>

CPropDlg::CPropDlg(CStringW titleDesc, CWnd* pParent) : CDialog(CPropDlg::IDD, pParent) {
	//{{AFX_DATA_INIT(CPropDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_titleDesc = titleDesc;
}

void CPropDlg::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CPropDlg)
	DDX_Control(pDX, IDC_PROPLIST, m_propList);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPropDlg, CDialog)
	//{{AFX_MSG_MAP(CPropDlg)
	ON_WM_PAINT()
	//ON_WM_NCHITTEST()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDOK, OnOk)
	//}}AFX_MSG_MAP
	ON_NOTIFY(LVN_ITEMACTIVATE, IDC_PROPLIST, OnLvnItemActivateProplist)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_PROPLIST, OnLvnItemchangedProplist)
	ON_BN_CLICKED(IDC_PROPERTIES_BUTTON, OnBnClickedPropertiesButton)
END_MESSAGE_MAP()


BEGIN_EASYSIZE_MAP(CPropDlg)
	//EASYSIZE(<control id>,		left,			top,			right,			bottom,		options)

	//Anchored to top/right
	EASYSIZE(IDC_PROPERTIES_BUTTON,	ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)
	EASYSIZE(IDOK,					ES_KEEPSIZE,	ES_BORDER,		ES_BORDER,		ES_KEEPSIZE,	0)

	//Anchored to left + top + right + bottom
	EASYSIZE(IDC_PROPLIST,			ES_BORDER,		ES_BORDER,		ES_BORDER,		ES_BORDER,		0)

	//EASYSIZE(<control id>,		left,			top,			right,			bottom,		options)	
END_EASYSIZE_MAP


bool GetPropertyValue(CComPtr<IDispatchEx> object, CStringW propertyName, VARIANT& result)
{
	DISPID dispId;
	BSTR name = SysAllocString(propertyName);
	HRESULT hr = object->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_SYSTEM_DEFAULT, &dispId);
	SysFreeString(name);
	if (FAILED(hr))
		return false;

	return GetPropertyValue(object, dispId, result);
}

bool GetPropertyValue(CComPtr<IDispatchEx> object, DISPID dispId, VARIANT& result)
{
	// Invoke the property get to get the property's value.
	//
	DISPPARAMS params;
	memset(&params, 0, sizeof(DISPPARAMS));
	VariantInit(&result);

	BSTR memberName = NULL;
	if (SUCCEEDED(object->GetMemberName(dispId, &memberName))) {
		// BEGIN HARDCODE - Accessing the "filters" property (for example, from Google Maps)
		// BEGIN HARDCODE - Accessing the "__sIEve_" property
		// causes crashes access violations deep in MSHTML.
		//

		bool skip = CString(memberName).Compare(L"filters") == 0;
		bool skip2 =CString(memberName).Find(L"__sIEve_",0) == 0; 

		SysFreeString(memberName);
		if (skip || skip2)
			return false;
	}

	if (SUCCEEDED(object->Invoke(dispId, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET,
		&params, &result, NULL, NULL))) {
		return true;
	}
	else {
		return false;
	}
}

MSHTML::IHTMLDocument2Ptr GetContentDocument(CComPtr<IDispatchEx> object)
{
	IHTMLDocument2Ptr doc = NULL;
	DISPID dispId;
	if ( SUCCEEDED(object->GetDispID(L"contentWindow", fdexNameCaseSensitive, &dispId)) )
	{
		VARIANT result;
		if ( GetPropertyValue(object, dispId, result))
		{
			if (result.vt == VT_DISPATCH)
			{
				if (result.pdispVal)
				{
					IHTMLWindow2Ptr wnd = result.pdispVal;
					IHTMLDocument2* tmpdoc = NULL;
					wnd->get_document(&tmpdoc);
					doc = tmpdoc;
					if ( tmpdoc != NULL && doc != NULL ) tmpdoc->Release();
				}
			}
		}
		VariantClear(&result);
	}
	return doc;
}

// Cordys Specific test
bool GetLibraryURL(CComPtr<IDispatchEx> object, BSTR *url)
{
	bool retVal = false;
	DISPID dispId;
	if ( SUCCEEDED(object->GetDispID(L"library", fdexNameCaseSensitive, &dispId)) )
	{
		VARIANT result;
		if (GetPropertyValue(object, dispId, result))
		{
			if (result.vt == VT_DISPATCH)
			{
				if (result.pdispVal)
				{
					CComQIPtr<IDispatchEx> dispEx = result.pdispVal;
					if ( GetPropertyValueByName(dispEx,L"url",url) )
					{
						retVal = true;
					}
				}
			}
		}
		VariantClear(&result);
	}
	return retVal;
}

bool GetPropertyValueByName(CComPtr<IDispatchEx> object, CComBSTR name, BSTR *sValue)
{
	bool retVal = false;
	DISPID dispId;
	if ( SUCCEEDED(object->GetDispID(name, fdexNameCaseSensitive, &dispId)) )
	{
		VARIANT result;
		if (GetPropertyValue(object, dispId, result))
		{
			if (result.vt == VT_DISPATCH)
			{
				if (result.pdispVal)
				{
					*sValue = CComBSTR(L"[object]").Detach();
				}
				else
				{
					*sValue = CComBSTR(L"[null object]").Detach();
				}
				retVal = true;
			}
			else
			{
				VariantChangeType(&result, &result, 0, VT_BSTR);
				if ((result.vt == VT_BSTR) && (result.bstrVal != NULL))
				{
					*sValue = CComBSTR(result.bstrVal).Detach();
					retVal = true;
				}
			}
		}
		VariantClear(&result);
	}
	return retVal;
}



void GetObjectProperties(CComPtr<IDispatchEx> object, CArray<DISPID> &raDispIDs,
						 CArray<CStringW> &rasNames, CArray<CStringW> &rasValues)
{
	// Iterate over all of the object's properties.
	//
	DISPID dispId = 0;
	HRESULT hr = object->GetNextDispID(fdexEnumAll, DISPID_STARTENUM, &dispId);
	while (SUCCEEDED(hr) && (hr != S_FALSE)) {
		BSTR memberName = NULL;
		if (SUCCEEDED(object->GetMemberName(dispId, &memberName))) {
			CStringW sName(memberName), sValue;
			if ( sName.Find(L"__sIEve_",0) != 0 )  // Hide sIEve internals to avoid confusion
			{
				VARIANT result;
				if (GetPropertyValue(object, dispId, result)) {
					// Add the new property to the list, changing the value to a string.
					//

					if (result.vt == VT_DISPATCH) {
						if (result.pdispVal)
						{
							sValue = CComBSTR(L"[object]");
						}
						else
							sValue = CComBSTR(L"[null object]");
					}
					else {
						VariantChangeType(&result, &result, 0, VT_BSTR);
						if ((result.vt == VT_BSTR) && (result.bstrVal != NULL))
							sValue = CComBSTR(result.bstrVal);
					}
				}
				else {
					sValue = CComBSTR(L"(unknown)");
				}
				VariantClear(&result);

				raDispIDs.Add(dispId);
				rasNames.Add(sName);
				rasValues.Add(sValue);

				SysFreeString(memberName);
			}
		}	

		hr = object->GetNextDispID(fdexEnumAll, dispId, &dispId);
	}

}

BOOL CPropDlg::OnInitDialog() {
	CDialog::OnInitDialog();

	SetWindowText(L"Properties - " + m_titleDesc);

	// Full-row selection is more intuitive
	//
	m_propList.SetExtendedStyle(m_propList.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

	// Set up the name/value colums for the property list.
	//
	m_propList.InsertColumn(0, L"Name", LVCFMT_LEFT, 128);
	m_propList.InsertColumn(1, L"Value", LVCFMT_LEFT, 1000);

	CArray<DISPID> aDispIDs;
	CArray<CStringW> asNames, asValues;
	GetObjectProperties(m_object, aDispIDs, asNames, asValues);
	ASSERT(aDispIDs.GetSize() == asNames.GetSize());
	ASSERT(aDispIDs.GetSize() == asValues.GetSize());

	for (int iItemCnt = 0; iItemCnt < aDispIDs.GetSize(); iItemCnt++)
	{
		int listItem = m_propList.InsertItem(0, asNames[iItemCnt]);
		m_propList.SetItemData(listItem, (DWORD_PTR)aDispIDs[iItemCnt]);
		m_propList.SetItemText(listItem, 1, asValues[iItemCnt]);
	}
	if (m_propList.GetItemCount() > 0)
		m_propList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);

	updateButtons();

	INIT_EASYSIZE;
	return TRUE;
}

void CPropDlg::OnPaint() {
	CDialog::OnPaint();
}

UINT CPropDlg::OnNcHitTest(CPoint point)
{
	UINT ht = CDialog::OnNcHitTest(point);
	return ht;
}

CStringW CPropDlg::getPropName(int idx) {
	DISPID dispId = (DISPID)m_propList.GetItemData(idx);
	BSTR memberName = NULL;
	CStringW name;
	if (SUCCEEDED(m_object->GetMemberName(dispId, &memberName))) {
		name = memberName;
		SysFreeString(memberName);
	}

	return name;
}

void CPropDlg::OnClose() {
	EndDialog(0);
}

void CPropDlg::OnOk() {
	EndDialog(0);
}

void CPropDlg::OnDestroy() {
}

void CPropDlg::setObject(CComPtr<IDispatchEx> obj) {
	m_object = obj;
}

void CPropDlg::OnLvnItemActivateProplist(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMITEMACTIVATE NMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	OnBnClickedPropertiesButton();
}

void CPropDlg::OnLvnItemchangedProplist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	updateButtons();
	*pResult = 0;
}

void CPropDlg::updateButtons()
{
	bool bEnableProperties = false;
	if (m_propList.GetSelectedCount() == 1)
	{
		int idx = m_propList.GetNextItem(-1, LVNI_SELECTED);
		DISPID dispId = (DISPID)m_propList.GetItemData(idx);

		VARIANT value;
		bEnableProperties = GetPropertyValue(m_object, dispId, value) &&
			value.vt == VT_DISPATCH && value.pdispVal;
		VariantClear(&value);
	}

	GetDlgItem(IDC_PROPERTIES_BUTTON)->EnableWindow(bEnableProperties);
}

void CPropDlg::OnBnClickedPropertiesButton()
{
	// Get the IDispatch interface so that we can dynamically query the
	//   element's properties.
	//
	if (m_propList.GetSelectedCount() == 1)
	{
		int idx = m_propList.GetNextItem(-1, LVNI_SELECTED);
		DISPID dispId = (DISPID)m_propList.GetItemData(idx);
		
		VARIANT value;
		if (GetPropertyValue(m_object, dispId, value) && value.vt == VT_DISPATCH) {
			CPropDlg propDlg(getPropName(idx), this);
			CComQIPtr<IDispatchEx> dispEx = value.pdispVal;
			if (dispEx) {
				propDlg.setObject(dispEx);
				propDlg.DoModal();
			}
		}
		VariantClear(&value);
	}
}

void CPropDlg::OnSize(UINT nType, int cx, int cy) {
	CDialog::OnSize(nType, cx, cy);
	UPDATE_EASYSIZE;
}

void CPropDlg::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
{
	// Using arbitrary figures here (dialog units would be better)
	//
	lpMMI->ptMinTrackSize.x = 350;
	lpMMI->ptMinTrackSize.y = 300;

	CWnd::OnGetMinMaxInfo(lpMMI);
}
