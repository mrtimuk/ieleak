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
	SetGridLineStep(12, 12);
	m_PointStep = 1;
}

CGraph::~CGraph()
{
	vector<Series*>::iterator iter = m_Series.begin();
	while (iter != m_Series.end())
	{
		delete *iter;
		iter++;
	}
	m_Series.clear();
}

void CGraph::SetBackgroundColor(COLORREF cl)
{
	m_BackgroundColor = cl;
}

void CGraph::SetGridLineColor(COLORREF cl)
{
	m_GridLineColor = cl;
}

void CGraph::SetGridLineStep(int x, int y)
{
	m_VertStep = x;
	m_HorzStep = y;
}

size_t CGraph::GetNumSeries()
{
	return m_Series.size();
}

size_t CGraph::AddSeries(COLORREF color)
{
	Series* series = new Series;
	series->color = color;
	series->visible = true;
	m_Series.push_back(series);
	return m_Series.size()-1;
}

COLORREF CGraph::GetSeriesColor(size_t series)
{
	ASSERT(series >= 0 && series < m_Series.size());
	return m_Series.at(series)->color;
}

bool CGraph::IsSeriesVisible(size_t series)
{
	ASSERT(series >= 0 && series < m_Series.size());
	return m_Series.at(series)->visible;
}

void CGraph::AddPoint(size_t series, int y)
{
	ASSERT(series >= 0 && series < m_Series.size());
	m_Series.at(series)->values.push_back(y);
}

void CGraph::SetSeriesVisible(size_t series, bool visible)
{
	ASSERT(series >= 0 && series < m_Series.size());
	m_Series.at(series)->visible = visible;
}

int CGraph::GetLeftMostPointInAnySeries(CRect zone)
{
	size_t maxItems = 0;

	// count the number of items
	vector<Series*>::iterator iter = m_Series.begin();
	while (iter != m_Series.end())
	{
		maxItems = max(maxItems, (*iter)->values.size());
		iter++;
	}

	if (maxItems == 0)
		return zone.right;
	else if (maxItems > (size_t)zone.Width())
		return zone.left;
	else
		return zone.right - (int)maxItems + 1;
}

bool CGraph::GetValueAtPoint(size_t series, CRect zone, int x, OUT int& value)
{
	ASSERT(series >= 0 && series < m_Series.size());
	vector<int>* values = &m_Series.at(series)->values;
	size_t numItems = values->size();

	size_t index = numItems - 1 - (zone.right - x);
	if (index < 0 || index >= numItems)
		return false;

	value = values->at(index);
	return true;
}

void CGraph::DrawGraph(CDC* pDC, CRect zone)
{
	DrawGridLines(pDC, zone);

	vector<Series*>::iterator iter = m_Series.begin();
	while (iter != m_Series.end())
	{
		if ((*iter)->visible)
			DrawGraphLine(*iter, pDC, zone);
		iter++;
	}
}

void CGraph::DrawGridLines(CDC* pDC, CRect zone)
{
	pDC->FillSolidRect(zone, m_BackgroundColor);

	CPen vPen(PS_SOLID, 1, m_GridLineColor);
	CPen* pOld = pDC->SelectObject(&vPen);

	DrawHorzGridLines(pDC, zone);
	DrawVertGridLines(pDC, zone);

	pDC->SelectObject(pOld);
}

void CGraph::DrawHorzGridLines(CDC* pDC, CRect zone)
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

void CGraph::DrawVertGridLines(CDC* pDC, CRect zone)
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

void CGraph::DrawGraphLine(Series* series, CDC* pDC, CRect zone)
{
	CPen vPen(PS_SOLID, 1, series->color);
	CPen* pOld = pDC->SelectObject(&vPen);

	// Draw all points, starting from the right. Do not draw more than PtCnt points.
	vector<int>::reverse_iterator iter = series->values.rbegin();

	int numPoints = zone.Width() / m_PointStep;
	int curPoint = numPoints-1;

	int min, max;
	CalcMinMax(series, numPoints, min, max);

	while (iter != series->values.rend() && curPoint >= 0)
	{
		CPoint pt;
		pt.x = zone.left + zone.Width() * curPoint / numPoints;
		pt.y = CalcYPlotPos(zone, min, max, *iter);

		if (iter == series->values.rbegin())
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

void CGraph::CalcMinMax(Series* series, int numPoints, int& min, int& max)
{
	min = 0;
	max = 0;

	bool hasMinMax = false;

	int i = 0;
	vector<int>::reverse_iterator iter = series->values.rbegin();
	while (iter != series->values.rend() && i < numPoints)
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
