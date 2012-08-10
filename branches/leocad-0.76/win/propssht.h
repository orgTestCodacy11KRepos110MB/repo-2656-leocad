// PropsSht.h : header file
//
// This class defines custom modal property sheet 
// CPropertiesSheet.
 
#ifndef __PROPSSHT_H__
#define __PROPSSHT_H__

#include "PropsPgs.h"

/////////////////////////////////////////////////////////////////////////////
// CPropertiesSheet

class CPropertiesSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CPropertiesSheet)

// Construction
public:
	CPropertiesSheet(bool ShowPieces, CWnd* pWndParent = NULL);

// Attributes
public:
	CPropertiesSummary m_PageSummary;
	CPropertiesScene m_PageScene;
	CPropertiesPieces m_PagePieces;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropertiesSheet)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPropertiesSheet();

// Generated message map functions
protected:
	//{{AFX_MSG(CPropertiesSheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif	// __PROPSSHT_H__