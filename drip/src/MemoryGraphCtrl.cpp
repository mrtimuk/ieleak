// MemoryGraphCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "MainBrowserDlg.hpp"
#include "MemoryGraphCtrl.h"
#include ".\memorygraphctrl.h"

#define TIMER_STATS	0

// CMemoryGraphCtrl

IMPLEMENT_DYNAMIC(CMemoryGraphCtrl, CMemDCCtrl)
CMemoryGraphCtrl::CMemoryGraphCtrl(CComObject<JSHook>* hook) {
	m_hook = hook;
	m_hook->AddRef();

	m_displayedMouseXPos = -1;
	m_memSeries = m_graph.AddSeries(RGB(0,255,0));
	m_domSeries = m_graph.AddSeries(RGB(255,255,0));
	VERIFY(m_statsFont.CreatePointFont(90, L"Sans Serif"));

	m_updateSpeed = kPaused;
}

CMemoryGraphCtrl::~CMemoryGraphCtrl() {
	m_hook->Release();
	m_hook = NULL;
}


BEGIN_MESSAGE_MAP(CMemoryGraphCtrl, CMemDCCtrl)
	//{{AFX_MSG_MAP(CMemoryGraphCtrl)
	ON_WM_CREATE()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_WM_RBUTTONUP()
	ON_WM_NCHITTEST()
	ON_COMMAND(ID_GRAPHUPDATESPEED_HIGH, OnGraphUpdateSpeedHigh)
	ON_COMMAND(ID_GRAPHUPDATESPEED_LOW, OnGraphUpdateSpeedLow)
	ON_COMMAND(ID_GRAPHUPDATESPEED_NORMAL, OnGraphUpdateSpeedNormal)
	ON_COMMAND(ID_GRAPHUPDATESPEED_PAUSED, OnGraphUpdateSpeedPaused)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//  CMemoryGraphCtrl message handlers

void CMemoryGraphCtrl::PreSubclassWindow() {
	CStatic::PreSubclassWindow();
	setUpdateSpeed(kNormal);
}

int CMemoryGraphCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) {
	setUpdateSpeed(kNormal);
	return CStatic::OnCreate(lpCreateStruct);
}

void CMemoryGraphCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	Invalidate();

	CMemDCCtrl::OnMouseMove(nFlags, point);
}

void CMemoryGraphCtrl::OnTimer(UINT_PTR nIDEvent) {
	if (nIDEvent == TIMER_STATS) {
		// Update the node count and memory usage
		//
		size_t usage = CMainBrowserDlg::GetMemoryUsage();
		addPoint((int)usage, (int)m_hook->getNodeCount());
	}

	CStatic::OnTimer(nIDEvent);
}

void CMemoryGraphCtrl::addPoint(int memUsage, int domNodes) {
	m_graph.AddPoint(m_memSeries, memUsage);
	m_graph.AddPoint(m_domSeries, domNodes);
	emptyPaintCache();
	RedrawWindow();
}

bool CMemoryGraphCtrl::shouldRedrawControl(CRect zone) {
	if (CMemDCCtrl::shouldRedrawControl(zone))
		return true;

	// The control will be invalidated every time a point is added,
	// so only force a redraw if the mouse has moved.
	if (m_displayedMouseXPos != getMouseXPos(zone))
		return true;

	return false;
}

void CMemoryGraphCtrl::drawControl(CDC* pDC, CRect zone) {
	m_graph.DrawGraph(pDC, zone);

	// Draw a line where the mouse is currently positioned.
	m_displayedMouseXPos = getMouseXPos(zone);
	CPen mousePen(PS_SOLID, 1, RGB(255,255,255));
	CPen* oldPen = pDC->SelectObject(&mousePen);
	pDC->MoveTo(m_displayedMouseXPos, 0);
	pDC->LineTo(m_displayedMouseXPos, zone.bottom);
	pDC->SelectObject(oldPen);

	// Determine the statistics to display
	vector<string> stats;

	int mem = 0;
	if (m_graph.GetValueAtPoint(m_memSeries, zone, m_displayedMouseXPos, mem)) {
		stats.push_back("Memory Usage: ");
		CStringA memStats;
		memStats.Format("%i", mem);
		stats.push_back((const char*)memStats);
	}
	else {
		stats.push_back("");
		stats.push_back("");
	}

	int nodes = 0;
	if (m_graph.GetValueAtPoint(m_domSeries, zone, m_displayedMouseXPos, nodes)) {
        CStringA nodeStats;
		nodeStats.Format("%i", nodes);
		stats.push_back("Total DOM Nodes: ");
		stats.push_back((const char*)nodeStats);
	}
	else {
		stats.push_back("");
		stats.push_back("");
	}

	ASSERT(stats.size() == m_graph.GetNumSeries()*2);

	// Determine the positions of the stats
	const int margin = 5;
	int xPos = zone.left + margin;
	int yPos = zone.top + margin;

	vector<CRect> statPos;
	CRect dimensions(xPos, yPos, xPos, yPos);
	for (size_t i = 0; i < stats.size(); i++) {
		// determine the position of the stat
		CPoint size = pDC->GetTextExtent(stats.at(i).c_str());
		CRect pos(xPos, yPos, xPos + size.x, yPos + size.y);
		statPos.push_back(pos);
		dimensions.UnionRect(&dimensions, &pos);

		yPos += size.y + margin;
	}

	// Draw the stats, if possible
	if (zone.PtInRect(dimensions.TopLeft()) && zone.PtInRect(dimensions.BottomRight())) {
		int oldMode = pDC->SetBkMode(TRANSPARENT);
		CFont* oldFont = pDC->SelectObject(&m_statsFont);

		for (size_t i = 0; i < stats.size(); i++) {
			CPoint pos = statPos.at(i).TopLeft();

			int oldColor = pDC->SetTextColor(m_graph.GetSeriesColor(i/2));
			pDC->TextOut(pos.x, pos.y, stats.at(i).c_str());
			pDC->SetTextColor(oldColor);
		}

		pDC->SelectObject(oldFont);
		pDC->SetBkMode(oldMode);
	}
}

int CMemoryGraphCtrl::getMouseXPos(CRect zone) {
	int x = zone.right;

	// get the cursor position relative to the control
	CPoint cursor;
	if (GetCursorPos(&cursor)) {
		ScreenToClient(&cursor);
		if (zone.PtInRect(cursor))
			x = cursor.x;
	}

	int min = m_graph.GetLeftMostPointInAnySeries(zone);
	if (x < min)
		x = min;

	return x;
}

void CMemoryGraphCtrl::setUpdateSpeed(UpdateSpeed speed)
{
	m_updateSpeed = speed;

	KillTimer(TIMER_STATS);

	// determine the new update interval
	int interval;
	switch (m_updateSpeed) {
		case kHigh:
			interval = 500;
			break;
		case kNormal:
			interval = 2000;
			break;
		case kLow:
			interval = 4000;
			break;
		case kPaused:
			return;
		default:
			ASSERT(false);
			return;
	}
	SetTimer(TIMER_STATS, interval, NULL);
}

void CMemoryGraphCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
	// the event's point is relative to the client, but the menu is displayed relative to the screen.
	CPoint screen = point;
	ClientToScreen(&screen);

	// load the menu
	CMenu* menu = CMenu::FromHandle(LoadMenu(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_GRAPH_MENU)));
	menu = menu->GetSubMenu(0);

	// set the radio button for the update speed
	UINT id = 0;
	switch (m_updateSpeed)  {
		case kHigh:		id = ID_GRAPHUPDATESPEED_HIGH; break;
		case kNormal:	id = ID_GRAPHUPDATESPEED_NORMAL; break;
		case kLow:		id = ID_GRAPHUPDATESPEED_LOW; break;
		case kPaused:	id = ID_GRAPHUPDATESPEED_PAUSED; break;
		default:		ASSERT(false); break;
	}
	CMenu* updateSpeedMenu = menu->GetSubMenu(0);
	updateSpeedMenu->CheckMenuRadioItem(ID_GRAPHUPDATESPEED_HIGH, ID_GRAPHUPDATESPEED_PAUSED, id, MF_BYCOMMAND);

	menu->TrackPopupMenu(TPM_LEFTBUTTON|TPM_RIGHTBUTTON, screen.x, screen.y, this, NULL);

	CMemDCCtrl::OnRButtonUp(nFlags, point);
}

UINT CMemoryGraphCtrl::OnNcHitTest(CPoint point)
{
	// to allow right-click events (instead of the SS_NOTIFY style)
	return HTCLIENT;
}

void CMemoryGraphCtrl::OnGraphUpdateSpeedHigh()
{
	setUpdateSpeed(kHigh);
}

void CMemoryGraphCtrl::OnGraphUpdateSpeedNormal()
{
	setUpdateSpeed(kNormal);
}

void CMemoryGraphCtrl::OnGraphUpdateSpeedLow()
{
	setUpdateSpeed(kLow);
}

void CMemoryGraphCtrl::OnGraphUpdateSpeedPaused()
{
	setUpdateSpeed(kPaused);
}
