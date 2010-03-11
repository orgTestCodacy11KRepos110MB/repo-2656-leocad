// PrefPage.cpp : implementation file
//

#include "lc_global.h"
#include "resource.h"
#include "PrefPage.h"
#include "prefsht.h"
#include "Tools.h"
#include "MainFrm.h"
#include "defines.h"
#include "keyboard.h"
#include "lc_application.h"
#include "library.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CPreferencesGeneral, CPropertyPage)
IMPLEMENT_DYNCREATE(CPreferencesDetail, CPropertyPage)
IMPLEMENT_DYNCREATE(CPreferencesDrawing, CPropertyPage)
IMPLEMENT_DYNCREATE(CPreferencesColors, CPropertyPage)
IMPLEMENT_DYNCREATE(CPreferencesPrint, CPropertyPage)
IMPLEMENT_DYNCREATE(CPreferencesKeyboard, CPropertyPage)

/////////////////////////////////////////////////////////////////////////////
// CPreferencesGeneral property page

CPreferencesGeneral::CPreferencesGeneral() : CPropertyPage(CPreferencesGeneral::IDD)
, m_strLibrary(_T(""))
, m_strColor(_T(""))
, m_Updates(0)
{
	//{{AFX_DATA_INIT(CPreferencesGeneral)
	m_bSubparts = FALSE;
	m_nSaveTime = 0;
	m_bNumbers = FALSE;
	m_strFolder = _T("");
	m_bAutoSave = FALSE;
	m_strUser = _T("");
	//}}AFX_DATA_INIT
}

CPreferencesGeneral::~CPreferencesGeneral()
{
}

void CPreferencesGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencesGeneral)
	DDX_Control(pDX, IDC_GENDLG_MOUSE, m_ctlMouse);
	DDX_Check(pDX, IDC_GENDLG_SUBPARTS, m_bSubparts);
	DDX_Text(pDX, IDC_GENDLG_SAVETIME, m_nSaveTime);
	DDV_MinMaxInt(pDX, m_nSaveTime, 1, 60);
	DDX_Check(pDX, IDC_GENDLG_NUMBERS, m_bNumbers);
	DDX_Text(pDX, IDC_GENDLG_FOLDER, m_strFolder);
	DDX_Check(pDX, IDC_GENDLG_AUTOSAVE, m_bAutoSave);
	DDX_Text(pDX, IDC_GENDLG_USER, m_strUser);
	DDV_MaxChars(pDX, m_strUser, 100);
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_GENDLG_LIBRARY, m_strLibrary);
	DDX_Text(pDX, IDC_GENDLG_COLOR, m_strColor);
	DDX_CBIndex(pDX, IDC_GENDLG_UPDATES, m_Updates);
}


BEGIN_MESSAGE_MAP(CPreferencesGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CPreferencesGeneral)
	ON_BN_CLICKED(IDC_GENDLG_FOLDERBTN, OnFolderBrowse)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_GENDLG_LIBRARY_BROWSE, &CPreferencesGeneral::OnBnClickedGendlgLibraryBrowse)
	ON_BN_CLICKED(IDC_GENDLG_COLOR_BROWSE, &CPreferencesGeneral::OnBnClickedColorBrowse)
END_MESSAGE_MAP()

void CPreferencesGeneral::OnFolderBrowse() 
{
	CString str;
	UpdateData(TRUE);
	if (FolderBrowse(&str, _T("Select default folder"), GetSafeHwnd()))
	{
		m_strFolder = str;
		UpdateData(FALSE);
	}
}

void CPreferencesGeneral::OnBnClickedGendlgLibraryBrowse()
{
	UpdateData(TRUE);
	CString str = m_strLibrary;
	if (FolderBrowse(&str, _T("Select library folder"), GetSafeHwnd()))
	{
		m_strLibrary = str;
		UpdateData(FALSE);
	}
}

void CPreferencesGeneral::OnBnClickedColorBrowse()
{
    UpdateData(TRUE);  

	CFileDialog dlg(TRUE, "*.ldr", m_strColor, 0, "Color Config Files (*.ldr)|*.ldr||", this);  

	if (dlg.DoModal() == IDOK)  
	{
		m_strColor = dlg.GetPathName();  
		UpdateData(FALSE);  

		CPreferencesSheet* Parent = (CPreferencesSheet*)GetParent();
		Parent->m_PageColors.LoadColorConfig(m_strColor);
	}  
}

void CPreferencesGeneral::SetOptions(int nSaveInterval, int nMouse, const char* strFolder, const char* strUser)
{
	m_nSaveTime = nSaveInterval & ~LC_AUTOSAVE_FLAG;
	m_bAutoSave = (nSaveInterval & LC_AUTOSAVE_FLAG) != 0;
	m_nMouse = nMouse;
	m_strFolder = strFolder;
	m_strUser = strUser;

	m_Updates = AfxGetApp()->GetProfileInt("Settings", "CheckUpdates", 1);
	int i = AfxGetApp()->GetProfileInt("Settings", "Piecebar Options", 0);
	m_bSubparts = (i & PIECEBAR_SUBPARTS) != 0;
	m_bNumbers = (i & PIECEBAR_PARTNUMBERS) != 0;
	m_strLibrary = AfxGetApp()->GetProfileString("Settings", "PiecesLibrary", "");
	m_strColor = AfxGetApp()->GetProfileString("Settings", "ColorConfig", "");
}

void CPreferencesGeneral::GetOptions(int* nSaveTime, int* nMouse, char* strFolder, char* strUser)
{
	if (m_bAutoSave) m_nSaveTime |= LC_AUTOSAVE_FLAG;
	*nSaveTime = m_nSaveTime;
	*nMouse = m_nMouse;
	strcpy(strFolder, m_strFolder);
	strcpy(strUser, m_strUser);

	int i = 0;
	if (m_bSubparts) i |= PIECEBAR_SUBPARTS;
	if (m_bNumbers) i |= PIECEBAR_PARTNUMBERS;
		
	AfxGetApp()->WriteProfileInt("Settings", "Piecebar Options", i);
	AfxGetApp()->WriteProfileInt("Settings", "CheckUpdates", m_Updates);
	AfxGetApp()->WriteProfileString("Settings", "PiecesLibrary", m_strLibrary);
	AfxGetApp()->WriteProfileString("Settings", "ColorConfig", m_strColor);
}

BOOL CPreferencesGeneral::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	m_ctlMouse.SetRange(1,20);
	m_ctlMouse.SetPos(m_nMouse);
	
	return TRUE;
}

void CPreferencesGeneral::OnOK() 
{
	m_nMouse = m_ctlMouse.GetPos();
	CPropertyPage::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CPreferencesDetail property page

CPreferencesDetail::CPreferencesDetail() : CPropertyPage(CPreferencesDetail::IDD)
{
	//{{AFX_DATA_INIT(CPreferencesDetail)
	m_bAntialiasing = FALSE;
	m_bEdges = FALSE;
	m_bLighting = FALSE;
	m_bSmooth = FALSE;
	m_fLineWidth = 0.0f;
	m_bFast = FALSE;
	m_DisableExt = FALSE;
	//}}AFX_DATA_INIT
}

CPreferencesDetail::~CPreferencesDetail()
{
}

void CPreferencesDetail::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencesDetail)
	DDX_Check(pDX, IDC_DETDLG_ANTIALIAS, m_bAntialiasing);
	DDX_Check(pDX, IDC_DETDLG_EDGES, m_bEdges);
	DDX_Check(pDX, IDC_DETDLG_LIGHTING, m_bLighting);
	DDX_Check(pDX, IDC_DETDLG_SMOOTH, m_bSmooth);
	DDX_Text(pDX, IDC_DETDLG_LINE, m_fLineWidth);
	DDV_MinMaxFloat(pDX, m_fLineWidth, 0.f, 10.f);
	DDX_Check(pDX, IDC_DETDLG_FAST, m_bFast);
	DDX_Check(pDX, IDC_DETDLG_DISABLEEXT, m_DisableExt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPreferencesDetail, CPropertyPage)
	//{{AFX_MSG_MAP(CPreferencesDetail)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CPreferencesDetail::SetOptions(DWORD dwDetail, float fLine)
{
	m_bAntialiasing = (dwDetail & LC_DET_ANTIALIAS) != 0;
	m_bEdges = (dwDetail & LC_DET_BRICKEDGES) != 0;
	m_bLighting = (dwDetail & LC_DET_LIGHTING) != 0;
	m_bSmooth =	(dwDetail & LC_DET_SMOOTH) != 0;
	m_bFast = (dwDetail & LC_DET_FAST) != 0;
	m_fLineWidth = fLine;

	m_DisableExt = AfxGetApp()->GetProfileInt("Settings", "DisableGLExtensions", 0);
}

void CPreferencesDetail::GetOptions(DWORD* dwDetail, float* fLine)
{
	*dwDetail = 0;
	if (m_bAntialiasing) *dwDetail |= LC_DET_ANTIALIAS;
	if (m_bEdges) *dwDetail |= LC_DET_BRICKEDGES;
	if (m_bLighting) *dwDetail |= LC_DET_LIGHTING;
	if (m_bSmooth) *dwDetail |= LC_DET_SMOOTH;
	if (m_bFast) *dwDetail |= LC_DET_FAST;
	*fLine = m_fLineWidth;

	AfxGetApp()->WriteProfileInt("Settings", "DisableGLExtensions", m_DisableExt);
}

/////////////////////////////////////////////////////////////////////////////
// CPreferencesDrawing property page

CPreferencesDrawing::CPreferencesDrawing() : CPropertyPage(CPreferencesDrawing::IDD)
{
	//{{AFX_DATA_INIT(CPreferencesDrawing)
	m_nAngle = 0;
	m_bAxis = FALSE;
	m_bCentimeters = FALSE;
	m_bFixed = FALSE;
	m_bGrid = FALSE;
	m_bLockX = FALSE;
	m_bLockY = FALSE;
	m_bLockZ = FALSE;
	m_bMove = FALSE;
	m_bSnapA = FALSE;
	m_bSnapX = FALSE;
	m_bSnapY = FALSE;
	m_bSnapZ = FALSE;
	m_bGlobal = FALSE;
	//}}AFX_DATA_INIT
}

CPreferencesDrawing::~CPreferencesDrawing()
{
}

void CPreferencesDrawing::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencesDrawing)
	DDX_Text(pDX, IDC_AIDDLG_ANGLE, m_nAngle);
	DDV_MinMaxInt(pDX, m_nAngle, 1, 180);
	DDX_Check(pDX, IDC_AIDDLG_AXIS, m_bAxis);
	DDX_Check(pDX, IDC_AIDDLG_CENTIMETERS, m_bCentimeters);
	DDX_Check(pDX, IDC_AIDDLG_FIXEDKEYS, m_bFixed);
	DDX_Check(pDX, IDC_AIDDLG_GRID, m_bGrid);
	DDX_Check(pDX, IDC_AIDDLG_LOCKX, m_bLockX);
	DDX_Check(pDX, IDC_AIDDLG_LOCKY, m_bLockY);
	DDX_Check(pDX, IDC_AIDDLG_LOCKZ, m_bLockZ);
	DDX_Check(pDX, IDC_AIDDLG_MOVE, m_bMove);
	DDX_Check(pDX, IDC_AIDDLG_SNAPA, m_bSnapA);
	DDX_Check(pDX, IDC_AIDDLG_SNAPX, m_bSnapX);
	DDX_Check(pDX, IDC_AIDDLG_SNAPY, m_bSnapY);
	DDX_Check(pDX, IDC_AIDDLG_SNAPZ, m_bSnapZ);
	DDX_Check(pDX, IDC_AIDDLG_GLOBAL, m_bGlobal);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPreferencesDrawing, CPropertyPage)
	//{{AFX_MSG_MAP(CPreferencesDrawing)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CPreferencesDrawing::SetOptions(unsigned long dwSnap, unsigned short nAngle)
{
	m_nAngle = nAngle;

	m_bAxis = (dwSnap & LC_DRAW_AXIS) != 0;
	m_bCentimeters = (dwSnap & LC_DRAW_CM_UNITS) != 0;
	m_bFixed = (dwSnap & LC_DRAW_MOVEAXIS) != 0;
	m_bGrid = (dwSnap & LC_DRAW_GRID) != 0;
	m_bLockX = (dwSnap & LC_DRAW_LOCK_X) != 0;
	m_bLockY = (dwSnap & LC_DRAW_LOCK_Y) != 0;
	m_bLockZ = (dwSnap & LC_DRAW_LOCK_Z) != 0;
	m_bMove = (dwSnap & LC_DRAW_MOVE) != 0;
	m_bSnapA = (dwSnap & LC_DRAW_SNAP_A) != 0;
	m_bSnapX = (dwSnap & LC_DRAW_SNAP_X) != 0;
	m_bSnapY = (dwSnap & LC_DRAW_SNAP_Y) != 0;
	m_bSnapZ = (dwSnap & LC_DRAW_SNAP_Z) != 0;
	m_bGlobal = (dwSnap & LC_DRAW_GLOBAL_SNAP) != 0;
}

void CPreferencesDrawing::GetOptions(unsigned long* dwSnap, unsigned short* nAngle)
{
	*nAngle = m_nAngle;

	*dwSnap = 0;
	if (m_bAxis) *dwSnap |= LC_DRAW_AXIS;
	if (m_bCentimeters) *dwSnap |= LC_DRAW_CM_UNITS;
	if (m_bFixed) *dwSnap |= LC_DRAW_MOVEAXIS;
	if (m_bGrid) *dwSnap |= LC_DRAW_GRID;
	if (m_bLockX) *dwSnap |= LC_DRAW_LOCK_X;
	if (m_bLockY) *dwSnap |= LC_DRAW_LOCK_Y;
	if (m_bLockZ) *dwSnap |= LC_DRAW_LOCK_Z;
	if (m_bMove) *dwSnap |= LC_DRAW_MOVE;
	if (m_bSnapA) *dwSnap |= LC_DRAW_SNAP_A;
	if (m_bSnapX) *dwSnap |= LC_DRAW_SNAP_X;
	if (m_bSnapY) *dwSnap |= LC_DRAW_SNAP_Y;
	if (m_bSnapZ) *dwSnap |= LC_DRAW_SNAP_Z;
	if (m_bGlobal) *dwSnap |= LC_DRAW_GLOBAL_SNAP;
}

/////////////////////////////////////////////////////////////////////////////
// CPreferencesColors property page

CPreferencesColors::CPreferencesColors() : CPropertyPage(CPreferencesColors::IDD)
, m_TabName(_T(""))
{
	//{{AFX_DATA_INIT(CPreferencesColors)
	//}}AFX_DATA_INIT
}

CPreferencesColors::~CPreferencesColors()
{
}

void CPreferencesColors::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencesColors)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_CLRDLG_AVAILABLE, m_AvailableList);
	DDX_Control(pDX, IDC_CLRDLG_CURRENT, m_CurrentList);
	DDX_Control(pDX, IDC_CLRDLG_TABLIST, m_TabList);
	DDX_Text(pDX, IDC_CLRDLG_TABNAME, m_TabName);
	DDV_MaxChars(pDX, m_TabName, 255);
}


BEGIN_MESSAGE_MAP(CPreferencesColors, CPropertyPage)
	//{{AFX_MSG_MAP(CPreferencesColors)
	//}}AFX_MSG_MAP
	ON_LBN_SELCHANGE(IDC_CLRDLG_TABLIST, &CPreferencesColors::OnLbnSelchangeTabList)
	ON_BN_CLICKED(IDC_CLRDLG_ADDCOLOR, &CPreferencesColors::OnBnClickedAddColor)
	ON_BN_CLICKED(IDC_CLRDLG_REMOVECOLOR, &CPreferencesColors::OnBnClickedRemoveColor)
	ON_BN_CLICKED(IDC_CLRDLG_UPCOLOR, &CPreferencesColors::OnBnClickedUpColor)
	ON_BN_CLICKED(IDC_CLRDLG_DOWNCOLOR, &CPreferencesColors::OnBnClickedDownColor)
	ON_LBN_SELCHANGE(IDC_CLRDLG_AVAILABLE, &CPreferencesColors::OnLbnSelchangeAvailable)
	ON_LBN_SELCHANGE(IDC_CLRDLG_CURRENT, &CPreferencesColors::OnLbnSelchangeCurrent)
	ON_BN_CLICKED(IDC_CLRDLG_UPTAB, &CPreferencesColors::OnBnClickedUpTab)
	ON_BN_CLICKED(IDC_CLRDLG_DOWNTAB, &CPreferencesColors::OnBnClickedDownTab)
	ON_BN_CLICKED(IDC_CLRDLG_DELETETAB, &CPreferencesColors::OnBnClickedDeleteTab)
	ON_BN_CLICKED(IDC_CLRDLG_NEWTAB, &CPreferencesColors::OnBnClickedNewTab)
	ON_BN_CLICKED(IDC_CLRDLG_RENAMETAB, &CPreferencesColors::OnBnClickedRenameTab)
	ON_BN_CLICKED(IDC_CLRDLG_RESET, &CPreferencesColors::OnBnClickedReset)
END_MESSAGE_MAP()

BOOL CPreferencesColors::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	UpdateTabs();
	m_TabList.SetCurSel(0);
	UpdateTabControls();
	UpdateColors();

	return TRUE;
}

void CPreferencesColors::LoadColorConfig(const CString& Path)
{
	lcFileDisk File;

	if (File.Open(Path, "rt"))
		m_ColorConfig.LoadColors(File);

	if (m_ColorConfig.mColors.GetSize() < 5)
		m_ColorConfig.LoadDefaultColors();

	m_ColorConfig.LoadConfig();

	if (m_ColorConfig.mColorGroups.GetSize() == 0)
		m_ColorConfig.LoadDefaultConfig();

	if (IsWindow(m_hWnd))
	{
		UpdateTabs();
		m_TabList.SetCurSel(0);
		UpdateTabControls();
		UpdateColors();
	}
}

void CPreferencesColors::OnBnClickedReset()
{
	if (MessageBox("Are you sure you want to reset the color groups?", "LeoCAD", MB_YESNO|MB_ICONQUESTION) == IDNO)
		return;

	m_ColorConfig.LoadDefaultColors();
	m_ColorConfig.LoadDefaultConfig();

	UpdateTabs();
	m_TabList.SetCurSel(0);
	UpdateTabControls();
	UpdateColors();
}

void CPreferencesColors::UpdateTabs()
{
	m_TabList.ResetContent();
	for (int i = 0; i < m_ColorConfig.mColorGroups.GetSize(); i++)
		m_TabList.AddString(m_ColorConfig.mColorGroups[i].Name);
}

void CPreferencesColors::UpdateTabControls()
{
	int Sel = m_TabList.GetCurSel();
	int Count = m_TabList.GetCount();

	if (Sel == LB_ERR)
		return;

	GetDlgItem(IDC_CLRDLG_UPTAB)->EnableWindow(Sel != 0 && Count > 1);
	GetDlgItem(IDC_CLRDLG_DOWNTAB)->EnableWindow(Sel != Count - 1 && Count > 1);
	GetDlgItem(IDC_CLRDLG_DELETETAB)->EnableWindow(Count > 1);
}

void CPreferencesColors::UpdateColors()
{
	m_AvailableList.ResetContent();
	m_CurrentList.ResetContent();

	int Sel = m_TabList.GetCurSel();

	if (Sel == LB_ERR)
		return;

	UpdateData(TRUE);
	m_TabName = m_ColorConfig.mColorGroups[Sel].Name;
	UpdateData(FALSE);

	lcColorGroup& Group = m_ColorConfig.mColorGroups[Sel];

	for (int i = 0; i < Group.Colors.GetSize(); i++)
	{
		int Idx = m_CurrentList.AddString(m_ColorConfig.mColors[Group.Colors[i]].Name);
		m_CurrentList.SetItemData(Idx, Group.Colors[i]);
	}

	for (int i = 0; i < m_ColorConfig.GetNumUserColors(); i++)
	{
		int j;

		for (j = 0; j < Group.Colors.GetSize(); j++)
			if (Group.Colors[j] == i)
				break;

		if (j == Group.Colors.GetSize())
		{
			int Idx = m_AvailableList.AddString(m_ColorConfig.mColors[i].Name);
			m_AvailableList.SetItemData(Idx, i);
		}
	}

	UpdateColorControls();
}

void CPreferencesColors::UpdateColorControls()
{
	int Available = m_AvailableList.GetSelCount();
	int Current = m_CurrentList.GetSelCount();
	int CurCount = m_CurrentList.GetCount();
	int CurSel = -1;

	if (Current == 1)
		m_CurrentList.GetSelItems(1, &CurSel);

	GetDlgItem(IDC_CLRDLG_ADDCOLOR)->EnableWindow(Available > 0);
	GetDlgItem(IDC_CLRDLG_REMOVECOLOR)->EnableWindow(Current > 0);
	GetDlgItem(IDC_CLRDLG_UPCOLOR)->EnableWindow(Current == 1 && CurSel != 0 && CurCount > 1);
	GetDlgItem(IDC_CLRDLG_DOWNCOLOR)->EnableWindow(Current == 1 && CurSel != CurCount - 1 && CurCount > 1);
}

void CPreferencesColors::OnBnClickedUpTab()
{
	int Sel = m_TabList.GetCurSel();
	if (Sel == LB_ERR || Sel == 0)
		return;

	lcColorGroup Group;
	Group = m_ColorConfig.mColorGroups[Sel];
	m_ColorConfig.mColorGroups[Sel] = m_ColorConfig.mColorGroups[Sel - 1];
	m_ColorConfig.mColorGroups[Sel - 1] = Group;

	UpdateTabs();
	m_TabList.SetCurSel(Sel - 1);
	UpdateTabControls();
}

void CPreferencesColors::OnBnClickedDownTab()
{
	int Sel = m_TabList.GetCurSel();
	if (Sel == LB_ERR || Sel == m_TabList.GetCount())
		return;

	lcColorGroup Group;
	Group = m_ColorConfig.mColorGroups[Sel];
	m_ColorConfig.mColorGroups[Sel] = m_ColorConfig.mColorGroups[Sel + 1];
	m_ColorConfig.mColorGroups[Sel + 1] = Group;

	UpdateTabs();
	m_TabList.SetCurSel(Sel + 1);
	UpdateTabControls();
}

void CPreferencesColors::OnBnClickedDeleteTab()
{
	int Sel = m_TabList.GetCurSel();

	if (Sel == LB_ERR)
		return;

	m_ColorConfig.mColorGroups.RemoveIndex(Sel);

	UpdateTabs();
	m_TabList.SetCurSel(0);
	UpdateTabControls();
	UpdateColors();
}

void CPreferencesColors::OnBnClickedNewTab()
{
	UpdateData(TRUE);

	lcColorGroup Group;
	Group.Name = "New Tab";
	m_ColorConfig.mColorGroups.Add(Group);

	UpdateTabs();
	m_TabList.SetCurSel(m_TabList.GetCount() - 1);
	UpdateTabControls();
	UpdateColors();

	CEdit* NameCtrl = (CEdit*)GetDlgItem(IDC_CLRDLG_TABNAME);
	NameCtrl->SetFocus();
	NameCtrl->SetSel(0, -1, FALSE);
}

void CPreferencesColors::OnBnClickedRenameTab()
{
	int Sel = m_TabList.GetCurSel();

	if (Sel == LB_ERR)
		return;

	UpdateData(TRUE);
	m_ColorConfig.mColorGroups[Sel].Name = m_TabName;

	UpdateTabs();
	m_TabList.SetCurSel(Sel);
}

void CPreferencesColors::OnLbnSelchangeTabList()
{
	UpdateTabControls();
	UpdateColors();
}

void CPreferencesColors::OnLbnSelchangeAvailable()
{
	UpdateColorControls();
}

void CPreferencesColors::OnLbnSelchangeCurrent()
{
	UpdateColorControls();
}

void CPreferencesColors::OnBnClickedAddColor()
{
	int Count = m_AvailableList.GetSelCount();
	int* Sel = new int[Count];

	m_AvailableList.GetSelItems(Count, Sel);

	int GroupIdx = m_TabList.GetCurSel();
	if (GroupIdx == LB_ERR)
		return;

	lcColorGroup& Group = m_ColorConfig.mColorGroups[GroupIdx];

	for (int i = 0; i < Count; i++)
	{
		int Color = m_AvailableList.GetItemData(Sel[i]);
		Group.Colors.Add(Color);
	}

	delete[] Sel;

	int Top = m_AvailableList.GetTopIndex();
	UpdateColors();
	m_AvailableList.SetTopIndex(Top);
}

void CPreferencesColors::OnBnClickedRemoveColor()
{
	int Count = m_CurrentList.GetSelCount();
	int* Sel = new int[Count];

	m_CurrentList.GetSelItems(Count, Sel);

	int GroupIdx = m_TabList.GetCurSel();
	if (GroupIdx == LB_ERR)
		return;

	lcColorGroup& Group = m_ColorConfig.mColorGroups[GroupIdx];

	for (int i = 0; i < Count; i++)
	{
		int Color = m_CurrentList.GetItemData(Sel[i]);
		for (int j = 0; j < Group.Colors.GetSize(); j++)
		{
			if (Color == Group.Colors[j])
			{
				Group.Colors.RemoveIndex(j);
				break;
			}
		}
	}

	delete[] Sel;

	int Top = m_CurrentList.GetTopIndex();
	UpdateColors();
	m_CurrentList.SetTopIndex(Top);
}

void CPreferencesColors::OnBnClickedUpColor()
{
	int GroupIdx = m_TabList.GetCurSel();
	if (GroupIdx == LB_ERR)
		return;

	int Count = m_CurrentList.GetSelCount();
	if (Count != 1)
		return;

	int Sel;
	m_CurrentList.GetSelItems(1, &Sel);
	if (Sel == 0)
		return;

	int Color = m_CurrentList.GetItemData(Sel);
	lcColorGroup& Group = m_ColorConfig.mColorGroups[GroupIdx];

	for (int j = 0; j < Group.Colors.GetSize(); j++)
	{
		if (Color == Group.Colors[j])
		{
			m_CurrentList.DeleteString(Sel-1);
			int Idx = m_CurrentList.InsertString(Sel, m_ColorConfig.mColors[Group.Colors[j-1]].Name);
			m_CurrentList.SetItemData(Idx, Group.Colors[j-1]);

			Group.Colors[j] = Group.Colors[j-1];
			Group.Colors[j-1] = Color;
			break;
		}
	}

	UpdateColorControls();
}

void CPreferencesColors::OnBnClickedDownColor()
{
	int GroupIdx = m_TabList.GetCurSel();
	if (GroupIdx == LB_ERR)
		return;

	int Count = m_CurrentList.GetSelCount();
	if (Count != 1)
		return;

	int Sel;
	m_CurrentList.GetSelItems(1, &Sel);
	if (Sel == m_CurrentList.GetCount())
		return;

	int Color = m_CurrentList.GetItemData(Sel);
	lcColorGroup& Group = m_ColorConfig.mColorGroups[GroupIdx];

	for (int j = 0; j < Group.Colors.GetSize(); j++)
	{
		if (Color == Group.Colors[j])
		{
			m_CurrentList.DeleteString(Sel+1);
			int Idx = m_CurrentList.InsertString(Sel, m_ColorConfig.mColors[Group.Colors[j+1]].Name);
			m_CurrentList.SetItemData(Idx, Group.Colors[j+1]);

			Group.Colors[j] = Group.Colors[j+1];
			Group.Colors[j+1] = Color;
			break;
		}
	}

	UpdateColorControls();
}

void CPreferencesColors::SetOptions()
{
	m_ColorConfig = g_App->m_ColorConfig;
}

void CPreferencesColors::GetOptions()
{
	if (m_ColorConfig != g_App->m_ColorConfig)
	{
		g_App->SetColorConfig(m_ColorConfig);
		((CMainFrame*)AfxGetMainWnd())->m_wndPiecesBar.m_wndColorsList.UpdateColorConfig();
		m_ColorConfig.SaveConfig();
	}
}


/////////////////////////////////////////////////////////////////////////////
// CPreferencesPrint property page

CPreferencesPrint::CPreferencesPrint() : CPropertyPage(CPreferencesPrint::IDD)
{
	//{{AFX_DATA_INIT(CPreferencesPrint)
	m_fBottom = 0.0f;
	m_fLeft = 0.0f;
	m_fRight = 0.0f;
	m_fTop = 0.0f;
	m_bNumbers = FALSE;
	m_strInstHeader = _T("");
	m_strInstFooter = _T("");
	m_bBorder = FALSE;
	m_nInstCols = 0;
	m_nInstRows = 0;
	m_nCatCols = 0;
	m_nCatRows = 0;
	m_strCatHeader = _T("");
	m_strCatFooter = _T("");
	m_bCatNames = FALSE;
	m_nCatOrientation = 0;
	//}}AFX_DATA_INIT
}

CPreferencesPrint::~CPreferencesPrint()
{
}

void CPreferencesPrint::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencesPrint)
	DDX_Text(pDX, IDC_PRNDLG_MARGIN_BOTTOM, m_fBottom);
	DDX_Text(pDX, IDC_PRNDLG_MARGIN_LEFT, m_fLeft);
	DDX_Text(pDX, IDC_PRNDLG_MARGIN_RIGHT, m_fRight);
	DDX_Text(pDX, IDC_PRNDLG_MARGIN_TOP, m_fTop);
	DDX_Check(pDX, IDC_PRNDLG_NUMBERS, m_bNumbers);
	DDX_Text(pDX, IDC_PRNDLG_HEADER, m_strInstHeader);
	DDX_Text(pDX, IDC_PRNDLG_FOOTER, m_strInstFooter);
	DDX_Check(pDX, IDC_PRNDLG_BORDER, m_bBorder);
	DDX_Text(pDX, IDC_PRNDLG_INST_COLS, m_nInstCols);
	DDV_MinMaxInt(pDX, m_nInstCols, 1, 20);
	DDX_Text(pDX, IDC_PRNDLG_INST_ROWS, m_nInstRows);
	DDV_MinMaxInt(pDX, m_nInstRows, 1, 30);
	DDX_Text(pDX, IDC_PRNDLG_CAT_COLS, m_nCatCols);
	DDV_MinMaxInt(pDX, m_nCatCols, 1, 40);
	DDX_Text(pDX, IDC_PRNDLG_CAT_ROWS, m_nCatRows);
	DDV_MinMaxInt(pDX, m_nCatRows, 1, 50);
	DDX_Text(pDX, IDC_PRNDLG_CAT_HEADER, m_strCatHeader);
	DDX_Text(pDX, IDC_PRNDLG_CAT_FOOTER, m_strCatFooter);
	DDX_Check(pDX, IDC_PRNDLG_CAT_NAMES, m_bCatNames);
	DDX_Radio(pDX, IDC_PRNDLG_CAT_HORIZONTAL, m_nCatOrientation);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPreferencesPrint, CPropertyPage)
	//{{AFX_MSG_MAP(CPreferencesPrint)
	ON_BN_CLICKED(IDC_PRNDLG_FOOTERBTN, OnFooterButton)
	ON_BN_CLICKED(IDC_PRNDLG_HEADERBTN, OnFooterButton)
	ON_BN_CLICKED(IDC_PRNDLG_CAT_FOOTERBTN, OnFooterButton)
	ON_BN_CLICKED(IDC_PRNDLG_CAT_HEADERBTN, OnFooterButton)
	//}}AFX_MSG_MAP
	ON_COMMAND_RANGE(ID_PRINT_FILENAME, ID_PRINT_RIGHTALIGN, OnHeaderClick)
END_MESSAGE_MAP()


static HBITMAP CreateArrow()
{
	HWND hwndDesktop = GetDesktopWindow(); 
	HDC hdcDesktop = GetDC(hwndDesktop); 
	HDC hdcMem = CreateCompatibleDC(hdcDesktop); 
	HBRUSH hbr = CreateSolidBrush(GetSysColor(COLOR_BTNFACE)); 
	HBITMAP hbm = CreateCompatibleBitmap(hdcDesktop, 4, 7);
	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbm); 

	// Paint the bitmap
	FillRect(hdcMem, CRect(0, 0, 4, 7), hbr);
	COLORREF c = GetSysColor(COLOR_BTNTEXT);

	for (int x = 0; x < 4; x++)
	for (int y = x; y < 7-x; y++)
		SetPixel(hdcMem, x, y, c);

	// Clean up
	SelectObject(hdcMem, hbmOld); 
	DeleteObject(hbr); 
	DeleteDC(hdcMem); 
	ReleaseDC(hwndDesktop, hdcDesktop); 
	return hbm;
}

BOOL CPreferencesPrint::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	SendDlgItemMessage(IDC_PRNDLG_HEADERBTN, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)CreateArrow());
	SendDlgItemMessage(IDC_PRNDLG_FOOTERBTN, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)CreateArrow());
	SendDlgItemMessage(IDC_PRNDLG_CAT_HEADERBTN, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)CreateArrow());
	SendDlgItemMessage(IDC_PRNDLG_CAT_FOOTERBTN, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)CreateArrow());
	
	return TRUE;
}

void CPreferencesPrint::OnFooterButton() 
{
	CMenu menu;
	CMenu* pPopup;
	RECT rc;

	if (SendDlgItemMessage(IDC_PRNDLG_HEADERBTN, BM_GETSTATE, 0, 0) & BST_FOCUS)
		::GetWindowRect(::GetDlgItem(m_hWnd, IDC_PRNDLG_HEADERBTN), &rc);
	else if (SendDlgItemMessage(IDC_PRNDLG_FOOTERBTN, BM_GETSTATE, 0, 0) & BST_FOCUS)
		::GetWindowRect(::GetDlgItem(m_hWnd, IDC_PRNDLG_FOOTERBTN), &rc);
	else if (SendDlgItemMessage(IDC_PRNDLG_CAT_HEADERBTN, BM_GETSTATE, 0, 0) & BST_FOCUS)
		::GetWindowRect(::GetDlgItem(m_hWnd, IDC_PRNDLG_CAT_HEADERBTN), &rc);
	else
		::GetWindowRect(::GetDlgItem(m_hWnd, IDC_PRNDLG_CAT_FOOTERBTN), &rc);

	menu.LoadMenu(IDR_POPUPS);
	pPopup = menu.GetSubMenu(3);
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, rc.right, rc.top, this);
}

void CPreferencesPrint::OnHeaderClick(UINT nID)
{
	char c[3] = { '&', 0, 0 };
	switch (nID)
	{
	case ID_PRINT_FILENAME:		c[1] = 'F'; break;
	case ID_PRINT_PAGENUMBER:	c[1] = 'P'; break;
	case ID_PRINT_TOTALPAGES:	c[1] = 'O'; break;
	case ID_PRINT_CURRENTTIME:	c[1] = 'T'; break;
	case ID_PRINT_CURRENTDATE:	c[1] = 'D'; break;
	case ID_PRINT_LEFTALIGN:	c[1] = 'L'; break;
	case ID_PRINT_CENTER:		c[1] = 'C'; break;
	case ID_PRINT_RIGHTALIGN:	c[1] = 'R'; break;
	case ID_PRINT_AUTHOR:		c[1] = 'A'; break;
	case ID_PRINT_DESCRIPTION:	c[1] = 'N'; break;
	}

	UINT CtrlID;
	if (SendDlgItemMessage(IDC_PRNDLG_HEADERBTN, BM_GETSTATE, 0, 0) & BST_FOCUS)
		CtrlID = IDC_PRNDLG_HEADER;
	else if (SendDlgItemMessage(IDC_PRNDLG_FOOTERBTN, BM_GETSTATE, 0, 0) & BST_FOCUS)
		CtrlID = IDC_PRNDLG_FOOTER;
	else if (SendDlgItemMessage(IDC_PRNDLG_CAT_HEADERBTN, BM_GETSTATE, 0, 0) & BST_FOCUS)
		CtrlID = IDC_PRNDLG_CAT_HEADER;
	else
		CtrlID = IDC_PRNDLG_CAT_FOOTER;

	SendDlgItemMessage(CtrlID, EM_REPLACESEL, TRUE, (LPARAM)&c);
	::SetFocus(::GetDlgItem(m_hWnd, CtrlID));
}

void CPreferencesPrint::SetOptions(CString strHeader, CString strFooter)
{
	DWORD dwPrint = AfxGetApp()->GetProfileInt("Settings", "Print", PRINT_NUMBERS|PRINT_BORDER|PRINT_NAMES);
	m_bNumbers = (dwPrint & PRINT_NUMBERS) != 0;
	m_bBorder = (dwPrint & PRINT_BORDER) != 0;
	m_bCatNames = (dwPrint & PRINT_NAMES) != 0;
	m_nCatOrientation = (dwPrint & PRINT_HORIZONTAL) ? 0 : 1;

	m_strInstHeader = strHeader;
	m_strInstFooter = strFooter;

	m_fLeft = (float)AfxGetApp()->GetProfileInt("Default", "Margin Left", 50)/100;
	m_fTop = (float)AfxGetApp()->GetProfileInt("Default", "Margin Top", 50)/100;
	m_fRight = (float)AfxGetApp()->GetProfileInt("Default", "Margin Right", 50)/100; 
	m_fBottom = (float)AfxGetApp()->GetProfileInt("Default", "Margin Bottom", 50)/100;
	m_nInstRows = AfxGetApp()->GetProfileInt("Default", "Print Rows", 1);
	m_nInstCols = AfxGetApp()->GetProfileInt("Default", "Print Columns", 1);
	m_nCatRows = AfxGetApp()->GetProfileInt("Default", "Catalog Rows", 10);
	m_nCatCols = AfxGetApp()->GetProfileInt("Default", "Catalog Columns", 3);
	m_strCatHeader = AfxGetApp()->GetProfileString("Default", "Catalog Header", "");
	m_strCatFooter = AfxGetApp()->GetProfileString("Default", "Catalog Footer", "Page &P");
}

void CPreferencesPrint::GetOptions(char* strHeader, char* strFooter)
{
	DWORD dwPrint = 0;
	if (m_bNumbers) dwPrint |= PRINT_NUMBERS;
	if (m_bBorder) dwPrint |= PRINT_BORDER;
	if (m_bCatNames) dwPrint |= PRINT_NAMES;
	if (m_nCatOrientation == 0) dwPrint |= PRINT_HORIZONTAL;
	AfxGetApp()->WriteProfileInt("Settings","Print", dwPrint);

	strcpy(strHeader, (LPCSTR)m_strInstHeader);
	strcpy(strFooter, (LPCSTR)m_strInstFooter);

	AfxGetApp()->WriteProfileInt("Default", "Margin Left", (int)(m_fLeft*100));
	AfxGetApp()->WriteProfileInt("Default", "Margin Top", (int)(m_fTop*100));
	AfxGetApp()->WriteProfileInt("Default", "Margin Right", (int)(m_fRight*100)); 
	AfxGetApp()->WriteProfileInt("Default", "Margin Bottom", (int)(m_fBottom*100));
	AfxGetApp()->WriteProfileInt("Default", "Print Rows", m_nInstRows);
	AfxGetApp()->WriteProfileInt("Default", "Print Columns", m_nInstCols);
	AfxGetApp()->WriteProfileInt("Default", "Catalog Rows", m_nCatRows);
	AfxGetApp()->WriteProfileInt("Default", "Catalog Columns", m_nCatCols);
	AfxGetApp()->WriteProfileString("Default", "Catalog Header", m_strCatHeader);
	AfxGetApp()->WriteProfileString("Default", "Catalog Footer", m_strCatFooter);
}

/////////////////////////////////////////////////////////////////////////////
// CPreferencesKeyboard property page

CPreferencesKeyboard::CPreferencesKeyboard() : CPropertyPage(CPreferencesKeyboard::IDD)
{
	//{{AFX_DATA_INIT(CPreferencesKeyboard)
	m_strFileName = _T("");
	//}}AFX_DATA_INIT
}

CPreferencesKeyboard::~CPreferencesKeyboard()
{
}

void CPreferencesKeyboard::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencesKeyboard)
	DDX_Control(pDX, IDC_KEYDLG_KEYEDIT, m_Edit);
	DDX_Control(pDX, IDC_KEYDLG_ASSIGN, m_Assign);
	DDX_Control(pDX, IDC_KEYDLG_REMOVE, m_Remove);
	DDX_Control(pDX, IDC_KEYDLG_CMDLIST, m_List);
	DDX_Control(pDX, IDC_KEYDLG_COMBO, m_Combo);
	DDX_Text(pDX, IDC_KEYDLG_FILENAME, m_strFileName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPreferencesKeyboard, CPropertyPage)
	//{{AFX_MSG_MAP(CPreferencesKeyboard)
	ON_BN_CLICKED(IDC_KEYDLG_REMOVE, OnKeydlgRemove)
	ON_BN_CLICKED(IDC_KEYDLG_ASSIGN, OnKeydlgAssign)
	ON_BN_CLICKED(IDC_KEYDLG_RESET, OnKeydlgReset)
	ON_LBN_SELCHANGE(IDC_KEYDLG_CMDLIST, OnSelchangeKeydlgCmdlist)
	ON_EN_CHANGE(IDC_KEYDLG_KEYEDIT, OnChangeKeydlgKeyedit)
	ON_BN_CLICKED(IDC_KEYDLG_SAVE, OnKeydlgSave)
	ON_BN_CLICKED(IDC_KEYDLG_LOAD, OnKeydlgLoad)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CPreferencesKeyboard::SetOptions()
{
	m_strFileName = AfxGetApp()->GetProfileString("Settings", "Keyboard", "");
}

void CPreferencesKeyboard::GetOptions()
{
	if (m_strFileName.GetLength())
	{
		if (!SaveKeyboardShortcuts(m_strFileName))
		{
			m_strFileName = "";
			AfxMessageBox("Error saving Keyboard Shortcuts file.", MB_OK | MB_ICONEXCLAMATION);
		}
	}

	AfxGetApp()->WriteProfileString("Settings", "Keyboard", m_strFileName);

  ((CMainFrame*)AfxGetMainWnd())->UpdateMenuAccelerators();
}

BOOL CPreferencesKeyboard::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// Fill the list with all commands available.
	for (int i = 0; i < KeyboardShortcutsCount; i++)
	{
		int Index = m_List.AddString(KeyboardShortcuts[i].Description);
		m_List.SetItemData(Index, i);
	}

	m_List.SetCurSel(0);
	OnSelchangeKeydlgCmdlist();

	return TRUE;
}

void CPreferencesKeyboard::OnKeydlgRemove() 
{
	int Sel = m_List.GetCurSel();

	if (Sel == LB_ERR)
		return;

	LC_KEYBOARD_COMMAND& Cmd = KeyboardShortcuts[m_List.GetItemData(Sel)];

	Sel = m_Combo.GetCurSel();

	if (Sel == CB_ERR)
		return;

	if (Sel == 0)
	{
		Cmd.Flags = LC_KEYMOD_2TO1(Cmd.Flags);
		Cmd.Key1 = Cmd.Key2;
		Cmd.Key2 = 0;
	}
	else
	{
		Cmd.Key2 = 0;
		Cmd.Flags &= ~LC_KEYMOD2_MASK;
	}

	OnSelchangeKeydlgCmdlist();
}

void CPreferencesKeyboard::OnKeydlgAssign() 
{
	int Sel = m_List.GetCurSel();

	if (Sel == LB_ERR)
		return;

	// Check if this shortcut is not already assigned to someone else.
	for (int i = 0; i < KeyboardShortcutsCount; i++)
	{
		LC_KEYBOARD_COMMAND& Cmd = KeyboardShortcuts[i];
		int Match = 0;

		if (Cmd.Key1 == m_Edit.m_Key)
		{
			if ((((Cmd.Flags & LC_KEYMOD1_SHIFT) != 0) == m_Edit.m_Shift) &&
			    (((Cmd.Flags & LC_KEYMOD1_CONTROL) != 0) == m_Edit.m_Control))
			{
				Match = 1;
			}
		}

		if (Cmd.Key2 == m_Edit.m_Key)
		{
			if ((((Cmd.Flags & LC_KEYMOD2_SHIFT) != 0) == m_Edit.m_Shift) &&
			    (((Cmd.Flags & LC_KEYMOD2_CONTROL) != 0) == m_Edit.m_Control))
			{
				Match = 2;
			}
		}

		if (Match)
		{
			CString Msg;

			Msg.Format("This shortcut is currently assigned to \"%s\", do you want to reassign it?",
			           Cmd.Description);

			if (AfxMessageBox(Msg, MB_YESNO | MB_ICONQUESTION) == IDNO)
			{
				return;
			}
			else
			{
				// Remove old shortcut.
				if (Match == 1)
				{
					Cmd.Flags = LC_KEYMOD_2TO1(Cmd.Flags);
					Cmd.Key1 = Cmd.Key2;
					Cmd.Key2 = 0;
				}
				else
				{
					Cmd.Key2 = 0;
					Cmd.Flags &= ~LC_KEYMOD2_MASK;
				}
			}
		}
	}

	LC_KEYBOARD_COMMAND& Cmd = KeyboardShortcuts[m_List.GetItemData(Sel)];

	// Assign new shortcut.
	if (Cmd.Key1 == 0)
	{
		Cmd.Key1 = m_Edit.m_Key;

		if (m_Edit.m_Shift)
			Cmd.Flags |= LC_KEYMOD1_SHIFT;

		if (m_Edit.m_Control)
			Cmd.Flags |= LC_KEYMOD1_CONTROL;
	}
	else
	{
		Cmd.Key2 = m_Edit.m_Key;

		if (m_Edit.m_Shift)
			Cmd.Flags |= LC_KEYMOD2_SHIFT;

		if (m_Edit.m_Control)
			Cmd.Flags |= LC_KEYMOD2_CONTROL;
	}

	m_Edit.ResetKey();
	OnSelchangeKeydlgCmdlist();
}

void CPreferencesKeyboard::OnSelchangeKeydlgCmdlist() 
{
	m_Combo.ResetContent();
	m_Remove.EnableWindow(false);
	m_Edit.SetWindowText("");

	int Sel = m_List.GetCurSel();

	if (Sel == LB_ERR)
		return;

	LC_KEYBOARD_COMMAND& Cmd = KeyboardShortcuts[m_List.GetItemData(Sel)];

	// Update the combo box with the shortcuts for the current selection.
	if (Cmd.Key1)
	{
		CString str;

		if (Cmd.Flags & LC_KEYMOD1_SHIFT)
			str = "Shift+";

		if (Cmd.Flags & LC_KEYMOD1_CONTROL)
			str += "Ctrl+";

		str += GetKeyName(Cmd.Key1);

		m_Combo.AddString(str);
		m_Combo.SetCurSel(0);
		m_Remove.EnableWindow(true);

		if (Cmd.Key2)
		{
			str = "";

			if (Cmd.Flags & LC_KEYMOD2_SHIFT)
				str = "Shift+";

			if (Cmd.Flags & LC_KEYMOD2_CONTROL)
				str += "Ctrl+";

			str += GetKeyName(Cmd.Key2);

			m_Combo.AddString(str);
		}
	}

	m_Assign.EnableWindow((Cmd.Key2 == 0) && m_Edit.m_Key);
}

void CPreferencesKeyboard::OnChangeKeydlgKeyedit() 
{
	if (m_Edit.m_Key == 0)
	{
		m_Assign.EnableWindow(false);
		return;
	}

	int Sel = m_List.GetCurSel();

	if (Sel == LB_ERR)
		return;

	LC_KEYBOARD_COMMAND& Cmd = KeyboardShortcuts[m_List.GetItemData(Sel)];

	if (Cmd.Key2 != 0)
	{
		m_Assign.EnableWindow(false);
		return;
	}

	m_Assign.EnableWindow(true);
}

void CPreferencesKeyboard::OnKeydlgReset() 
{
	if (AfxMessageBox("Are you sure you want to reset the Keyboard Shortcuts to the default settings?", MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		ResetKeyboardShortcuts();
		OnSelchangeKeydlgCmdlist();
		m_strFileName = "";
		UpdateData(FALSE);
	}
}

void CPreferencesKeyboard::OnKeydlgSave() 
{
	UpdateData(TRUE);

	CFileDialog dlg(FALSE, "*.lkb", m_strFileName, OFN_OVERWRITEPROMPT, "LeoCAD Keyboard Layout Files (*.lkb)|*.lkb||", this);

	if (dlg.DoModal() == IDOK)
	{
		if (SaveKeyboardShortcuts(dlg.GetPathName()))
		{
			m_strFileName = dlg.GetPathName();
			UpdateData(FALSE);
		}
		else
		{
			AfxMessageBox("Error saving file.", MB_OK | MB_ICONEXCLAMATION);
		}
	}
}

void CPreferencesKeyboard::OnKeydlgLoad() 
{
	CFileDialog dlg(TRUE, "*.lkb", m_strFileName, 0, "LeoCAD Keyboard Layout Files (*.lkb)|*.lkb||", this);

	if (dlg.DoModal() == IDOK)
	{
		if (LoadKeyboardShortcuts(dlg.GetPathName()))
		{
			UpdateData(TRUE);
			m_strFileName = dlg.GetPathName();
			UpdateData(FALSE);
		}
		else
		{
			AfxMessageBox("Error loading file.", MB_OK | MB_ICONEXCLAMATION);
		}
	}
}
