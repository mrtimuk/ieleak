#pragma once

#include "Graph.h"
#include "JSHook.hpp"
#include "MemDCCtrl.h"

// CMemoryGraphCtrl

class CMemoryGraphCtrl : public CMemDCCtrl
{
	DECLARE_DYNAMIC(CMemoryGraphCtrl)

public:
	CMemoryGraphCtrl(CComObject<JSHook>* hook);
	virtual ~CMemoryGraphCtrl();

// Attributes
public:

// Operations
public:
	void addPoint(int memUsage, int domNodes);

	enum UpdateSpeed {
		kHigh,
		kNormal,
		kLow,
		kPaused
	};
	enum GraphSeries {
		kMemory,
		kDOM
	};
	void setUpdateSpeed(UpdateSpeed speed);
	void setActiveSeries(GraphSeries series);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphCtrl)
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(CGraphCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg NCHITTEST_RESULT OnNcHitTest(CPoint point);
	afx_msg void OnGraphShowMemory();
	afx_msg void OnGraphShowDOM();
	afx_msg void OnGraphUpdateSpeedHigh();
	afx_msg void OnGraphUpdateSpeedLow();
	afx_msg void OnGraphUpdateSpeedNormal();
	afx_msg void OnGraphUpdateSpeedPaused();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	virtual bool shouldRedrawControl(CRect zone);
	virtual void drawControl(CDC* pDC, CRect zone);

private:
	int getMouseXPos(CRect zone);

	CComObject<JSHook>* m_hook;
	CGraph m_graph;
	size_t m_memSeries, m_domSeries;
	int m_displayedMouseXPos;
	CFont m_statsFont;

	UpdateSpeed m_updateSpeed;
	GraphSeries m_activeSeries;
};
