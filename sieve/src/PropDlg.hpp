#pragma once

#include <afxtempl.h>
#include "resource.h"

void GetObjectProperties(CComPtr<IDispatchEx> object, CArray<DISPID> &raDispIDs,
						 CArray<CStringW> &rasNames, CArray<CStringW> &rasValues);

bool GetPropertyValue(CComPtr<IDispatchEx> object, CStringW propertyName, VARIANT& result);
bool GetPropertyValue(CComPtr<IDispatchEx> object, DISPID dispId, VARIANT& result);
bool GetPropertyValueByName(CComPtr<IDispatchEx> object, CComBSTR name, BSTR *sValue);
MSHTML::IHTMLDocument2Ptr GetContentDocument(CComPtr<IDispatchEx> object);
bool GetLibraryURL(CComPtr<IDispatchEx> object, BSTR *url);



// A dialog box for displaying an object's properties and their values.
//
class CPropDlg : public CDialog {
	DECLARE_EASYSIZE
private:
	enum { IDD = IDD_PROPERTIES };
	CListCtrl				m_propList;
	CComPtr<IDispatchEx>	m_object;
	CStringW				m_titleDesc;

	CStringW getPropName(int idx);
	void updateButtons();

public:
	CPropDlg(CStringW titleDesc, CWnd* pParent = NULL);
	void setObject(CComPtr<IDispatchEx> obj);

	DECLARE_MESSAGE_MAP()

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg UINT OnNcHitTest(CPoint point);
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnOk();
	afx_msg void OnLvnItemActivateProplist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedProplist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedPropertiesButton();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
};
