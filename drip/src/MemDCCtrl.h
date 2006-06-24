#pragma once


// CMemDCCtrl

class CMemDCCtrl : public CStatic
{
	DECLARE_DYNAMIC(CMemDCCtrl)

public:
	CMemDCCtrl();
	virtual ~CMemDCCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMemDCCtrl)
	//}}AFX_VIRTUAL

// Implementation
	// Generated message map functions
protected:
	//{{AFX_MSG(CMemDCCtrl)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	virtual bool shouldRedrawControl(CRect zone);
	virtual void drawControl(CDC* pDC, CRect zone)=0;

protected:
	void emptyPaintCache();

private:
	CBitmap* m_bitmap;
};
