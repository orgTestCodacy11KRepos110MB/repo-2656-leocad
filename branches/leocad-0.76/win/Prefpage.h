// PrefPage.h : header file
//

#ifndef __PREFPAGE_H__
#define __PREFPAGE_H__

#include "keyedit.h"
#include "lc_colors.h"
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CPreferencesGeneral dialog

class CPreferencesGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(CPreferencesGeneral)

// Construction
public:
	void SetOptions(int nSaveInterval, int nMouse, const char* strFolder, const char* strUser);
	void GetOptions(int* nSaveTime, int* nMouse, char* strFolder, char* strUser);
	CPreferencesGeneral();
	~CPreferencesGeneral();

// Dialog Data
	//{{AFX_DATA(CPreferencesGeneral)
	enum { IDD = IDD_PREFGENERAL };
	CSliderCtrl	m_ctlMouse;
	BOOL	m_bSubparts;
	int		m_nSaveTime;
	BOOL	m_bNumbers;
	CString	m_strFolder;
	BOOL	m_bAutoSave;
	CString	m_bUser;
	CString	m_strUser;
	int		m_Updates;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesGeneral)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	int m_nMouse;

	// Generated message map functions
	//{{AFX_MSG(CPreferencesGeneral)
	afx_msg void OnFolderBrowse();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedGendlgLibraryBrowse();
	CString m_strLibrary;
	CString m_strColor;
	afx_msg void OnBnClickedColorBrowse();
};


/////////////////////////////////////////////////////////////////////////////
// CPreferencesDetail dialog

class CPreferencesDetail : public CPropertyPage
{
	DECLARE_DYNCREATE(CPreferencesDetail)

// Construction
public:
	void SetOptions(DWORD dwDetail, float fLine);
	void GetOptions(DWORD* dwDetail, float* fLine);
	CPreferencesDetail();
	~CPreferencesDetail();

// Dialog Data
	//{{AFX_DATA(CPreferencesDetail)
	enum { IDD = IDD_PREFDETAIL };
	BOOL	m_bAntialiasing;
	BOOL	m_bEdges;
	BOOL	m_bLighting;
	BOOL	m_bSmooth;
	float	m_fLineWidth;
	BOOL	m_bFast;
	BOOL	m_DisableExt;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesDetail)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPreferencesDetail)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CPreferencesDrawing dialog

class CPreferencesDrawing : public CPropertyPage
{
	DECLARE_DYNCREATE(CPreferencesDrawing)

// Construction
public:
	void SetOptions(unsigned long dwSnap, unsigned short nAngle);
	void GetOptions(unsigned long* dwSnap, unsigned short* nAngle);
	CPreferencesDrawing();
	~CPreferencesDrawing();

// Dialog Data
	//{{AFX_DATA(CPreferencesDrawing)
	enum { IDD = IDD_PREFDRAWING };
	int		m_nAngle;
	BOOL	m_bAxis;
	BOOL	m_bCentimeters;
	BOOL	m_bFixed;
	BOOL	m_bGrid;
	BOOL	m_bLockX;
	BOOL	m_bLockY;
	BOOL	m_bLockZ;
	BOOL	m_bMove;
	BOOL	m_bSnapA;
	BOOL	m_bSnapX;
	BOOL	m_bSnapY;
	BOOL	m_bSnapZ;
	BOOL	m_bGlobal;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesDrawing)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPreferencesDrawing)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////
// CPreferencesColors dialog

class CPreferencesColors : public CPropertyPage
{
	DECLARE_DYNCREATE(CPreferencesColors)

// Construction
public:
	void SetOptions();
	void GetOptions();
	CPreferencesColors();
	~CPreferencesColors();

	void LoadColorConfig(const CString& Path);

// Dialog Data
	//{{AFX_DATA(CPreferencesColors)
	enum { IDD = IDD_PREFCOLORS };
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesColors)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	lcColorConfig m_ColorConfig;

	void UpdateTabs();
	void UpdateTabControls();
	void UpdateColors();
	void UpdateColorControls();

	// Generated message map functions
	//{{AFX_MSG(CPreferencesColors)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
	CListBox m_AvailableList;
	CListBox m_CurrentList;
	CListBox m_TabList;
	afx_msg void OnLbnSelchangeTabList();
	afx_msg void OnBnClickedAddColor();
	afx_msg void OnBnClickedRemoveColor();
	afx_msg void OnBnClickedUpColor();
	afx_msg void OnBnClickedDownColor();
	afx_msg void OnLbnSelchangeAvailable();
	afx_msg void OnLbnSelchangeCurrent();
	afx_msg void OnBnClickedUpTab();
	afx_msg void OnBnClickedDownTab();
	afx_msg void OnBnClickedDeleteTab();
	afx_msg void OnBnClickedNewTab();
	afx_msg void OnBnClickedRenameTab();
	CString m_TabName;
	afx_msg void OnBnClickedImport();
	afx_msg void OnBnClickedReset();
};


/////////////////////////////////////////////////////////////////////////////
// CPreferencesPrint dialog

class CPreferencesPrint : public CPropertyPage
{
	DECLARE_DYNCREATE(CPreferencesPrint)

// Construction
public:
	void SetOptions(CString strHeader, CString strFooter);
	void GetOptions(char* strHeader, char* strFooter);
	CPreferencesPrint();
	~CPreferencesPrint();

// Dialog Data
	//{{AFX_DATA(CPreferencesPrint)
	enum { IDD = IDD_PREFPRINT };
	float	m_fBottom;
	float	m_fLeft;
	float	m_fRight;
	float	m_fTop;
	BOOL	m_bNumbers;
	CString	m_strInstHeader;
	CString	m_strInstFooter;
	BOOL	m_bBorder;
	int		m_nInstCols;
	int		m_nInstRows;
	int		m_nCatCols;
	int		m_nCatRows;
	CString	m_strCatHeader;
	CString	m_strCatFooter;
	BOOL	m_bCatNames;
	int		m_nCatOrientation;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesPrint)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void OnHeaderClick(UINT nID);
	// Generated message map functions
	//{{AFX_MSG(CPreferencesPrint)
	virtual BOOL OnInitDialog();
	afx_msg void OnFooterButton();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CPreferencesKeyboard dialog

class CPreferencesKeyboard : public CPropertyPage
{
	DECLARE_DYNCREATE(CPreferencesKeyboard)

// Construction
public:
	void SetOptions();
	void GetOptions();
	CPreferencesKeyboard();
	~CPreferencesKeyboard();

// Dialog Data
	//{{AFX_DATA(CPreferencesKeyboard)
	enum { IDD = IDD_PREFKEYBOARD };
	CKeyEdit	m_Edit;
	CButton	m_Assign;
	CButton	m_Remove;
	CListBox	m_List;
	CComboBox	m_Combo;
	CString	m_strFileName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesKeyboard)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPreferencesKeyboard)
	virtual BOOL OnInitDialog();
	afx_msg void OnKeydlgRemove();
	afx_msg void OnKeydlgAssign();
	afx_msg void OnKeydlgReset();
	afx_msg void OnSelchangeKeydlgCmdlist();
	afx_msg void OnChangeKeydlgKeyedit();
	afx_msg void OnKeydlgSave();
	afx_msg void OnKeydlgLoad();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

#endif // __PREFPAGE_H__