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
	void SetGraphLineColor(COLORREF cl);
	void SetGridLineStep(int x, int y);

	void AddPoint(int y);

	void DrawGraph(CDC* pDC, CRect Zone);

private:
	COLORREF m_BackgroundColor, m_GridLineColor, m_GraphLineColor;
	int m_VertStep, m_HorzStep, m_PointStep;

	list<int> m_Values;

	void DrawGridLines(CDC* pDC, CRect zone);
	void DrawHorzGridLines(CDC* pDC, CRect zone);
	void DrawVertGridLines(CDC* pDC, CRect zone);

	void DrawGraphLine(CDC* pDC, CRect Zone);
	int CalcYPlotPos(CRect zone, int min, int max, int y);

	void CalcMinMax(int numPoints, int& min, int& max);
};


class CGraphCtrl : public CStatic
{
// Construction
public:
	CGraphCtrl();

	void ResetPaintCache();

private:
	void UpdatePaintCache();
	CBitmap* m_pBitmap;

	void DrawGraph(CDC* pDC);

// Attributes
public:

// Operations
public:
	void AddPoint(int y);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGraphCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CGraphCtrl)
	afx_msg void OnPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	virtual void DrawControl(CDC* pDC);
	CGraph m_Graph;
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};


#endif // !defined(AFX_GRAPH_H__43C3693F_1CD5_11D6_A6FF_CA0C1DCB2838__INCLUDED_)
