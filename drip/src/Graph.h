// Graph.h: interface for the CGraph class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GRAPH_H__43C3693F_1CD5_11D6_A6FF_CA0C1DCB2838__INCLUDED_)
#define AFX_GRAPH_H__43C3693F_1CD5_11D6_A6FF_CA0C1DCB2838__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <list>

using namespace std;

class CGraph
{
public:
	CGraph();
	virtual ~CGraph();

	void SetBackgroundColor(COLORREF cl);
	void SetGridLineColor(COLORREF cl);
	void SetGridLineStep(int x, int y);

	size_t GetNumSeries();
	size_t AddSeries(COLORREF color);
	COLORREF GetSeriesColor(size_t series);
	bool IsSeriesVisible(size_t series);

	void AddPoint(size_t series, int y);
	void SetSeriesVisible(size_t series, bool visible);

	// returns the right edge if there are no points
	int GetLeftMostPointInAnySeries(CRect zone);
	bool GetValueAtPoint(size_t series, CRect zone, int x, OUT int& value);

	void DrawGraph(CDC* pDC, CRect Zone);

private:
	COLORREF m_BackgroundColor, m_GridLineColor;
	int m_VertStep, m_HorzStep, m_PointStep;

	struct Series
	{
		COLORREF color;
		vector<int> values;
		bool visible;
	};
	vector<Series*> m_Series;

	void DrawGridLines(CDC* pDC, CRect zone);
	void DrawHorzGridLines(CDC* pDC, CRect zone);
	void DrawVertGridLines(CDC* pDC, CRect zone);

	void DrawGraphLine(Series* series, CDC* pDC, CRect Zone);
	int CalcYPlotPos(CRect zone, int min, int max, int y);

	void CalcMinMax(Series* series, int numPoints, int& min, int& max);
};

#endif // !defined(AFX_GRAPH_H__43C3693F_1CD5_11D6_A6FF_CA0C1DCB2838__INCLUDED_)
