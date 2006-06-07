// Graph.cpp: implementation of the CGraph class.
//
// This code was adapted from Salick Cogan's Graph Animation.
// (See http://www.codeguru.com/Cpp/G-M/multimedia/graphics/article.php/c4737)
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include ".\graph.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGraph::CGraph()
{
	SetBackgroundColor(RGB(0,0,0));
   SetGridLineColor(RGB(0, 128, 64));
	SetGraphLineColor(RGB(0, 255, 0));
	SetGridLineStep(12, 12);
	m_PointStep = 1;
}

CGraph::~CGraph()
{
}

void CGraph::SetBackgroundColor(COLORREF cl)
{
	m_BackgroundColor = cl;
}

void CGraph::SetGridLineColor(COLORREF cl)
{
	m_GridLineColor = cl;
}

void CGraph::SetGraphLineColor(COLORREF cl)
{
	m_GraphLineColor = cl;
}

void CGraph::SetGridLineStep(int x, int y)
{
	m_VertStep = x;
	m_HorzStep = y;
}

void CGraph::AddPoint(int y)
{
   m_Values.push_back(y);
}

void CGraph::DrawGraph(CDC *pDC, CRect zone)
{
	DrawGridLines(pDC, zone);
	DrawGraphLine(pDC, zone);
}

void CGraph::DrawGridLines(CDC* pDC, CRect zone)
{
	pDC->FillSolidRect(zone, m_BackgroundColor);

	CPen vPen(PS_SOLID, 1, m_GridLineColor);
   CPen *pOld = pDC->SelectObject(&vPen);

   DrawHorzGridLines(pDC, zone);
   DrawVertGridLines(pDC, zone);

   pDC->SelectObject(pOld);
}

void CGraph::DrawHorzGridLines(CDC *pDC, CRect zone)
{
	CPoint pt;
	for (int y = zone.Height(); y >= 0; y -= m_HorzStep)
   {
      pt.x = zone.left;
		pt.y = zone.top + y;
		pDC->MoveTo(pt);
		
		pt.x = zone.left + zone.Width();
		pt.y = zone.top + y;
		pDC->LineTo(pt);
	}
}

void CGraph::DrawVertGridLines(CDC *pDC, CRect zone)
{
	CPoint pt;
   for (int x = m_VertStep; x <= zone.Width(); x += m_VertStep)
	{
		pt.x = zone.left + x;
		pt.y = zone.top; 
		pDC->MoveTo(pt);

		pt.x = zone.left + x;
		pt.y = zone.top + zone.Height();
		pDC->LineTo(pt);
	}
}

void CGraph::DrawGraphLine(CDC* pDC, CRect zone)
{
   CPen vPen(PS_SOLID, 1, m_GraphLineColor);
	CPen *pOld = pDC->SelectObject(&vPen);

   // Draw all points, starting from the right. Do not draw more than PtCnt points.
   list<int>::reverse_iterator iter = m_Values.rbegin();
	
	int numPoints = zone.Width() / m_PointStep;
   int curPoint = numPoints-1;

	int min, max;
	CalcMinMax(numPoints, min, max);

   while (iter != m_Values.rend() && curPoint >= 0)
   {
		CPoint pt;
		pt.x = zone.left + zone.Width() * curPoint / numPoints;
		pt.y = CalcYPlotPos(zone, min, max, *iter);

      if (iter == m_Values.rbegin())
         pDC->MoveTo(pt);
      else
		   pDC->LineTo(pt);

		iter++;
      curPoint--;
   }

   pDC->SelectObject(pOld);
}

int CGraph::CalcYPlotPos(CRect zone, int min, int max, int y)
{
	// Allow one vertical step on the top and bottom
	CRect adjZone = zone;
	if (adjZone.Height() > m_VertStep)
	{
		adjZone.top += m_VertStep;
		adjZone.bottom -= m_VertStep;
	}

	// Given the position within the data range, calculate the position in the zone.
	int range = (max - min);
	double decimal = y-min;
	if (range)
		decimal /= range;
	else
		decimal = 0;
	double offset = adjZone.Height() * decimal;

	return adjZone.bottom - (int)offset;
}

void CGraph::CalcMinMax(int numPoints, int& min, int& max)
{
	min = 0;
	max = 0;

	bool hasMinMax = false;

	int i = 0;
	list<int>::reverse_iterator iter = m_Values.rbegin();
	while (iter != m_Values.rend() && i < numPoints)
	{
		if (hasMinMax)
		{
			min = min(min, *iter);
			max = max(max, *iter);
		}
		else
		{
			min = *iter;
			max = *iter;
			hasMinMax = true;
		}

		i++;
		iter++;
	}
}




/////////////////////////////////////////////////////////////////////////////
// CGraphCtrl

CGraphCtrl::CGraphCtrl()
{
	m_pBitmap = NULL;
}

CGraphCtrl::~CGraphCtrl()
{
	ResetPaintCache();
}


BEGIN_MESSAGE_MAP(CGraphCtrl, CStatic)
	//{{AFX_MSG_MAP(CGraphCtrl)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGraphCtrl message handlers

void CGraphCtrl::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	// Ensure that the bitmap has been created and is up-to-date
	CRect zone;
	GetClientRect(zone);
	if (m_pBitmap && m_pBitmap->GetBitmapDimension() != CSize(zone.BottomRight()))
		ResetPaintCache();

	if (!m_pBitmap)
	{
      // Get the number of color planes and color bits used on the current display
		HDC hdcScreen = CreateDC(L"DISPLAY", NULL, NULL, NULL);
      int iPlanes = GetDeviceCaps(hdcScreen, PLANES);
      int iBitCount = GetDeviceCaps(hdcScreen, BITSPIXEL);
      DeleteDC(hdcScreen);

      // Allocate and create the bitmap
      m_pBitmap = new CBitmap;
		m_pBitmap->CreateBitmap(zone.Width(), zone.Height(), iPlanes, iBitCount, NULL);

      // Save the dimensions of the bitmap.
      m_pBitmap->SetBitmapDimension(zone.Width(), zone.Height());

		CDC dcBitmapCache;
		dcBitmapCache.CreateCompatibleDC(&dc);
		dcBitmapCache.SelectObject(m_pBitmap);
		m_Graph.DrawGraph(&dcBitmapCache, zone);
	}

	// Copy to the DC
   CDC dcBitmap;
   dcBitmap.CreateCompatibleDC(&dc);
   dcBitmap.SelectObject(m_pBitmap);
	dc.BitBlt(zone.left, zone.top, zone.Width(), zone.Height(), &dcBitmap, 0, 0, SRCCOPY);
}

void CGraphCtrl::ResetPaintCache()
{
	delete m_pBitmap;
	m_pBitmap = NULL;
}

void CGraphCtrl::AddPoint(int y)
{
	m_Graph.AddPoint(y);
	ResetPaintCache();
	RedrawWindow();
}

void CGraphCtrl::DrawControl(CDC* pDC)
{
	CRect zone;
	GetClientRect(zone);
	m_Graph.DrawGraph(pDC, zone);
}

BOOL CGraphCtrl::OnEraseBkgnd(CDC* pDC)
{
	return false;
}
