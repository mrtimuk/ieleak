//-------------------------------------------------------------------
// CVORegistry header file
//-------------------------------------------------------------------
// 
// Copyright ©2000 Virtual Office Systems Incorporated
// All Rights Reserved                      
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included.
//
// This code can be compiled, modified and distributed freely, providing
// that this copyright information remains intact in the distribution.
//
// This code may be compiled in original or modified form in any private 
// or commercial application.
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability for any damage, in any form, caused
// by this code. Use it at your own risk.
//-------------------------------------------------------------------

#if !defined(AFX_VOREGISTRY_H__CF669509_0779_46F6_AFB4_A7E78C5219E3__INCLUDED_)
#define AFX_VOREGISTRY_H__CF669509_0779_46F6_AFB4_A7E78C5219E3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "VOString.h"

class CVORegistry  
{
public:
	DWORD GetValueSize(LPCTSTR pcszValueName);
	enum ValueType { typeError = 0, typeBinary, typeDWORD, typeString, typeStringList };
	enum ValueType GetValueType(LPCTSTR pcszValueName);
	CVORegistry(HKEY hkeyParent, LPCTSTR pcszSubkey, BOOL fCreateIfNew = TRUE);
	CVORegistry(const CVORegistry& rSrc);	// Default Copy Constructor
	virtual ~CVORegistry();
	BOOL Close();
	BOOL DeleteSubkey(LPCTSTR pcszSubkey);
	BOOL DeleteValue(LPCTSTR pcszValueName);
	DWORD GetDisposition()	{ return m_dwDisposition; }
	BOOL GetFirstSubkey(CVOString& strSubkey);
	BOOL GetFirstValue(CVOString &strValueName);
	BOOL GetNextSubkey(CVOString& strSubkey);
	BOOL GetNextValue(CVOString &strValueName);
	BOOL GetSubkey(CVOString& strSubkey, int nOffset);
	BOOL ReadBinary(LPCTSTR pcszKey, LPBYTE pData, DWORD dwBufferSize);
	BOOL ReadBinary(LPCTSTR pcszKey, LPBYTE pData, LPDWORD lpcbData);
	DWORD ReadDWORD(LPCTSTR pcszKey, DWORD dwDefault = 0xDEF0);
	CVOString ReadString(LPCTSTR pcszKey, LPCTSTR pcszDefault = NULL);
	BOOL SubkeyExists(LPCTSTR pcszSubkey);
	BOOL ValueExists(LPCTSTR pcszValueName);
	BOOL WriteBinary(LPCTSTR pcszKey, LPBYTE pData, DWORD cbData);
	BOOL WriteDWORD(LPCTSTR pcszKey, DWORD dwValue);
	BOOL WriteString(LPCTSTR pcszKey, LPCTSTR pcszValue);
	operator HKEY()	{ return m_hkey; }
	operator HKEY*() { return &m_hkey; }
	operator LPCTSTR() { return m_strSubkey; }

protected:
	HKEY m_hkey;
	DWORD m_dwDisposition;
	int m_nSubkeyIndex;
	CVOString m_strSubkey;
};

#endif // !defined(AFX_REGISTRY_H__CF669509_0779_46F6_AFB4_A7E78C5219E3__INCLUDED_)
