// MemDCCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "MemDCCtrl.h"


// CMemDCCtrl

IMPLEMENT_DYNAMIC(CMemDCCtrl, CStatic)
CMemDCCtrl::CMemDCCtrl() {
	m_bitmap = NULL;
}

CMemDCCtrl::~CMemDCCtrl() {
	emptyPaintCache();
}

BEGIN_MESSAGE_MAP(CMemDCCtrl, CStatic)
	//{{AFX_MSG_MAP(CMemDCCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CMemDCCtrl message handlers

void CMemDCCtrl::OnPaint() {
	CPaintDC dc(this); // device context for painting

	// Ensure that the bitmap has been created and is up-to-date
	CRect zone;
	GetClientRect(zone);
	if (m_bitmap && shouldRedrawControl(zone))
		emptyPaintCache();

	if (!m_bitmap) {
		// Get the number of color planes and color bits used on the current display
		HDC hdcScreen = CreateDC(L"DISPLAY", NULL, NULL, NULL);
		int iPlanes = GetDeviceCaps(hdcScreen, PLANES);
		int iBitCount = GetDeviceCaps(hdcScreen, BITSPIXEL);
		DeleteDC(hdcScreen);

		// Allocate and create the bitmap
		m_bitmap = new CBitmap;
		m_bitmap->CreateBitmap(zone.Width(), zone.Height(), iPlanes, iBitCount, NULL);

		// Save the dimensions of the bitmap.
		m_bitmap->SetBitmapDimension(zone.Width(), zone.Height());

		CDC dcBitmapCache;
		dcBitmapCache.CreateCompatibleDC(&dc);
		dcBitmapCache.SelectObject(m_bitmap);
		drawControl(&dcBitmapCache, zone);
	}

	// Copy to the DC
	CDC dcBitmap;
	dcBitmap.CreateCompatibleDC(&dc);
	dcBitmap.SelectObject(m_bitmap);
	dc.BitBlt(zone.left, zone.top, zone.Width(), zone.Height(), &dcBitmap, 0, 0, SRCCOPY);
}

BOOL CMemDCCtrl::OnEraseBkgnd(CDC* pDC) {
	return false;
}

void CMemDCCtrl::emptyPaintCache() {
	delete m_bitmap;
	m_bitmap = NULL;
}

bool CMemDCCtrl::shouldRedrawControl(CRect zone) {
	// Redraw the control when it's resized.
	if (m_bitmap->GetBitmapDimension() != CSize(zone.BottomRight()))
		return true;

	return false;
}
