#include "stdafx.h"
#include "DlgResizeHelper.h"

void DlgResizeHelper::Init(HWND a_hParent) {
  m_hParent = a_hParent;
  m_ctrls.clear();
  if (::IsWindow(m_hParent)) {

    // keep original parent size
    ::GetWindowRect(m_hParent, m_origParentSize);

    // get all child windows and store their original sizes and positions
    HWND hCtrl = ::GetTopWindow(m_hParent);
    while (hCtrl) {
      CtrlSize cs;
      cs.m_hCtrl = hCtrl;
      ::GetWindowRect(hCtrl, cs.m_origSize);
      ::ScreenToClient(m_hParent, &cs.m_origSize.TopLeft());
      ::ScreenToClient(m_hParent, &cs.m_origSize.BottomRight());
      m_ctrls.push_back(cs);

      hCtrl = ::GetNextWindow(hCtrl, GW_HWNDNEXT);
    }
  }
}

void DlgResizeHelper::Add(HWND a_hWnd) {
  if (m_hParent && a_hWnd) {
    CtrlSize cs;
    cs.m_hCtrl = a_hWnd;
    ::GetWindowRect(a_hWnd, cs.m_origSize);
    ::ScreenToClient(m_hParent, &cs.m_origSize.TopLeft());
    ::ScreenToClient(m_hParent, &cs.m_origSize.BottomRight());
    m_ctrls.push_back(cs);
  }
}

void DlgResizeHelper::OnSize() {
  if (::IsWindow(m_hParent)) {
    CRect currParentSize;
    ::GetWindowRect(m_hParent, currParentSize);

    double xRatio = ((double) currParentSize.Width()) / m_origParentSize.Width();
    double yRatio = ((double) currParentSize.Height()) / m_origParentSize.Height();

    // resize child windows according to their fix attributes
    CtrlCont_t::const_iterator it;
    for (it=m_ctrls.begin(); it!=m_ctrls.end(); ++it) {
      CRect currCtrlSize;
      EHFix hFix = it->m_hFix;
      EVFix vFix = it->m_vFix;

      // might go easier ;-)
      if (hFix & kLeft) {
        currCtrlSize.left = it->m_origSize.left;
      } else {
        currCtrlSize.left = ((hFix & kWidth) && (hFix & kRight)) ? (it->m_origSize.left + currParentSize.Width() - m_origParentSize.Width()) : (LONG)(it->m_origSize.left * xRatio);
      }
      if (hFix & kRight) {
        currCtrlSize.right = it->m_origSize.right + currParentSize.Width() - m_origParentSize.Width();
      } else {
        currCtrlSize.right = (hFix & kWidth) ? (currCtrlSize.left + it->m_origSize.Width()) : (LONG)(it->m_origSize.right * xRatio);
      }

      if (vFix & kTop) {
        currCtrlSize.top = it->m_origSize.top;
      } else {
        currCtrlSize.top = ((vFix & kHeight) && (vFix & kBottom)) ? (it->m_origSize.top + currParentSize.Height() - m_origParentSize.Height()) : (LONG)(it->m_origSize.top * yRatio);
      }
      if (vFix & kBottom) {
        currCtrlSize.bottom = it->m_origSize.bottom + currParentSize.Height() - m_origParentSize.Height();
      } else {
        currCtrlSize.bottom = (vFix & kHeight) ? (currCtrlSize.top + it->m_origSize.Height()) : (LONG)(it->m_origSize.bottom * yRatio);
      }

      // resize child window
      ::MoveWindow(it->m_hCtrl, currCtrlSize.left, currCtrlSize.top, currCtrlSize.Width(), currCtrlSize.Height(), TRUE);
    }
  }
}

BOOL DlgResizeHelper::Fix(HWND a_hCtrl, EHFix a_hFix, EVFix a_vFix) {
  CtrlCont_t::iterator it;
  for(it = m_ctrls.begin(); it!=m_ctrls.end(); ++it) {
    if (it->m_hCtrl == a_hCtrl) {
      it->m_hFix = a_hFix;
      it->m_vFix = a_vFix;
      return TRUE;
    }
  }
  return FALSE;
}

BOOL DlgResizeHelper::Fix(int a_itemId, EHFix a_hFix, EVFix a_vFix) {
  return Fix(::GetDlgItem(m_hParent, a_itemId), a_hFix, a_vFix);
}

BOOL DlgResizeHelper::Fix(EHFix a_hFix, EVFix a_vFix) {
  CtrlCont_t::iterator it;
  for(it = m_ctrls.begin(); it!=m_ctrls.end(); ++it) {
    it->m_hFix = a_hFix;
    it->m_vFix = a_vFix;
  }
  return TRUE;
}

/*
UINT DlgResizeHelper::Fix(LPCTSTR a_pszClassName, EHFix a_hFix, EVFix a_vFix) {
  char pszCN[200];  // ToDo: size?
  UINT cnt = 0;
  CtrlCont_t::iterator it;
  for(it = m_ctrls.begin(); it!=m_ctrls.end(); ++it) {
    ::GetClassName(it->m_hCtrl, pszCN, sizeof(pszCN));
    if (strcmp(pszCN, a_pszClassName) == 0) {
      cnt++;
      it->m_hFix = a_hFix;
      it->m_vFix = a_vFix;
    }
  }
  return cnt;
}
*/

void DlgResizeHelper::OnGripperPaint()
{
  if(!IsIconic(m_hParent) && !IsZoomed(m_hParent))
  {
    CRect rect;
    GetClientRect(m_hParent, &rect);
    rect.left = rect.right - ::GetSystemMetrics(SM_CXHSCROLL);
    rect.top = rect.bottom - ::GetSystemMetrics(SM_CYVSCROLL);

    HDC hDC = GetDC(m_hParent);
    DrawFrameControl(hDC, rect, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
    ReleaseDC(m_hParent, hDC);
  }
}

void DlgResizeHelper::OnGripperNcHitTest(CPoint point, IN OUT UINT& ht)
{
  // If it was in the client area...
  if(ht == HTCLIENT && !IsZoomed(m_hParent))
  {
    // Get the rect for the dlg, and resize the rect
    // so that it contains only the sizer.  If the test
    // is in that area, set the e-num value so that it
    // will think it is the bottom-right corner of the dlg.
    CRect rc;
    GetWindowRect(m_hParent, rc);
    rc.left = rc.right - GetSystemMetrics(SM_CXHSCROLL);
    rc.top = rc.bottom - GetSystemMetrics(SM_CYVSCROLL);
    if(rc.PtInRect(point))
       ht = HTBOTTOMRIGHT;
  }
}
