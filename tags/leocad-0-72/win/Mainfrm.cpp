// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "LeoCAD.h"
#include "MainFrm.h"
#include "Camera.h"
#include "project.h"
#include "globals.h"

#include "Print.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TOOLBAR_VERSION 1

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_SETFOCUS()
	ON_WM_MEASUREITEM()
	ON_WM_MENUCHAR()
	ON_WM_INITMENUPOPUP()
	ON_COMMAND(ID_VIEW_FULLSCREEN, OnViewFullscreen)
	ON_WM_GETMINMAXINFO()
	ON_COMMAND(ID_FILE_PRINTPIECELIST, OnFilePrintPieceList)
	ON_WM_ACTIVATEAPP()
	//}}AFX_MSG_MAP
	ON_COMMAND_RANGE(ID_PIECEBAR_ZOOMPREVIEW, ID_PIECEBAR_SUBPARTS, OnPieceBar)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PIECEBAR_ZOOMPREVIEW, ID_PIECEBAR_SUBPARTS, OnUpdatePieceBar)
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
	// User messages
	ON_MESSAGE(WM_LC_UPDATE_LIST, OnUpdateList)
	ON_MESSAGE(WM_LC_POPUP_CLOSE, OnPopupClose)
	ON_MESSAGE(WM_LC_ADD_COMBO_STRING, OnAddString)
	ON_MESSAGE(WM_LC_UPDATE_INFO, OnUpdateInfo)
	ON_MESSAGE(WM_LC_UPDATE_SETTINGS, UpdateSettings)
	// Toolbar show/hide
	ON_COMMAND_EX(ID_VIEW_ANIMATION_BAR, OnBarCheck)
	ON_COMMAND_EX(ID_VIEW_TOOLS_BAR, OnBarCheck)
	ON_COMMAND_EX(ID_VIEW_PIECES_BAR, OnBarCheck)
	ON_COMMAND_EX(ID_VIEW_MODIFY_BAR, OnBarCheck)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ANIMATION_BAR, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TOOLS_BAR, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PIECES_BAR, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MODIFY_BAR, OnUpdateControlBarMenu)
END_MESSAGE_MAP()

static UINT indicators[] =
	{ ID_SEPARATOR, ID_INDICATOR_POSITION, ID_INDICATOR_SNAP, ID_INDICATOR_STEP };

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_pwndFullScrnBar = NULL;
	m_bAutoMenuEnable = FALSE;
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetPaneStyle(0, SBPS_STRETCH|SBPS_NORMAL);

	if (!m_wndStandardBar.Create(this) ||
		!m_wndStandardBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	m_wndStandardBar.SetBarStyle(m_wndStandardBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndStandardBar.SetWindowText (_T("Standard"));
	m_wndStandardBar.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);
	m_wndStandardBar.SetButtonStyle(m_wndStandardBar.CommandToIndex(ID_LOCK_ON), m_wndStandardBar.GetButtonStyle(m_wndStandardBar.CommandToIndex(ID_LOCK_ON)) | TBSTYLE_DROPDOWN);
	m_wndStandardBar.SetButtonStyle(m_wndStandardBar.CommandToIndex(ID_SNAP_ON), m_wndStandardBar.GetButtonStyle(m_wndStandardBar.CommandToIndex(ID_SNAP_ON)) | TBSTYLE_DROPDOWN);
	m_wndStandardBar.EnableDocking(CBRS_ALIGN_ANY);

	if (!m_wndToolsBar.Create(this, WS_CHILD | WS_VISIBLE | CBRS_TOP, ID_VIEW_TOOLS_BAR) ||
		!m_wndToolsBar.LoadToolBar(IDR_TOOLSBAR))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	m_wndToolsBar.SetBarStyle(m_wndToolsBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndToolsBar.SetWindowText (_T("Drawing"));
	m_wndToolsBar.EnableDocking(CBRS_ALIGN_ANY);

	if (!m_wndAnimationBar.Create(this, WS_CHILD | WS_VISIBLE | CBRS_TOP, ID_VIEW_ANIMATION_BAR) ||
		!m_wndAnimationBar.LoadToolBar(IDR_ANIMATORBAR))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	m_wndAnimationBar.SetBarStyle(m_wndAnimationBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndAnimationBar.SetWindowText (_T("Animation"));
	m_wndAnimationBar.EnableDocking(CBRS_ALIGN_ANY);

	if (!m_wndPiecesBar.Create(_T("Pieces"), this, CSize(200, 100),
		TRUE, ID_VIEW_PIECES_BAR))
	{
		TRACE0("Failed to create pieces bar\n");
		return -1;      // fail to create
	}

	if (!m_wndModifyDlg.Create(this, IDD_MODIFY,
	    CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_HIDE_INPLACE,
		ID_VIEW_MODIFY_BAR))
	{
		TRACE0("Failed to create modify dialog bar\n");
		return -1;		// fail to create
	}

	EnableDocking(CBRS_ALIGN_ANY);
	m_wndModifyDlg.SetWindowText (_T("Modify"));
	m_wndModifyDlg.EnableDocking(0);
	ShowControlBar(&m_wndModifyDlg, FALSE, FALSE);
	FloatControlBar(&m_wndModifyDlg, CPoint(10,10));

	// To change the resizable control bar sizes, just change the following
	// members. If you need to change them later, don't forget to call
	// RecalcLayout() or DelayRecalcLayout() after you set the size.
	// You can use this technique to load/save the size of the control bar.
	m_wndPiecesBar.m_sizeVert = CSize(226, -1); // y size ignored (stretched)
	m_wndPiecesBar.m_sizeFloat = CSize(226, 270);
	m_wndPiecesBar.SetBarStyle(m_wndPiecesBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndPiecesBar.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);

	DockControlBar(&m_wndStandardBar);
	DockControlBar(&m_wndToolsBar);
	DockControlBar(&m_wndPiecesBar, AFX_IDW_DOCKBAR_RIGHT);
		
	CRect rect;
	RecalcLayout(TRUE);
	m_wndToolsBar.GetWindowRect(&rect);
	rect.OffsetRect(1,0);
	DockControlBar(&m_wndAnimationBar, AFX_IDW_DOCKBAR_TOP, &rect);

	if (theApp.GetProfileInt(_T("Settings"), _T("ToolBarVersion"), 0) == TOOLBAR_VERSION)
		LoadBarState("Toolbars");

	// Bitmap menus are cool !
	CMenu* pMenu = GetMenu();
	if (pMenu)
		pMenu->DestroyMenu();
	HMENU hMenu = NewMenu();
	pMenu = CMenu::FromHandle (hMenu);
	SetMenu (pMenu);
	m_hMenuDefault = hMenu;

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	int status = theApp.GetProfileInt("Settings", "Window Status", -1);
	cs.style &= ~WS_VISIBLE;

	if (status != -1)
	{
		int r,l,b,t;
		char szBuf[60];
		strcpy (szBuf, theApp.GetProfileString("Settings","Window Position"));
		sscanf(szBuf,"%d, %d, %d, %d", &t, &r, &b, &l);

		cs.cx = r - l;
		cs.cy = b - t;
		
		RECT workArea;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
		l += workArea.left;
		t += workArea.top;

		cs.x = min(l, GetSystemMetrics(SM_CXSCREEN) - GetSystemMetrics(SM_CXICON));
		cs.y = min(t, GetSystemMetrics(SM_CYSCREEN) - GetSystemMetrics(SM_CYICON));
	}

	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

// lParam = update pieces, wParam = update colors
LONG CMainFrame::OnUpdateList(UINT lParam, LONG wParam)
{
	if (lParam)
	{
		if (lParam > 2)
			m_wndPiecesBar.m_nCurGroup = lParam - 2;
		m_wndPiecesBar.m_wndPiecesList.UpdateList();
	}

	if (wParam != 0)
	{
		int x = wParam-1;
		if (x < 14)
		    x *= 2;
		else
			x = ((x-14)*2)+1;

		m_wndPiecesBar.m_wndColorsList.SetCurSel(x);
	}

	return TRUE;
}

// Add a string to the pieces combo
LONG CMainFrame::OnAddString(UINT lParam, LONG /*wParam*/)
{
	if (lParam == NULL)
	{
		// Clear list
		m_wndPiecesBar.m_wndPiecesCombo.ResetContent();
		return TRUE;
	}

	// Search if the string is already there
	for (int i = 0; i < m_wndPiecesBar.m_wndPiecesCombo.GetCount();i++)
	{
		char tmp[100];
		m_wndPiecesBar.m_wndPiecesCombo.GetLBText (i, tmp);
		if (strcmp ((char*)lParam, tmp) == 0)
			return TRUE;
	}
	m_wndPiecesBar.m_wndPiecesCombo.AddString ((char*)lParam);

	return TRUE;
}

HMENU CMainFrame::NewMenu()
{
	m_bmpMenu.LoadMenu(IDR_MAINFRAME);
	m_bmpMenu.AddFromToolBar(&m_wndStandardBar, IDR_MAINFRAME);

	// The first parameter is the menu option text. If it's NULL, keep the current text
	// The second option is the ID of the menu option, or the menu
	// option text (use this for adding bitmaps to popup options) to change.
	// The third option is the resource ID of the bitmap.This can also be a
	// toolbar ID in which case the class searches the toolbar for the
	// appropriate bitmap. Only Bitmap and Toolbar resources are supported.
	m_bmpMenu.ModifyODMenu(NULL, ID_FILE_PROPERTIES, IDB_INFO);
	m_bmpMenu.ModifyODMenu(NULL, ID_FILE_SAVEPICTURE, IDB_PHOTO);
	m_bmpMenu.ModifyODMenu(NULL, ID_PIECE_DELETE, IDB_DELETE);
	m_bmpMenu.ModifyODMenu(NULL, ID_PIECE_GROUP, IDB_GROUP);
	m_bmpMenu.ModifyODMenu(NULL, ID_PIECE_UNGROUP, IDB_UNGROUP);
	m_bmpMenu.ModifyODMenu(NULL, ID_VIEW_PREFERENCES, IDB_PREFERENCES);
	m_bmpMenu.ModifyODMenu(NULL, ID_VIEW_ZOOMOUT, IDB_ZOOMOUT);
	m_bmpMenu.ModifyODMenu(NULL, ID_VIEW_ZOOMIN, IDB_ZOOMIN);
	m_bmpMenu.ModifyODMenu(NULL, ID_VIEW_FULLSCREEN, IDB_FULLSCREEN);
	m_bmpMenu.ModifyODMenu(NULL, ID_HELP_FINDER, IDB_HELP);
	m_bmpMenu.ModifyODMenu(NULL, ID_HELP_LEOCADHOMEPAGE, IDB_HOME);
	m_bmpMenu.ModifyODMenu(NULL, ID_HELP_SENDEMAIL, IDB_MAIL);

	m_bmpMenu.ModifyODMenu(NULL, _T("Cameras"), IDB_CAMERA);

/*
	m_menubar.ModifyODMenu(NULL,"Step", IDB_STEP);
*/
	return m_bmpMenu.Detach();
}

void CMainFrame::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	if(lpMeasureItemStruct->CtlType == ODT_MENU)
	{
		if(IsMenu((HMENU)lpMeasureItemStruct->itemID))
		{
			CMenu* cmenu = CMenu::FromHandle((HMENU)lpMeasureItemStruct->itemID);
			if(m_bmpMenu.IsMenu(cmenu))
			{
				m_bmpMenu.MeasureItem(lpMeasureItemStruct);
				return;
			}
		}
	}

	CFrameWnd::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}

LRESULT CMainFrame::OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu) 
{
	if (m_bmpMenu.IsMenu(pMenu))
		return CBMPMenu::FindKeyboardShortcut(nChar, nFlags, pMenu);
	else
		return CFrameWnd::OnMenuChar(nChar, nFlags, pMenu);
}

void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu) 
{
	CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

	if(!bSysMenu)
	{
		if(m_bmpMenu.IsMenu(pPopupMenu))
			CBMPMenu::UpdateMenu(pPopupMenu);
	}
}

LONG CMainFrame::OnUpdateInfo(UINT lParam, LONG wParam)
{
	m_wndModifyDlg.UpdateInfo((void*)lParam, (BYTE)wParam);

	char str[32];
	float pos[3];
	project->GetFocusPosition(pos);
	sprintf (str, "X: %.2f Y: %.2f Z: %.2f", pos[0], pos[1], pos[2]);
	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_POSITION), str);

	return TRUE;
}

LONG CMainFrame::OnPopupClose(UINT /*lParam*/, LONG /*wParam*/)
{
	m_wndStatusBar.m_pPopup = NULL;
	return TRUE;
}

LONG CMainFrame::UpdateSettings(UINT /*lParam*/, LONG /*wParam*/)
{
	int i = theApp.GetProfileInt("Settings", "Piecebar Options", 
		PIECEBAR_PREVIEW|PIECEBAR_GROUP|PIECEBAR_COMBO|PIECEBAR_ZOOMPREVIEW);
	m_wndPiecesBar.m_bPreview = (i & PIECEBAR_PREVIEW) != 0;
	m_wndPiecesBar.m_bSubParts = (i & PIECEBAR_SUBPARTS) != 0;
	m_wndPiecesBar.m_bGroups = (i & PIECEBAR_GROUP) != 0;
	m_wndPiecesBar.m_bCombo = (i & PIECEBAR_COMBO) != 0;
	m_wndPiecesBar.m_bNumbers = (i & PIECEBAR_PARTNUMBERS) != 0;
	m_wndPiecesBar.m_wndPiecePreview.m_bZoomPreview = (i & PIECEBAR_ZOOMPREVIEW) != 0;

	RECT rc;
	m_wndPiecesBar.GetClientRect(&rc);
	m_wndPiecesBar.PostMessage(WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
	PostMessage(WM_LC_UPDATE_LIST, 1, 0);

	return TRUE;
}

void CMainFrame::OnClose() 
{
	if (m_lpfnCloseProc != NULL && !(*m_lpfnCloseProc)(this))
		return;

	if (!project->SaveModified())
		return;

	if (GetStyle() & WS_VISIBLE)
	{
		// save window size and position when destroyed
		WINDOWPLACEMENT wp;
		char szBuf[60];

		if (m_pwndFullScrnBar)
		{
			m_pwndFullScrnBar->DestroyWindow();
			delete m_pwndFullScrnBar;
			m_wndStatusBar.ShowWindow(SW_SHOWNORMAL);
			wp = m_wpPrev;
		}
		else
		{
			wp.length = sizeof(wp);
			GetWindowPlacement(&wp);
		}

		wsprintf(szBuf,"%d, %d, %d, %d", wp.rcNormalPosition.top, wp.rcNormalPosition.right, 
			wp.rcNormalPosition.bottom, wp.rcNormalPosition.left);
		theApp.WriteProfileString("Settings","Window Position", szBuf);
		theApp.WriteProfileInt("Settings", "Window Status", wp.showCmd);

		SaveBarState("Toolbars");
		theApp.WriteProfileInt(_T("Settings"), _T("ToolBarVersion"), TOOLBAR_VERSION);
	}

	AfxGetApp()->HideApplication();
	GetActiveDocument()->OnCloseDocument();
	AfxPostQuitMessage(0);
}

void CMainFrame::OnSetFocus(CWnd* pOldWnd) 
{
	CFrameWnd::OnSetFocus(pOldWnd);
	
	if (m_wndStatusBar.m_pPopup)
		m_wndStatusBar.m_pPopup->DestroyWindow();
}

void CMainFrame::OnPieceBar(UINT nID)
{
	switch (nID)
	{
		case ID_PIECEBAR_ZOOMPREVIEW:
		{
			m_wndPiecesBar.m_wndPiecePreview.m_bZoomPreview = !m_wndPiecesBar.m_wndPiecePreview.m_bZoomPreview;	
			m_wndPiecesBar.m_wndPiecePreview.PostMessage(WM_PAINT);
		} break;
		case ID_PIECEBAR_GROUP:
		{
			m_wndPiecesBar.m_bGroups = !m_wndPiecesBar.m_bGroups;
			m_wndPiecesBar.m_wndPiecesList.UpdateList();
		} break;
		case ID_PIECEBAR_PREVIEW:
		{
			m_wndPiecesBar.m_bPreview = !m_wndPiecesBar.m_bPreview;
		} break;
		case ID_PIECEBAR_NUMBERS:
		{
			m_wndPiecesBar.m_bNumbers = !m_wndPiecesBar.m_bNumbers;
		} break;
		case ID_PIECEBAR_COMBOBOX:
		{
			m_wndPiecesBar.m_bCombo = !m_wndPiecesBar.m_bCombo;
		} break;
		case ID_PIECEBAR_SUBPARTS:
		{
			m_wndPiecesBar.m_bSubParts = !m_wndPiecesBar.m_bSubParts;
			m_wndPiecesBar.m_wndPiecesList.UpdateList();
		} break;
	}

	if ((nID != ID_PIECEBAR_ZOOMPREVIEW) && (nID != ID_PIECEBAR_SUBPARTS))
	{
		RECT rc;
		m_wndPiecesBar.GetClientRect(&rc);
		m_wndPiecesBar.PostMessage(WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
		
		if (nID == ID_PIECEBAR_NUMBERS)
			PostMessage(WM_LC_UPDATE_LIST, 1, 0);
	}

	UINT u = 0;
	if (m_wndPiecesBar.m_bPreview) u |= PIECEBAR_PREVIEW;
	if (m_wndPiecesBar.m_bSubParts) u |= PIECEBAR_SUBPARTS;
	if (m_wndPiecesBar.m_bGroups) u |= PIECEBAR_GROUP;
	if (m_wndPiecesBar.m_bCombo) u |= PIECEBAR_COMBO;
	if (m_wndPiecesBar.m_bNumbers) u |= PIECEBAR_PARTNUMBERS;
	if (m_wndPiecesBar.m_wndPiecePreview.m_bZoomPreview) u |= PIECEBAR_ZOOMPREVIEW;
	theApp.WriteProfileInt("Settings", "Piecebar Options", u);
}

void CMainFrame::OnUpdatePieceBar(CCmdUI* pCmdUI)
{
	switch (pCmdUI->m_nID)
	{
		case ID_PIECEBAR_ZOOMPREVIEW:
		{
			if (m_wndPiecesBar.m_bPreview)
				pCmdUI->SetCheck(m_wndPiecesBar.m_wndPiecePreview.m_bZoomPreview);
			else
				pCmdUI->Enable(FALSE);
		} break;
		case ID_PIECEBAR_GROUP:
			pCmdUI->SetCheck(m_wndPiecesBar.m_bGroups); break;
		case ID_PIECEBAR_PREVIEW:
			pCmdUI->SetCheck(m_wndPiecesBar.m_bPreview); break;
		case ID_PIECEBAR_NUMBERS:
			pCmdUI->SetCheck(m_wndPiecesBar.m_bNumbers); break;
		case ID_PIECEBAR_COMBOBOX:
			pCmdUI->SetCheck(m_wndPiecesBar.m_bCombo); break;
		case ID_PIECEBAR_SUBPARTS:
			pCmdUI->SetCheck(m_wndPiecesBar.m_bSubParts); break;
	}
}

void CMainFrame::OnViewFullscreen() 
{
	RECT rectDesktop;
	WINDOWPLACEMENT wpNew;
	
	if (m_pwndFullScrnBar == NULL)
	{
		m_wndStatusBar.ShowWindow(SW_HIDE);
		GetWindowPlacement (&m_wpPrev);
		m_wpPrev.length = sizeof m_wpPrev;
		
		//Adjust RECT to new size of window
		::GetWindowRect (::GetDesktopWindow(), &rectDesktop);
		::AdjustWindowRectEx(&rectDesktop, GetStyle(), TRUE, GetExStyle());

		// Remember this for OnGetMinMaxInfo()
		m_FullScreenWindowRect = rectDesktop;
		
		wpNew = m_wpPrev;
		wpNew.showCmd =  SW_SHOWNORMAL;
		wpNew.rcNormalPosition = rectDesktop;
		
		m_pwndFullScrnBar = new CToolBar;
		
		if(!m_pwndFullScrnBar->Create(this,CBRS_SIZE_DYNAMIC|CBRS_FLOATING)
			|| !m_pwndFullScrnBar->LoadToolBar(IDR_FULLSCREEN))
		{
			TRACE0("Failed to create toolbar\n");
			return; 	 // fail to create
		}
		
		//don't allow the toolbar to dock
		m_pwndFullScrnBar->EnableDocking(0);
		m_pwndFullScrnBar->SetWindowPos(0,30,30,
			0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_SHOWWINDOW);
		m_pwndFullScrnBar->SetWindowText(_T("Full Screen"));
		m_pwndFullScrnBar->GetToolBarCtrl().CheckButton(ID_VIEW_FULLSCREEN, TRUE);
		FloatControlBar(m_pwndFullScrnBar, CPoint(30,30));
	}
	else
	{
		m_pwndFullScrnBar->DestroyWindow();
		delete m_pwndFullScrnBar;
		m_pwndFullScrnBar = NULL;
		m_wndStatusBar.ShowWindow(SW_SHOWNORMAL);
		wpNew = m_wpPrev;
	}

	SetWindowPlacement (&wpNew);
}

void CMainFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	if (m_pwndFullScrnBar != NULL)
	{
		lpMMI->ptMaxSize.y = m_FullScreenWindowRect.Height();
		lpMMI->ptMaxTrackSize.y = lpMMI->ptMaxSize.y;
		lpMMI->ptMaxSize.x = m_FullScreenWindowRect.Width();
		lpMMI->ptMaxTrackSize.x = lpMMI->ptMaxSize.x;
	}
	else
		CFrameWnd::OnGetMinMaxInfo(lpMMI);
}

void CMainFrame::GetMessageString(UINT nID, CString& rMessage) const
{
	if (nID >= ID_CAMERA_FIRST && nID <= ID_CAMERA_LAST)
	{
		Camera* pCamera = project->GetCamera(nID-ID_CAMERA_FIRST);
		rMessage = "Use the camera \"";
		rMessage += pCamera->GetName();
		rMessage += "\"";
	}
	else
		CFrameWnd::GetMessageString(nID, rMessage);
}

void CMainFrame::OnFilePrintPieceList() 
{
	AfxBeginThread(PrintPiecesFunction, this);
}

// Pass all commands to the project.
BOOL CMainFrame::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	int nID = LOWORD(wParam);

	if (nID >= ID_SNAP_12 && nID <= ID_SNAP_100)
	{
		project->HandleCommand(LC_TOOLBAR_SNAPMOVEMENU, nID - ID_SNAP_12);
		return TRUE;
	}

	if (nID >= ID_CAMERA_FIRST && nID <= ID_CAMERA_LAST)
	{
		project->HandleCommand(LC_VIEW_CAMERA_MENU, nID - ID_CAMERA_FIRST);
		return TRUE;
	}
	
	if (nID >= ID_FILE_MRU_FILE1 && nID <= ID_FILE_MRU_FILE4)
	{
		project->HandleCommand(LC_FILE_RECENT, nID - ID_FILE_MRU_FILE1);
		return TRUE;
	}

	if (nID >= ID_VIEWPORT01 && nID <= ID_VIEWPORT14)
	{
		project->HandleCommand(LC_VIEW_VIEWPORTS, nID - ID_VIEWPORT01);
		return TRUE;
	}

	if (nID >= ID_ACTION_SELECT && nID <= ID_ACTION_ROLL)
	{
		project->SetAction(nID - ID_ACTION_SELECT);
		return TRUE;
	}

	switch (nID)
	{
		case ID_FILE_NEW: {
			project->HandleCommand(LC_FILE_NEW, 0);
		} break;

		case ID_FILE_OPEN: {
			project->HandleCommand(LC_FILE_OPEN, 0);
		} break;

		case ID_FILE_MERGE: {
			project->HandleCommand(LC_FILE_MERGE, 0);
		} break;

		case ID_FILE_SAVE: {
			project->HandleCommand(LC_FILE_SAVE, 0);
		} break;

		case ID_FILE_SAVE_AS: {
			project->HandleCommand(LC_FILE_SAVEAS, 0);
		} break;

		case ID_FILE_SAVEPICTURE: {
			project->HandleCommand(LC_FILE_PICTURE, 0);
		} break;

		case ID_FILE_EXPORT_3DSTUDIO: {
			project->HandleCommand(LC_FILE_3DS, 0);
		} break;

		case ID_FILE_EXPORT_HTML: {
			project->HandleCommand(LC_FILE_HTML, 0);
		} break;

		case ID_FILE_EXPORT_POVRAY: {
			project->HandleCommand(LC_FILE_POVRAY, 0);
		} break;

		case ID_FILE_EXPORT_WAVEFRONT: {
			project->HandleCommand(LC_FILE_WAVEFRONT, 0);
		} break;

		case ID_FILE_PROPERTIES: {
			project->HandleCommand(LC_FILE_PROPERTIES, 0);
		} break;

		case ID_FILE_TERRAINEDITOR: {
			project->HandleCommand(LC_FILE_TERRAIN, 0);
		} break;

		case ID_FILE_EDITPIECELIBRARY: {
			project->HandleCommand(LC_FILE_LIBRARY, 0);
		} break;

		case ID_EDIT_REDO: {
			project->HandleCommand(LC_EDIT_REDO, 0);
		} break;

		case ID_EDIT_UNDO: {
			project->HandleCommand(LC_EDIT_UNDO, 0);
		} break;

		case ID_EDIT_CUT: {
			project->HandleCommand(LC_EDIT_CUT, 0);
		} break;
		
		case ID_EDIT_COPY: {
			project->HandleCommand(LC_EDIT_COPY, 0);
		} break;

		case ID_EDIT_PASTE: {
			project->HandleCommand(LC_EDIT_PASTE, 0);
		} break;

		case ID_EDIT_SELECTALL: {
			project->HandleCommand(LC_EDIT_SELECT_ALL, 0);
		} break;

		case ID_EDIT_SELECTNONE: {
			project->HandleCommand(LC_EDIT_SELECT_NONE, 0);
		} break;
		
		case ID_EDIT_SELECTINVERT: {
			project->HandleCommand(LC_EDIT_SELECT_INVERT, 0);
		} break;
		
		case ID_EDIT_SELECTBYNAME: {
			project->HandleCommand(LC_EDIT_SELECT_BYNAME, 0);
		} break;

		case ID_PIECE_INSERT: {
			project->HandleCommand(LC_PIECE_INSERT, 0);
		} break;

		case ID_PIECE_DELETE: {
			project->HandleCommand(LC_PIECE_DELETE, 0);
		} break;

		case ID_PIECE_MINIFIGWIZARD: {
			project->HandleCommand(LC_PIECE_MINIFIG, 0);
		} break;

		case ID_PIECE_ARRAY: {
			project->HandleCommand(LC_PIECE_ARRAY, 0);
		} break;

		case ID_PIECE_COPYKEYS: {
			project->HandleCommand(LC_PIECE_COPYKEYS, 0);
		} break;

		case ID_PIECE_GROUP: {
			project->HandleCommand(LC_PIECE_GROUP, 0);
		} break;
		
		case ID_PIECE_UNGROUP: {
			project->HandleCommand(LC_PIECE_UNGROUP, 0);
		} break;
		
		case ID_PIECE_ATTACH: {
			project->HandleCommand(LC_PIECE_GROUP_ADD, 0);
		} break;

		case ID_PIECE_DETACH: {
			project->HandleCommand(LC_PIECE_GROUP_REMOVE, 0);
		} break;

		case ID_PIECE_EDITGROUPS: {
			project->HandleCommand(LC_PIECE_GROUP_EDIT, 0);
		} break;

		case ID_PIECE_HIDESELECTED: {
			project->HandleCommand(LC_PIECE_HIDE_SELECTED, 0);
		} break;

		case ID_PIECE_HIDEUNSELECTED: {
			project->HandleCommand(LC_PIECE_HIDE_UNSELECTED, 0);
		} break;
		
		case ID_PIECE_UNHIDEALL: {
			project->HandleCommand(LC_PIECE_UNHIDE_ALL, 0);
		} break;

		case ID_PIECE_PREVIOUS: {
			project->HandleCommand(LC_PIECE_PREVIOUS, 0);
		} break;

		case ID_PIECE_NEXT: {
			project->HandleCommand(LC_PIECE_NEXT, 0);
		} break;

		case ID_VIEW_PREFERENCES: {
			project->HandleCommand(LC_VIEW_PREFERENCES, 0);
		} break;

		case ID_VIEW_ZOOMIN: {
			project->HandleCommand(LC_VIEW_ZOOMIN, 0);
		} break;

		case ID_VIEW_ZOOMOUT: {
			project->HandleCommand(LC_VIEW_ZOOMOUT, 0);
		} break;

		case ID_ZOOM_EXTENTS: {
			project->HandleCommand(LC_VIEW_ZOOMEXTENTS, 0);
		} break;

		case ID_VIEW_STEP_NEXT: {
			project->HandleCommand(LC_VIEW_STEP_NEXT, 0);
		} break;

		case ID_VIEW_STEP_PREVIOUS: {
			project->HandleCommand(LC_VIEW_STEP_PREVIOUS, 0);
		} break;

		case ID_VIEW_STEP_FIRST: {
			project->HandleCommand(LC_VIEW_STEP_FIRST, 0);
		} break;

		case ID_VIEW_STEP_LAST: {
			project->HandleCommand(LC_VIEW_STEP_LAST, 0);
		} break;

		case ID_VIEW_STEP_CHOOSE: {
			project->HandleCommand(LC_VIEW_STEP_CHOOSE, 0);
		} break;

		case ID_ANIMATOR_STOP: {
			project->HandleCommand(LC_VIEW_STOP, 0);
		} break;

		case ID_ANIMATOR_PLAY: {
			project->HandleCommand(LC_VIEW_PLAY, 0);
		} break;

		case ID_VIEW_CAMERAS_RESET: {
			project->HandleCommand(LC_VIEW_CAMERA_RESET, 0);
		} break;

		case ID_ANIMATOR_KEY: {
			project->HandleCommand(LC_TOOLBAR_ADDKEYS, 0);
		} break;

		case ID_ANIMATOR_TOGGLE: {
			project->HandleCommand(LC_TOOLBAR_ANIMATION, 0);
		} break;

		case ID_RENDER_BOX: {
			project->HandleCommand(LC_TOOLBAR_FASTRENDER, 0);
		} break;

		case ID_RENDER_BACKGROUND: {
			project->HandleCommand(LC_TOOLBAR_BACKGROUND, 0);
		} break;

		case ID_SNAP_SNAPX: {
			project->HandleCommand(LC_TOOLBAR_SNAPMENU, 0);
		} break;

		case ID_SNAP_SNAPY: {
			project->HandleCommand(LC_TOOLBAR_SNAPMENU, 1);
		} break;

		case ID_SNAP_SNAPZ: {
			project->HandleCommand(LC_TOOLBAR_SNAPMENU, 2);
		} break;

		case ID_SNAP_ON:
		case ID_SNAP_SNAPALL: {
			project->HandleCommand(LC_TOOLBAR_SNAPMENU, 3);
		} break;

		case ID_SNAP_SNAPNONE: {
			project->HandleCommand(LC_TOOLBAR_SNAPMENU, 4);
		} break;

		case ID_LOCK_LOCKX: {
			project->HandleCommand(LC_TOOLBAR_LOCKMENU, 0);
		} break;

		case ID_LOCK_LOCKY: {
			project->HandleCommand(LC_TOOLBAR_LOCKMENU, 1);
		} break;

		case ID_LOCK_LOCKZ: {
			project->HandleCommand(LC_TOOLBAR_LOCKMENU, 2);
		} break;

		case ID_LOCK_UNLOCKALL: {
			project->HandleCommand(LC_TOOLBAR_LOCKMENU, 3);
		} break;

		case ID_LOCK_2BUTTONS: {
			project->HandleCommand(LC_TOOLBAR_LOCKMENU, 4);
		} break;

		case ID_LOCK_3DMOVEMENT: {
			project->HandleCommand(LC_TOOLBAR_LOCKMENU, 5);
		} break;

		case ID_SNAP_ANGLE: {
			project->HandleCommand(LC_TOOLBAR_SNAPMENU, 5);
		} break;

		case ID_APP_ABOUT: {
			project->HandleCommand(LC_HELP_ABOUT, 0);
		} break;

		default: return CFrameWnd::OnCommand(wParam, lParam);
	}

	return TRUE;
}

void CMainFrame::OnActivateApp(BOOL bActive, HTASK hTask) 
{
	CFrameWnd::OnActivateApp(bActive, hTask);
	
	project->HandleNotify(LC_ACTIVATE, bActive ? 1 : 0);
}
