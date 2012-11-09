// StepPop.cpp : implementation file
//

#include "lc_global.h"
#include "StepPop.h"

#include "project.h"
#include "lc_application.h"
#include "lc_model.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStepPopup

CStepPopup::CStepPopup()
{
}

CStepPopup::CStepPopup(CPoint pt, CWnd* pParentWnd)
{
	pt.x = min (pt.x, GetSystemMetrics(SM_CXSCREEN)-100);
	pt.y = min (pt.y, GetSystemMetrics(SM_CYSCREEN)-45);

	CString szClassName = AfxRegisterWndClass(CS_SAVEBITS|CS_HREDRAW|CS_VREDRAW, 0, (HBRUSH)GetStockObject(LTGRAY_BRUSH),0);
	CWnd::CreateEx(0, szClassName, _T(""), WS_VISIBLE|WS_POPUP|WS_DLGFRAME, 
		pt.x, pt.y, 100, 45, pParentWnd->GetSafeHwnd(), 0, NULL);

	m_Slider.Create (WS_CHILD|WS_VISIBLE|TBS_BOTH|TBS_HORZ|TBS_NOTICKS, CRect(5,10,90,30), this, 1000);

	m_Slider.SetRange(1, lcGetActiveProject()->IsAnimation() ? lcGetActiveProject()->m_ActiveModel->m_TotalFrames : LC_OBJECT_TIME_MAX);
	m_Slider.SetPos(lcGetActiveProject()->m_ActiveModel->m_CurFrame);
}

CStepPopup::~CStepPopup()
{
}


BEGIN_MESSAGE_MAP(CStepPopup, CWnd)
	//{{AFX_MSG_MAP(CStepPopup)
	ON_WM_NCDESTROY()
	ON_WM_KILLFOCUS()
	ON_WM_ACTIVATEAPP()
	ON_WM_HSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CStepPopup message handlers

void CStepPopup::OnNcDestroy() 
{
	AfxGetMainWnd()->SendMessage(WM_LC_POPUP_CLOSE);
	CWnd::OnNcDestroy();
    delete this;
}

void CStepPopup::OnKillFocus(CWnd* pNewWnd) 
{
	CWnd::OnKillFocus(pNewWnd);

	if (pNewWnd && (pNewWnd->m_hWnd != m_Slider.m_hWnd))
		DestroyWindow();
}

void CStepPopup::OnActivateApp(BOOL bActive, ACTIVATEAPPPARAM hTask) 
{
	CWnd::OnActivateApp(bActive, hTask);
	
	if (!bActive)
		DestroyWindow();
}

void CStepPopup::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	int pos = m_Slider.GetPos();
	if (pos > 0)
		lcGetActiveProject()->HandleCommand(LC_VIEW_STEP_SET, pos);

	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}