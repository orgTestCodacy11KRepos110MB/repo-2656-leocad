#include "lc_global.h"
#include "lc_math.h"
#include "lc_mesh.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <locale.h>
#include "opengl.h"
#include "pieceinf.h"
#include "lc_texture.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "group.h"
#include "terrain.h"
#include "project.h"
#include "image.h"
#include "system.h"
#include "minifig.h"
#include "curve.h"
#include "lc_mainwindow.h"
#include "view.h"
#include "lc_library.h"
#include "texfont.h"
#include "debug.h"
#include "lc_application.h"
#include "lc_profile.h"
#include "lc_model.h"
#include "lc_piece.h"
#include "lc_camera.h"
#include "lc_light.h"

/////////////////////////////////////////////////////////////////////////////
// Project construction/destruction

Project::Project()
{
	mActiveModel = new lcModel;
	mActiveModel->mProperties.mName = "Model 1";
	mModels.Add(mActiveModel);

	m_bModified = false;
	m_pPieces = NULL;
	m_pLights = NULL;
	m_pGroups = NULL;
	m_pUndoList = NULL;
	m_pRedoList = NULL;
	m_pTerrain = new Terrain();
	m_pBackground = new lcTexture();
	memset(&mSearchOptions, 0, sizeof(mSearchOptions));
}

Project::~Project()
{
	DeleteContents(false);

	delete m_pTerrain;
	delete m_pBackground;
}


/////////////////////////////////////////////////////////////////////////////
// Project attributes, general services

void Project::UpdateInterface()
{
	// Update all user interface elements.
	gMainWindow->UpdateTransformMode();
	gMainWindow->UpdateCurrentTool();
	gMainWindow->UpdateAddKeys();

	gMainWindow->UpdateUndoRedo(m_pUndoList->pNext ? m_pUndoList->strText : NULL, m_pRedoList ? m_pRedoList->strText : NULL);
	gMainWindow->UpdatePaste(g_App->mClipboard != NULL);
	SystemUpdatePlay(true, false);
	gMainWindow->UpdateCategories();
	gMainWindow->UpdateTitle(m_strTitle, m_bModified);

	gMainWindow->UpdateFocusObject();
	gMainWindow->UpdateLockSnap(m_nSnap);
	gMainWindow->UpdateSnap();
	gMainWindow->UpdateCameraMenu();
	gMainWindow->UpdateModelMenu();
	UpdateSelection();
	gMainWindow->UpdateCurrentTime();

	for (int i = 0; i < gMainWindow->mViews.GetSize(); i++)
	{
		gMainWindow->mViews[i]->MakeCurrent();
		RenderInitialize();
	}

	UpdateSelection();
}

void Project::SetTitle(const char* Title)
{
	strcpy(m_strTitle, Title);

	gMainWindow->UpdateTitle(m_strTitle, m_bModified);
}

void Project::DeleteContents(bool bUndo)
{
	Piece* pPiece;
	Light* pLight;
	Group* pGroup;

	memset(m_strAuthor, 0, sizeof(m_strAuthor));
	memset(m_strDescription, 0, sizeof(m_strDescription));
	memset(m_strComments, 0, sizeof(m_strComments));

	if (!bUndo)
	{
		LC_UNDOINFO* pUndo;

		while (m_pUndoList)
		{
			pUndo = m_pUndoList;
			m_pUndoList = m_pUndoList->pNext;
			delete pUndo;
		}

		while (m_pRedoList)
		{
			pUndo = m_pRedoList;
			m_pRedoList = m_pRedoList->pNext;
			delete pUndo;
		}

		m_pRedoList = NULL;
		m_pUndoList = NULL;

		m_pBackground->Unload();
		m_pTerrain->LoadDefaults(true);
	}

	while (m_pPieces)
	{
		pPiece = m_pPieces;
		m_pPieces = m_pPieces->m_pNext;
		delete pPiece;
	}

	if (!bUndo)
	{
		for (int ViewIdx = 0; ViewIdx < gMainWindow->mViews.GetSize(); ViewIdx++)
		{
			View* view = gMainWindow->mViews[ViewIdx];

			if (!view->mCamera->IsSimple())
				view->SetDefaultCamera();
		}
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		delete mCameras[CameraIdx];
	mCameras.RemoveAll();

	while (m_pLights)
	{
		pLight = m_pLights;
		m_pLights = m_pLights->m_pNext;
		delete pLight;
	}

	while (m_pGroups)
	{
		pGroup = m_pGroups;
		m_pGroups = m_pGroups->m_pNext;
		delete pGroup;
	}
}

// Only call after DeleteContents()
void Project::LoadDefaults(bool cameras)
{
	int i;

	gMainWindow->SetTransformMode(LC_TRANSFORM_RELATIVE_TRANSLATION);
	gMainWindow->SetCurrentTool(LC_TOOL_SELECT);
	gMainWindow->SetAddKeys(false);
	gMainWindow->SetColorIndex(lcGetColorIndex(4));

	// Default values
	m_bAnimation = false;
	m_bUndoOriginal = true;
	gMainWindow->UpdateUndoRedo(NULL, NULL);
	m_nAngleSnap = (unsigned short)lcGetProfileInt(LC_PROFILE_ANGLE_SNAP);
	m_nSnap = lcGetProfileInt(LC_PROFILE_SNAP);
	gMainWindow->UpdateLockSnap(m_nSnap);
	m_nMoveSnap = 0x0304;
	gMainWindow->UpdateSnap();
	m_nCurStep = 1;
	m_nTotalFrames = 100;
	gMainWindow->UpdateCurrentTime();
	strcpy(m_strHeader, "");
	strcpy(m_strFooter, "Page &P");
	strcpy(m_strBackground, lcGetProfileString(LC_PROFILE_DEFAULT_BACKGROUND_TEXTURE));
	m_pTerrain->LoadDefaults(true);

	for (i = 0; i < gMainWindow->mViews.GetSize (); i++)
	{
		gMainWindow->mViews[i]->MakeCurrent();
		RenderInitialize();
	}

	if (cameras)
	{
		for (i = 0; i < gMainWindow->mViews.GetSize(); i++)
			if (!gMainWindow->mViews[i]->mCamera->IsSimple())
				gMainWindow->mViews[i]->SetDefaultCamera();

		gMainWindow->UpdateCameraMenu();
	}

	SystemPieceComboAdd(NULL);
	UpdateSelection();
	gMainWindow->UpdateFocusObject();
}

/////////////////////////////////////////////////////////////////////////////
// Standard file menu commands

// Read a .lcd file
bool Project::FileLoad(lcFile* file, bool bUndo, bool bMerge)
{
	lcint32 i, count;
	char id[32];
	lcuint32 rgb;
	float fv = 0.4f;
	lcuint8 ch;
	lcuint16 sh;

	file->Seek(0, SEEK_SET);
	file->ReadBuffer(id, 32);
	sscanf(&id[7], "%f", &fv);

	// Fix the ugly floating point reading on computers with different decimal points.
	if (fv == 0.0f)
	{
		lconv *loc = localeconv();
		id[8] = loc->decimal_point[0];
		sscanf(&id[7], "%f", &fv);

		if (fv == 0.0f)
			return false;
	}

	if (fv > 0.4f)
		file->ReadFloats(&fv, 1);

	file->ReadU32(&rgb, 1);//m_fBackground
	if (!bMerge)
	{
//		m_fBackground[0] = (float)((unsigned char) (rgb))/255;
//		m_fBackground[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
//		m_fBackground[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	}

	if (fv < 0.6f) // old view
	{
		Camera* pCam = new Camera(false);
		pCam->CreateName(mCameras);
		mCameras.Add(pCam);

		double eye[3], target[3];
		file->ReadDoubles(eye, 3);
		file->ReadDoubles(target, 3);
		float tmp[3] = { (float)eye[0], (float)eye[1], (float)eye[2] };
		pCam->ChangeKey(1, false, false, tmp, LC_CK_EYE);
		pCam->ChangeKey(1, true, false, tmp, LC_CK_EYE);
		tmp[0] = (float)target[0]; tmp[1] = (float)target[1]; tmp[2] = (float)target[2];
		pCam->ChangeKey(1, false, false, tmp, LC_CK_TARGET);
		pCam->ChangeKey(1, true, false, tmp, LC_CK_TARGET);

		// Create up vector
		lcVector3 UpVector(0, 0, 1), FrontVector((float)(eye[0] - target[0]), (float)(eye[1] - target[1]), (float)(eye[2] - target[2])), SideVector;
		FrontVector.Normalize();
		if (FrontVector == UpVector)
			SideVector = lcVector3(1, 0, 0);
		else
			SideVector = lcCross(FrontVector, UpVector);
		UpVector = lcNormalize(lcCross(SideVector, FrontVector));
		pCam->ChangeKey(1, false, false, UpVector, LC_CK_UP);
		pCam->ChangeKey(1, true, false, UpVector, LC_CK_UP);
	}

	if (bMerge)
		file->Seek(32, SEEK_CUR);
	else
	{
		lcuint32 u;
		float f;
		file->ReadS32(&i, 1); m_nAngleSnap = i;
		file->ReadU32(&u, 1); //m_nSnap
		file->ReadFloats(&f, 1); //m_fLineWidth
		file->ReadU32(&u, 1); //m_nDetail
		file->ReadS32(&i, 1); //m_nCurGroup = i;
		file->ReadS32(&i, 1); //m_nCurColor = i;
		file->ReadS32(&i, 1); //action = i;
		file->ReadS32(&i, 1); m_nCurStep = i;
	}

	if (fv > 0.8f)
		file->ReadU32();//m_nScene

	file->ReadS32(&count, 1);
//	SystemStartProgressBar(0, count, 1, "Loading project...");
	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	Library->OpenCache();

	while (count--)
	{
		if (fv > 0.4f)
		{
			Piece* pPiece = new Piece(NULL);
			pPiece->FileLoad(*file);

			if (bMerge)
				for (Piece* p = m_pPieces; p; p = p->m_pNext)
					if (strcmp(p->GetName(), pPiece->GetName()) == 0)
					{
						pPiece->CreateName(m_pPieces);
						break;
					}

			if (strlen(pPiece->GetName()) == 0)
				pPiece->CreateName(m_pPieces);

			AddPiece(pPiece);
			if (!bUndo)
				SystemPieceComboAdd(pPiece->mPieceInfo->m_strDescription);
		}
		else
		{
			char name[LC_PIECE_NAME_LEN];
			float pos[3], rot[3];
			lcuint8 color, step, group;

			file->ReadFloats(pos, 3);
			file->ReadFloats(rot, 3);
			file->ReadU8(&color, 1);
			file->ReadBuffer(name, 9);
			file->ReadU8(&step, 1);
			file->ReadU8(&group, 1);

			PieceInfo* pInfo = Library->FindPiece(name, true);
			Piece* pPiece = new Piece(pInfo);

			pPiece->Initialize(pos[0], pos[1], pos[2], step, 1);
			pPiece->SetColorCode(lcGetColorCodeFromOriginalColor(color));
			pPiece->CreateName(m_pPieces);
			AddPiece(pPiece);

			lcMatrix44 ModelWorld = lcMul(lcMatrix44RotationZ(rot[2] * LC_DTOR), lcMul(lcMatrix44RotationY(rot[1] * LC_DTOR), lcMatrix44RotationX(rot[0] * LC_DTOR)));
			lcVector4 AxisAngle = lcMatrix44ToAxisAngle(ModelWorld);
			AxisAngle[3] *= LC_RTOD;

			pPiece->ChangeKey(1, false, false, AxisAngle, LC_PK_ROTATION);
			pPiece->ChangeKey(1, true, false, AxisAngle, LC_PK_ROTATION);
//			pPiece->SetGroup((Group*)group);
			SystemPieceComboAdd(pInfo->m_strDescription);
		}

//		SytemStepProgressBar();
	}

	Library->CloseCache();
//	SytemEndProgressBar();

	if (!bMerge)
	{
		if (fv >= 0.4f)
		{
			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF) file->ReadU16(&sh, 1); else sh = ch;
			if (sh > 100)
				file->Seek(sh, SEEK_CUR);
			else
				file->ReadBuffer(m_strAuthor, sh);

			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF) file->ReadU16(&sh, 1); else sh = ch;
			if (sh > 100)
				file->Seek(sh, SEEK_CUR);
			else
				file->ReadBuffer(m_strDescription, sh);

			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF && fv < 1.3f) file->ReadU16(&sh, 1); else sh = ch;
			if (sh > 255)
				file->Seek(sh, SEEK_CUR);
			else
				file->ReadBuffer(m_strComments, sh);
		}
	}
	else
	{
		if (fv >= 0.4f)
		{
			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF) file->ReadU16(&sh, 1); else sh = ch;
			file->Seek (sh, SEEK_CUR);

			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF) file->ReadU16(&sh, 1); else sh = ch;
			file->Seek (sh, SEEK_CUR);

			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF && fv < 1.3f) file->ReadU16(&sh, 1); else sh = ch;
			file->Seek (sh, SEEK_CUR);
		}
	}

	if (fv >= 0.5f)
	{
		file->ReadS32(&count, 1);

		Group* pGroup;
		Group* pLastGroup = NULL;
		for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
			pLastGroup = pGroup;

		pGroup = pLastGroup;
		for (i = 0; i < count; i++)
		{
			if (pGroup)
			{
				pGroup->m_pNext = new Group();
				pGroup = pGroup->m_pNext;
			}
			else
				m_pGroups = pGroup = new Group();
		}
		pLastGroup = pLastGroup ? pLastGroup->m_pNext : m_pGroups;

		for (pGroup = pLastGroup; pGroup; pGroup = pGroup->m_pNext)
		{
			if (fv < 1.0f)
			{
				file->ReadBuffer(pGroup->m_strName, 65);
				file->ReadBuffer(&ch, 1);
				pGroup->m_fCenter[0] = 0;
				pGroup->m_fCenter[1] = 0;
				pGroup->m_fCenter[2] = 0;
				pGroup->m_pGroup = (Group*)-1;
			}
			else
				pGroup->FileLoad(file);
		}

		for (pGroup = pLastGroup; pGroup; pGroup = pGroup->m_pNext)
		{
			i = LC_POINTER_TO_INT(pGroup->m_pGroup);
			pGroup->m_pGroup = NULL;

			if (i > 0xFFFF || i == -1)
				continue;

			for (Group* g2 = pLastGroup; g2; g2 = g2->m_pNext)
			{
				if (i == 0)
				{
					pGroup->m_pGroup = g2;
					break;
				}

				i--;
			}
		}


		Piece* pPiece;
		for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		{
			i = LC_POINTER_TO_INT(pPiece->GetGroup());
			pPiece->SetGroup(NULL);

			if (i > 0xFFFF || i == -1)
				continue;

			for (pGroup = pLastGroup; pGroup; pGroup = pGroup->m_pNext)
			{
				if (i == 0)
				{
					pPiece->SetGroup(pGroup);
					break;
				}

				i--;
			}
		}

		RemoveEmptyGroups();
	}

	if (!bMerge)
	{
		if (fv >= 0.6f)
		{
			if (fv < 1.0f)
			{
				lcuint32 ViewportMode;
				file->ReadU32(&ViewportMode, 1);
			}
			else
			{
				lcuint8 ViewportMode, ActiveViewport;
				file->ReadU8(&ViewportMode, 1);
				file->ReadU8(&ActiveViewport, 1);
			}

			file->ReadS32(&count, 1);
			for (i = 0; i < count; i++)
				mCameras.Add(new Camera(false));

			if (count < 7)
			{
				Camera* pCam = new Camera(false);
				for (i = 0; i < count; i++)
					pCam->FileLoad(*file);
				delete pCam;
			}
			else
			{
				for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
					mCameras[CameraIdx]->FileLoad(*file);
			}
		}

		if (fv >= 0.7f)
		{
			for (count = 0; count < 4; count++)
			{
				file->ReadS32(&i, 1);

//				Camera* pCam = m_pCameras;
//				while (i--)
//					pCam = pCam->m_pNext;
//				m_pViewCameras[count] = pCam;
			}

			file->ReadU32(&rgb, 1);
//			m_fFogColor[0] = (float)((unsigned char) (rgb))/255;
//			m_fFogColor[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
//			m_fFogColor[2] = (float)((unsigned char) ((rgb) >> 16))/255;

			float f;
			if (fv < 1.0f)
			{
				file->ReadU32(&rgb, 1);
//				m_fFogDensity = (float)rgb/100;
			}
			else
				file->ReadFloats(&f, 1);//m_fFogDensity

			if (fv < 1.3f)
			{
				file->ReadU8(&ch, 1);
				if (ch == 0xFF)
					file->ReadU16(&sh, 1);
				sh = ch;
			}
			else
				file->ReadU16(&sh, 1);

			if (sh < LC_MAXPATH)
				file->ReadBuffer(m_strBackground, sh);
			else
				file->Seek (sh, SEEK_CUR);
		}

		if (fv >= 0.8f)
		{
			file->ReadBuffer(&ch, 1);
			file->ReadBuffer(m_strHeader, ch);
			file->ReadBuffer(&ch, 1);
			file->ReadBuffer(m_strFooter, ch);
		}

		if (fv > 0.9f)
		{
			file->ReadU32(&rgb, 1);
//			m_fAmbient[0] = (float)((unsigned char) (rgb))/255;
//			m_fAmbient[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
//			m_fAmbient[2] = (float)((unsigned char) ((rgb) >> 16))/255;

			if (fv < 1.3f)
			{
				file->ReadS32(&i, 1); // m_bAnimation = (i != 0);
				file->ReadS32(&i, 1); // m_bAddKeys = (i != 0);
				file->ReadU8(&ch, 1); // m_nFPS
				file->ReadS32(&i, 1); // m_nCurFrame = i;
				file->ReadU16(&m_nTotalFrames, 1);
				file->ReadS32(&i, 1); //m_nGridSize = i;
				file->ReadS32(&i, 1); //m_nMoveSnap = i;
			}
			else
			{
				file->ReadU8(&ch, 1); // m_bAnimation = (ch != 0);
				file->ReadU8(&ch, 1); // m_bAddKeys = (ch != 0);
				file->ReadU8(&ch, 1); // m_nFPS
				file->ReadU16(&sh, 1); // m_nCurFrame
				file->ReadU16(&m_nTotalFrames, 1);
				file->ReadU16(&sh, 1); // m_nGridSize = sh;
				file->ReadU16(&sh, 1);
				if (fv >= 1.4f)
					m_nMoveSnap = sh;
			}
		}

		if (fv > 1.0f)
		{
			file->ReadU32(&rgb, 1);
//			m_fGradient1[0] = (float)((unsigned char) (rgb))/255;
//			m_fGradient1[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
//			m_fGradient1[2] = (float)((unsigned char) ((rgb) >> 16))/255;
			file->ReadU32(&rgb, 1);
//			m_fGradient2[0] = (float)((unsigned char) (rgb))/255;
//			m_fGradient2[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
//			m_fGradient2[2] = (float)((unsigned char) ((rgb) >> 16))/255;

			if (fv > 1.1f)
				m_pTerrain->FileLoad (file);
			else
			{
				file->Seek(4, SEEK_CUR);
				file->ReadBuffer(&ch, 1);
				file->Seek(ch, SEEK_CUR);
			}
		}
	}

	for (i = 0; i < gMainWindow->mViews.GetSize (); i++)
	{
		gMainWindow->mViews[i]->MakeCurrent();
		RenderInitialize();
	}

	CalculateStep();

	if (!bUndo)
		SelectAndFocusNone(false);

	if (!bMerge)
		gMainWindow->UpdateFocusObject();

	if (!bMerge)
	{
		for (int ViewIdx = 0; ViewIdx < gMainWindow->mViews.GetSize(); ViewIdx++)
		{
			View* view = gMainWindow->mViews[ViewIdx];

			if (!view->mCamera->IsSimple())
				view->SetDefaultCamera();
		}

		if (!bUndo)
			ZoomExtents(0, gMainWindow->mViews.GetSize());
	}

	gMainWindow->UpdateLockSnap(m_nSnap);
	gMainWindow->UpdateSnap();
	gMainWindow->UpdateCameraMenu();
	UpdateSelection();
	gMainWindow->UpdateCurrentTime();
	gMainWindow->UpdateAllViews();

	return true;
}

void Project::FileSave(lcFile* file, bool bUndo)
{
	float ver_flt = 1.4f; // LeoCAD 0.75 - (and this should have been an integer).
	lcuint8 ch;
	lcuint16 sh;
	int i, j;

	const char LC_STR_VERSION[32] = "LeoCAD 0.7 Project\0\0";

	file->Seek(0, SEEK_SET);
	file->WriteBuffer(LC_STR_VERSION, 32);
	file->WriteFloats(&ver_flt, 1);

	file->WriteU32(0xffffffff);//m_fBackground

	i = m_nAngleSnap; file->WriteS32(&i, 1);
	file->WriteU32(&m_nSnap, 1);
	file->WriteFloat(1.0f);//m_fLineWidth
	file->WriteU32(0); // m_nDetail
	i = 0;//i = m_nCurGroup;
	file->WriteS32(&i, 1);
	i = 0;//i = m_nCurColor;
	file->WriteS32(&i, 1);
	file->WriteS32(0);//m_nCurAction
	i = m_nCurStep; file->WriteS32(&i, 1);
	file->WriteU32(0);//m_nScene

	Piece* pPiece;
	for (i = 0, pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		i++;
	file->WriteS32(&i, 1);

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		pPiece->FileSave(*file);

	ch = strlen(m_strAuthor);
	file->WriteBuffer(&ch, 1);
	file->WriteBuffer(m_strAuthor, ch);
	ch = strlen(m_strDescription);
	file->WriteBuffer(&ch, 1);
	file->WriteBuffer(m_strDescription, ch);
	ch = strlen(m_strComments);
	file->WriteBuffer(&ch, 1);
	file->WriteBuffer(m_strComments, ch);

	Group* pGroup;
	for (i = 0, pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
		i++;
	file->WriteS32(&i, 1);

	for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
		pGroup->FileSave(file, m_pGroups);

	lcuint8 ViewportMode = 0, ActiveViewport = 0;
	file->WriteU8(&ViewportMode, 1);
	file->WriteU8(&ActiveViewport, 1);

	i = mCameras.GetSize();
	file->WriteS32(&i, 1);

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		mCameras[CameraIdx]->FileSave(*file);

	for (j = 0; j < 4; j++)
	{
		i = 0;
//		for (i = 0, pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
//			if (pCamera == m_pViewCameras[j])
//				break;
//			else
//				i++;

		file->WriteS32(&i, 1);
	}

	file->WriteU32(0xffffffff);//m_fFogColor
	file->WriteFloat(0.1f);//m_fFogDensity
	sh = strlen(m_strBackground);
	file->WriteU16(&sh, 1);
	file->WriteBuffer(m_strBackground, sh);
	ch = strlen(m_strHeader);
	file->WriteBuffer(&ch, 1);
	file->WriteBuffer(m_strHeader, ch);
	ch = strlen(m_strFooter);
	file->WriteBuffer(&ch, 1);
	file->WriteBuffer(m_strFooter, ch);
	// 0.60 (1.0)
	file->WriteU32(0xffffffff);//m_fAmbient
	ch = m_bAnimation;
	file->WriteBuffer(&ch, 1);
	ch = gMainWindow->mAddKeys;
	file->WriteU8(&ch, 1);
	file->WriteU8 (24); // m_nFPS
	file->WriteU16(1); // m_nCurFrame
	file->WriteU16(&m_nTotalFrames, 1);
	file->WriteU16(0); // m_nGridSize
	file->WriteU16(&m_nMoveSnap, 1);
	// 0.62 (1.1)
	file->WriteU32(0xffffffff); //m_fGradient1
	file->WriteU32(0xffffffff); //m_fGradient2
	// 0.64 (1.2)
	m_pTerrain->FileSave(file);

	if (!bUndo)
	{
		lcuint32 pos = 0;
/*
		// TODO: add file preview
		i = lcGetProfileValue("Default", "Save Preview", 0);
		if (i != 0)
		{
			pos = file->GetPosition();

			Image* image = new Image[1];
			LC_IMAGE_OPTS opts;
			opts.interlaced = false;
			opts.transparent = false;
			opts.format = LC_IMAGE_GIF;

			i = m_nCurStep;
			CreateImages(image, 120, 100, i, i, false);
			image[0].FileSave (*file, &opts);
			delete []image;
		}
*/
		file->WriteU32(&pos, 1);
	}
}

void Project::FileReadMPD(lcFile& MPD, lcArray<LC_FILEENTRY*>& FileArray) const
{
	LC_FILEENTRY* CurFile = NULL;
	char Buf[1024];

	while (MPD.ReadLine(Buf, 1024))
	{
		String Line(Buf);

		Line.TrimLeft();

		if (Line[0] != '0')
		{
			// Copy current line.
			if (CurFile != NULL)
				CurFile->File.WriteBuffer(Buf, strlen(Buf));

			continue;
		}

		Line.TrimRight();
		Line = Line.Right(Line.GetLength() - 1);
		Line.TrimLeft();

		// Find where a subfile starts.
		if (Line.CompareNoCase("FILE", 4) == 0)
		{
			Line = Line.Right(Line.GetLength() - 4);
			Line.TrimLeft();

			// Create a new file.
			CurFile = new LC_FILEENTRY();
			strncpy(CurFile->FileName, Line, sizeof(CurFile->FileName));
			FileArray.Add(CurFile);
		}
		else if (Line.CompareNoCase("ENDFILE", 7) == 0)
		{
			// File ends here.
			CurFile = NULL;
		}
		else if (CurFile != NULL)
		{
			// Copy current line.
			CurFile->File.WriteBuffer(Buf, strlen(Buf));
		}
	}
}

void Project::FileReadLDraw(lcFile* file, const lcMatrix44& CurrentTransform, int* nOk, int DefColor, int* nStep, lcArray<LC_FILEENTRY*>& FileArray)
{
	char Line[1024];

	// Save file offset.
	lcuint32 Offset = file->GetPosition();
	file->Seek(0, SEEK_SET);

	while (file->ReadLine(Line, 1024))
	{
		bool read = true;
		char* Ptr = Line;
		char* Tokens[15];

		for (int TokenIdx = 0; TokenIdx < 15; TokenIdx++)
		{
			Tokens[TokenIdx] = 0;

			while (*Ptr && *Ptr <= 32)
			{
				*Ptr = 0;
				Ptr++;
			}

			Tokens[TokenIdx] = Ptr;

			while (*Ptr > 32)
				Ptr++;
		}

		if (!Tokens[0])
			continue;

		int LineType = atoi(Tokens[0]);

		if (LineType == 0)
		{
			if (Tokens[1])
			{
				strupr(Tokens[1]);

				if (!strcmp(Tokens[1], "STEP"))
					(*nStep)++;
			}

			continue;
		}

		if (LineType != 1)
			continue;

		bool Error = false;
		for (int TokenIdx = 1; TokenIdx < 15; TokenIdx++)
		{
			if (!Tokens[TokenIdx])
			{
				Error = true;
				break;
			}
		}

		if (Error)
			continue;

		int ColorCode = atoi(Tokens[1]);

		float Matrix[12];
		for (int TokenIdx = 2; TokenIdx < 14; TokenIdx++)
			Matrix[TokenIdx - 2] = atof(Tokens[TokenIdx]);

		lcMatrix44 IncludeTransform(lcVector4(Matrix[3], Matrix[6], Matrix[9], 0.0f), lcVector4(Matrix[4], Matrix[7], Matrix[10], 0.0f),
		                            lcVector4(Matrix[5], Matrix[8], Matrix[11], 0.0f), lcVector4(Matrix[0], Matrix[1], Matrix[2], 1.0f));

		IncludeTransform = lcMul(IncludeTransform, CurrentTransform);

		if (ColorCode == 16)
			ColorCode = DefColor;

		char* IncludeName = Tokens[14];
		for (Ptr = IncludeName; *Ptr; Ptr++)
			if (*Ptr == '\r' || *Ptr == '\n')
				*Ptr = 0;

		// See if it's a piece in the library
		if (strlen(IncludeName) < LC_PIECE_NAME_LEN)
		{
			char name[LC_PIECE_NAME_LEN];
			strcpy(name, IncludeName);
			strupr(name);

			Ptr = strrchr(name, '.');
			if (Ptr != NULL)
				*Ptr = 0;

			PieceInfo* pInfo = lcGetPiecesLibrary()->FindPiece(name, false);
			if (pInfo != NULL)
			{
				Piece* pPiece = new Piece(pInfo);
				read = false;

				float* Matrix = IncludeTransform;
				lcMatrix44 Transform(lcVector4(Matrix[0], Matrix[2], -Matrix[1], 0.0f), lcVector4(Matrix[8], Matrix[10], -Matrix[9], 0.0f),
				                     lcVector4(-Matrix[4], -Matrix[6], Matrix[5], 0.0f), lcVector4(0.0f, 0.0f, 0.0f, 1.0f));

				lcVector4 AxisAngle = lcMatrix44ToAxisAngle(Transform);
				AxisAngle[3] *= LC_RTOD;

				pPiece->Initialize(IncludeTransform[3].x / 25.0f, IncludeTransform[3].y / 25.0f, IncludeTransform[3].z / 25.0f, *nStep, 1);
				pPiece->SetColorCode(ColorCode);
				pPiece->CreateName(m_pPieces);
				AddPiece(pPiece);
				pPiece->ChangeKey(1, false, false, AxisAngle, LC_PK_ROTATION);
				pPiece->ChangeKey(1, true, false, AxisAngle, LC_PK_ROTATION);
				SystemPieceComboAdd(pInfo->m_strDescription);
				(*nOk)++;
			}
		}

		// Check for MPD files first.
		if (read)
		{
			for (int i = 0; i < FileArray.GetSize(); i++)
			{
				if (stricmp(FileArray[i]->FileName, IncludeName) == 0)
				{
					FileReadLDraw(&FileArray[i]->File, IncludeTransform, nOk, ColorCode, nStep, FileArray);
					read = false;
					break;
				}
			}
		}

		// Try to read the file from disk.
		if (read)
		{
			lcDiskFile tf;

			if (tf.Open(IncludeName, "rt"))
			{
				FileReadLDraw(&tf, IncludeTransform, nOk, ColorCode, nStep, FileArray);
				read = false;
			}
		}

		if (read)
		{
			// Create a placeholder.
			char name[LC_PIECE_NAME_LEN];
			strcpy(name, IncludeName);
			strupr(name);

			Ptr = strrchr(name, '.');
			if (Ptr != NULL)
				*Ptr = 0;

			PieceInfo* Info = lcGetPiecesLibrary()->CreatePlaceholder(name);

			Piece* pPiece = new Piece(Info);
			read = false;

			float* Matrix = IncludeTransform;
			lcMatrix44 Transform(lcVector4(Matrix[0], Matrix[2], -Matrix[1], 0.0f), lcVector4(Matrix[8], Matrix[10], -Matrix[9], 0.0f),
			                     lcVector4(-Matrix[4], -Matrix[6], Matrix[5], 0.0f), lcVector4(0.0f, 0.0f, 0.0f, 1.0f));

			lcVector4 AxisAngle = lcMatrix44ToAxisAngle(Transform);
			AxisAngle[3] *= LC_RTOD;

			pPiece->Initialize(IncludeTransform[3].x, IncludeTransform[3].y, IncludeTransform[3].z, *nStep, 1);
			pPiece->SetColorCode(ColorCode);
			pPiece->CreateName(m_pPieces);
			AddPiece(pPiece);
			pPiece->ChangeKey(1, false, false, AxisAngle, LC_PK_ROTATION);
			pPiece->ChangeKey(1, true, false, AxisAngle, LC_PK_ROTATION);
			SystemPieceComboAdd(Info->m_strDescription);
			(*nOk)++;
		}
	}

	// Restore file offset.
	file->Seek(Offset, SEEK_SET);
}

bool Project::DoSave(const char* FileName)
{
	char SaveFileName[LC_MAXPATH];

	if (FileName && FileName[0])
		strcpy(SaveFileName, FileName);
	else
	{
		if (m_strPathName[0])
			strcpy(SaveFileName, m_strPathName);
	else
	{
			strcpy(SaveFileName, lcGetProfileString(LC_PROFILE_PROJECTS_PATH));

			int Length = strlen(SaveFileName);
			if (Length && (SaveFileName[Length - 1] != '/' && SaveFileName[Length - 1] != '\\'))
				strcat(SaveFileName, "/");

			strcat(SaveFileName, m_strTitle);

			// check for dubious filename
			int iBad = strcspn(SaveFileName, " #%;/\\");
			if (iBad != -1)
				SaveFileName[iBad] = 0;

			strcat(SaveFileName, ".lcd");
		}

		if (!gMainWindow->DoDialog(LC_DIALOG_SAVE_PROJECT, SaveFileName))
			return false;
	}

	lcDiskFile file;
	if (!file.Open(SaveFileName, "wb"))
	{
//		MessageBox("Failed to save.");
		return false;
	}

	char ext[4];
	memset(ext, 0, 4);
	const char* ptr = strrchr(SaveFileName, '.');
	if (ptr != NULL)
	{
		strncpy(ext, ptr+1, 3);
		strlwr(ext);
	}

	if ((strcmp(ext, "dat") == 0) || (strcmp(ext, "ldr") == 0))
	{
		Piece* pPiece;
		int i, steps = GetLastStep();
		char buf[256];

		ptr = strrchr(m_strPathName, '\\');
		if (ptr == NULL)
			ptr = strrchr(m_strPathName, '/');
		if (ptr == NULL)
			ptr = m_strPathName;
		else
			ptr++;

		sprintf(buf, "0 Model exported from LeoCAD\r\n"
					"0 Original name: %s\r\n", ptr);
		if (strlen(m_strAuthor) != 0)
		{
			strcat(buf, "0 Author: ");
			strcat(buf, m_strAuthor);
			strcat(buf, "\r\n");
		}
		strcat(buf, "\r\n");
		file.WriteBuffer(buf, strlen(buf));

		const char* OldLocale = setlocale(LC_NUMERIC, "C");

		for (i = 1; i <= steps; i++)
		{
			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if ((pPiece->IsVisible(i, false)) && (pPiece->GetStepShow() == i))
				{
					const float* f = pPiece->mModelWorld;
					sprintf (buf, " 1 %d %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %s.DAT\r\n",
					         pPiece->mColorCode, f[12] * 25.0f, -f[14] * 25.0f, f[13] *25.0f, f[0], -f[8], f[4], -f[2], f[10], -f[6], f[1], -f[9], f[5], pPiece->mPieceInfo->m_strName);
					file.WriteBuffer(buf, strlen(buf));
				}
			}

			if (i != steps)
				file.WriteBuffer("0 STEP\r\n", 8);
		}
		file.WriteBuffer("0\r\n", 3);

		setlocale(LC_NUMERIC, OldLocale);
	}
	else
		FileSave(&file, false);     // save me

	file.Close();

	SetModifiedFlag(false);     // back to unmodified

	// reset the title and change the document name
	SetPathName(SaveFileName, true);

	return true; // success
}

// return true if ok to continue
bool Project::SaveModified()
{
	if (!IsModified())
		return true;        // ok to continue

	// get name/title of document
	char name[LC_MAXPATH];
	if (strlen(m_strPathName) == 0)
	{
		// get name based on caption
		strcpy(name, m_strTitle);
		if (strlen(name) == 0)
			strcpy(name, "Untitled");
	}
	else
	{
		// get name based on file title of path name
		char* p;

		p = strrchr(m_strPathName, '\\');
		if (!p)
			p = strrchr(m_strPathName, '/');

		if (p)
			strcpy(name, ++p);
		else
			strcpy(name, m_strPathName);
	}

	char prompt[512];
	sprintf(prompt, "Save changes to %s ?", name);

	switch (gMainWindow->DoMessageBox(prompt, LC_MB_YESNOCANCEL))
	{
	case LC_CANCEL:
		return false;       // don't continue

	case LC_YES:
		// If so, either Save or Update, as appropriate
		if (!DoSave(m_strPathName))
			return false;
		break;

	case LC_NO:
		// If not saving changes, revert the document
		break;
	}

	return true;    // keep going
}

void Project::SetModifiedFlag(bool Modified)
{
	if (m_bModified != Modified)
	{
		m_bModified = Modified;
		gMainWindow->UpdateModified(m_bModified);
	}
}

/////////////////////////////////////////////////////////////////////////////
// File operations

bool Project::OnNewDocument()
{
	SetTitle("Untitled");
	DeleteContents(false);
	memset(m_strPathName, 0, sizeof(m_strPathName)); // no path name yet
	strcpy(m_strAuthor, lcGetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME));
	SetModifiedFlag(false); // make clean
	LoadDefaults(true);
	CheckPoint("");

	return true;
}

bool Project::OpenProject(const char* FileName)
{
	if (!SaveModified())
		return false;  // Leave the original one

	bool WasModified = IsModified();
	SetModifiedFlag(false);  // Not dirty for open

	if (!OnOpenDocument(FileName))
	{
		// Check if we corrupted the original document
		if (!IsModified())
			SetModifiedFlag(WasModified);
		else
			OnNewDocument();

		return false;  // Open failed
	}

	SetPathName(FileName, true);

	return true;
}

bool Project::OnOpenDocument (const char* lpszPathName)
{
  lcDiskFile file;
  bool bSuccess = false;

  if (!file.Open(lpszPathName, "rb"))
  {
//    MessageBox("Failed to open file.");
    return false;
  }

  char ext[4];
  const char *ptr;
  memset(ext, 0, 4);
  ptr = strrchr(lpszPathName, '.');
  if (ptr != NULL)
  {
    strncpy(ext, ptr+1, 3);
    strlwr(ext);
  }

  bool datfile = false;
  bool mpdfile = false;

  // Find out what file type we're loading.
  if ((strcmp(ext, "dat") == 0) || (strcmp(ext, "ldr") == 0))
    datfile = true;
  else if (strcmp(ext, "mpd") == 0)
    mpdfile = true;

  // Delete the current project.
  DeleteContents(false);
  LoadDefaults(datfile || mpdfile);
  SetModifiedFlag(true);  // dirty during loading

  if (file.GetLength() != 0)
  {
	lcArray<LC_FILEENTRY*> FileArray;

    // Unpack the MPD file.
    if (mpdfile)
    {
      FileReadMPD(file, FileArray);

      if (FileArray.GetSize() == 0)
      {
        file.Seek(0, SEEK_SET);
        mpdfile = false;
        datfile = true;
      }
    }

    if (datfile || mpdfile)
    {
      int ok = 0, step = 1;
      lcMatrix44 mat = lcMatrix44Identity();

      if (mpdfile)
        FileReadLDraw(&FileArray[0]->File, mat, &ok, 16, &step, FileArray);
      else
        FileReadLDraw(&file, mat, &ok, 16, &step, FileArray);

      m_nCurStep = step;
	  gMainWindow->UpdateCurrentTime();
	  gMainWindow->UpdateFocusObject();
      UpdateSelection();
      CalculateStep();

	  ZoomExtents(0, gMainWindow->mViews.GetSize());
	  gMainWindow->UpdateAllViews();

      bSuccess = true;
    }
    else
    {
      // Load a LeoCAD file.
      bSuccess = FileLoad(&file, false, false);
    }

    // Clean up.
    if (mpdfile)
    {
      for (int i = 0; i < FileArray.GetSize(); i++)
        delete FileArray[i];
    }
  }

  file.Close();

  if (bSuccess == false)
  {
//    MessageBox("Failed to load.");
    DeleteContents(false);   // remove failed contents
    return false;
  }

  CheckPoint("");

  SetModifiedFlag(false);     // start off with unmodified

  return true;
}

void Project::SetPathName(const char* PathName, bool bAddToMRU)
{
	strcpy(m_strPathName, PathName);

	// always capture the complete file name including extension (if present)
	const char* lpszTemp = PathName;
	for (const char* lpsz = PathName; *lpsz != '\0'; lpsz++)
	{
		// remember last directory/drive separator
		if (*lpsz == '\\' || *lpsz == '/' || *lpsz == ':')
			lpszTemp = lpsz + 1;
	}

	// set the document title based on path name
	SetTitle(lpszTemp);

	// add it to the file MRU list
	if (bAddToMRU)
		gMainWindow->AddRecentFile(m_strPathName);
}

/////////////////////////////////////////////////////////////////////////////
// Undo/Redo support

// Save current state.
void Project::CheckPoint (const char* text)
{
	LC_UNDOINFO* pTmp;
	LC_UNDOINFO* pUndo = new LC_UNDOINFO;
	int i;

	strcpy(pUndo->strText, text);
	FileSave(&pUndo->file, true);

	for (pTmp = m_pUndoList, i = 0; pTmp; pTmp = pTmp->pNext, i++)
		if ((i == 30) && (pTmp->pNext != NULL))
		{
			delete pTmp->pNext;
			pTmp->pNext = NULL;
			m_bUndoOriginal = false;
		}

	pUndo->pNext = m_pUndoList;
	m_pUndoList = pUndo;

	while (m_pRedoList)
	{
		pUndo = m_pRedoList;
		m_pRedoList = m_pRedoList->pNext;
		delete pUndo;
	}
	m_pRedoList = NULL;

	gMainWindow->UpdateUndoRedo(m_pUndoList->pNext ? m_pUndoList->strText : NULL, NULL);
}

/////////////////////////////////////////////////////////////////////////////
// Project rendering

void Project::Render(View* View, bool ToMemory)
{
	glViewport(0, 0, View->mWidth, View->mHeight);
	glEnableClientState(GL_VERTEX_ARRAY);

	mActiveModel->DrawBackground(View);
	mActiveModel->DrawScene(View, !ToMemory);

	glDisableClientState(GL_VERTEX_ARRAY);
}

void Project::RenderSceneObjects(View* view)
{
#ifdef LC_DEBUG
	RenderDebugPrimitives();
#endif
/*
	if (m_nCurAction == LC_TOOL_INSERT || mDropPiece)
	{
		lcVector3 Pos;
		lcVector4 Rot;
		GetPieceInsertPosition(view, m_nDownX, m_nDownY, Pos, Rot);
		PieceInfo* PreviewPiece = mDropPiece ? mDropPiece : m_pCurPiece;

		glPushMatrix();
		glTranslatef(Pos[0], Pos[1], Pos[2]);
		glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);
		glLineWidth(2*m_fLineWidth);
		PreviewPiece->RenderPiece(gMainWindow->mColorIndex);
		glLineWidth(m_fLineWidth);
		glPopMatrix();
	}

	// Draw axis icon
	if (m_nSnap & LC_DRAW_AXIS)
	{
//		glClear(GL_DEPTH_BUFFER_BIT);

		lcMatrix44 Mats[3];
		Mats[0] = view->mCamera->mWorldView;
		Mats[0].SetTranslation(lcVector3(0, 0, 0));
		Mats[1] = lcMul(lcMatrix44(lcVector4(0, 1, 0, 0), lcVector4(1, 0, 0, 0), lcVector4(0, 0, 1, 0), lcVector4(0, 0, 0, 1)), Mats[0]);
		Mats[2] = lcMul(lcMatrix44(lcVector4(0, 0, 1, 0), lcVector4(0, 1, 0, 0), lcVector4(1, 0, 0, 0), lcVector4(0, 0, 0, 1)), Mats[0]);

		lcVector3 pts[3] =
		{
			lcMul30(lcVector3(20, 0, 0), Mats[0]),
			lcMul30(lcVector3(0, 20, 0), Mats[0]),
			lcMul30(lcVector3(0, 0, 20), Mats[0]),
		};

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, view->mWidth, 0, view->mHeight, -50, 50);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glTranslatef(25.375f, 25.375f, 0.0f);

		// Draw the arrows.
		lcVector3 Verts[11];
		Verts[0] = lcVector3(0.0f, 0.0f, 0.0f);

		glVertexPointer(3, GL_FLOAT, 0, Verts);

		for (int i = 0; i < 3; i++)
		{
			switch (i)
			{
			case 0:
				glColor4f(0.8f, 0.0f, 0.0f, 1.0f);
				break;
			case 1:
				glColor4f(0.0f, 0.8f, 0.0f, 1.0f);
				break;
			case 2:
				glColor4f(0.0f, 0.0f, 0.8f, 1.0f);
				break;
			}

			Verts[1] = pts[i];

			for (int j = 0; j < 9; j++)
				Verts[j+2] = lcMul30(lcVector3(12.0f, cosf(LC_2PI * j / 8) * 3.0f, sinf(LC_2PI * j / 8) * 3.0f), Mats[i]);

			glDrawArrays(GL_LINES, 0, 2);
			glDrawArrays(GL_TRIANGLE_FAN, 1, 10);
		}

		// Draw the text.
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		m_pScreenFont->MakeCurrent();
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		m_pScreenFont->PrintText(pts[0][0], pts[0][1], 40.0f, "X");
		m_pScreenFont->PrintText(pts[1][0], pts[1][1], 40.0f, "Y");
		m_pScreenFont->PrintText(pts[2][0], pts[2][1], 40.0f, "Z");

		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
	*/
}

void Project::RenderInitialize()
{
/*
	if (m_nScene & LC_SCENE_FLOOR)
		m_pTerrain->LoadTexture();

	if (m_nScene & LC_SCENE_BG)
		if (!m_pBackground->Load(m_strBackground, LC_TEXTURE_WRAPU | LC_TEXTURE_WRAPV))
		{
			m_nScene &= ~LC_SCENE_BG;
//			AfxMessageBox ("Could not load background");
		}
*/
}

/////////////////////////////////////////////////////////////////////////////
// Project functions

void Project::AddPiece(Piece* pPiece)
{
	if (m_pPieces != NULL)
	{
		pPiece->m_pNext = m_pPieces;
		m_pPieces = pPiece;
	}
	else
	{
		m_pPieces = pPiece;
		pPiece->m_pNext = NULL;
	}
}

void Project::CalculateStep()
{
	Piece* pPiece;
	Light* pLight;

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		pPiece->UpdatePosition(m_nCurStep, m_bAnimation);

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		mCameras[CameraIdx]->UpdatePosition(m_nCurStep, m_bAnimation);

	for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
		pLight->UpdatePosition(m_nCurStep, m_bAnimation);
}

void Project::UpdateSelection()
{
	unsigned long flags = 0;
	int SelectedCount = 0;
	Object* Focus = NULL;

	if (m_pPieces == NULL)
		flags |= LC_SEL_NO_PIECES;
	else
	{
		Piece* pPiece;
		Group* pGroup = NULL;
		bool first = true;

		for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		{
			if (pPiece->IsSelected())
			{
				SelectedCount++;

				if (pPiece->IsFocused())
					Focus = pPiece;

				if (flags & LC_SEL_PIECE)
					flags |= LC_SEL_MULTIPLE;
				else
					flags |= LC_SEL_PIECE;

				if (pPiece->GetGroup() != NULL)
				{
					flags |= LC_SEL_GROUP;
					if (pPiece->IsFocused())
						flags |= LC_SEL_FOCUSGROUP;
				}

				if (first)
				{
					pGroup = pPiece->GetGroup();
					first = false;
				}
				else
				{
					if (pGroup != pPiece->GetGroup())
						flags |= LC_SEL_CANGROUP;
					else
						if (pGroup == NULL)
							flags |= LC_SEL_CANGROUP;
				}
			}
			else
			{
				flags |= LC_SEL_UNSELECTED;

				if (pPiece->IsHidden())
					flags |= LC_SEL_HIDDEN;
			}
		}
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		Camera* pCamera = mCameras[CameraIdx];

		if (pCamera->IsSelected())
		{
			flags |= LC_SEL_CAMERA;
			SelectedCount++;

			if (pCamera->IsEyeFocused() || pCamera->IsTargetFocused())
				Focus = pCamera;
		}
	}

	for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
		if (pLight->IsSelected())
		{
			flags |= LC_SEL_LIGHT;
			SelectedCount++;

			if (pLight->IsEyeFocused() || pLight->IsTargetFocused())
				Focus = pLight;
		}

	gMainWindow->UpdateSelectedObjects(flags, SelectedCount, Focus);
}

unsigned char Project::GetLastStep()
{
	unsigned char last = 1;
	for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		last = lcMax(last, pPiece->GetStepShow());

	return last;
}

void Project::FindPiece(bool FindFirst, bool SearchForward)
{
	if (!m_pPieces)
		return;

	Piece* Start = NULL;
	if (!FindFirst)
	{
		for (Start = m_pPieces; Start; Start = Start->m_pNext)
			if (Start->IsFocused())
				break;

		if (Start && !Start->IsVisible(m_nCurStep, m_bAnimation))
			Start = NULL;
	}

	SelectAndFocusNone(false);

	Piece* Current = Start;

	for (;;)
	{
		if (SearchForward)
		{
			Current = Current ? Current->m_pNext : m_pPieces;
		}
		else
		{
			if (Current == m_pPieces)
				Current = NULL;

			for (Piece* piece = m_pPieces; piece; piece = piece->m_pNext)
			{
				if (piece->m_pNext == Current)
				{
					Current = piece;
					break;
				}
			}
		}

		if (Current == Start)
			break;

		if (!Current)
			continue;

		if (!Current->IsVisible(m_nCurStep, m_bAnimation))
			continue;

		if ((!mSearchOptions.MatchInfo || Current->mPieceInfo == mSearchOptions.Info) &&
			(!mSearchOptions.MatchColor || Current->mColorIndex == mSearchOptions.ColorIndex) &&
			(!mSearchOptions.MatchName || strcasestr(Current->GetName(), mSearchOptions.Name)))
		{
			Current->Select(true, true, false);
			Group* TopGroup = Current->GetTopGroup();
			if (TopGroup)
			{
				for (Piece* piece = m_pPieces; piece; piece = piece->m_pNext)
					if ((piece->IsVisible(m_nCurStep, m_bAnimation)) &&
						(piece->GetTopGroup() == TopGroup))
						piece->Select (true, false, false);
			}

			UpdateSelection();
			gMainWindow->UpdateAllViews();
			gMainWindow->UpdateFocusObject();

			break;
		}
	}
}

void Project::ZoomExtents(int FirstView, int LastView)
{
	if (!m_pPieces)
		return;

	float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };

	for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		if (pPiece->IsVisible(m_nCurStep, m_bAnimation))
			pPiece->CompareBoundingBox(bs);

	lcVector3 Center((bs[0] + bs[3]) / 2, (bs[1] + bs[4]) / 2, (bs[2] + bs[5]) / 2);

	lcVector3 Points[8] =
	{
		lcVector3(bs[0], bs[1], bs[5]),
		lcVector3(bs[3], bs[1], bs[5]),
		lcVector3(bs[0], bs[1], bs[2]),
		lcVector3(bs[3], bs[4], bs[5]),
		lcVector3(bs[3], bs[4], bs[2]),
		lcVector3(bs[0], bs[4], bs[2]),
		lcVector3(bs[0], bs[4], bs[5]),
		lcVector3(bs[3], bs[1], bs[2])
	};

	for (int vp = FirstView; vp < LastView; vp++)
	{
		View* view = gMainWindow->mViews[vp];

		mActiveModel->ZoomExtents(view, Center, Points, gMainWindow->mAddKeys);
	}

	gMainWindow->UpdateFocusObject();
	gMainWindow->UpdateAllViews();
}

void Project::CreateImages(Image* images, int width, int height, unsigned short from, unsigned short to, bool hilite)
{
	if (!GL_BeginRenderToTexture(width, height))
	{
		gMainWindow->DoMessageBox("Error creating images.", LC_MB_ICONERROR | LC_MB_OK);
		return;
	}

	unsigned short oldtime;
	unsigned char* buf = (unsigned char*)malloc (width*height*3);
	oldtime = m_nCurStep;

	View view(this);
	view.SetCamera(gMainWindow->mActiveView->mCamera, false);
	view.mWidth = width;
	view.mHeight = height;

	if (!hilite)
		SelectAndFocusNone(false);

	RenderInitialize();

	for (int i = from; i <= to; i++)
	{
		m_nCurStep = i;

		if (hilite)
		{
			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if ((m_bAnimation && pPiece->GetFrameShow() == i) ||
					(!m_bAnimation && pPiece->GetStepShow() == i))
					pPiece->Select (true, false, false);
				else
					pPiece->Select (false, false, false);
			}
		}

		CalculateStep();
		Render(&view, true);
		images[i-from].FromOpenGL (width, height);
	}

	m_nCurStep = (unsigned char)oldtime;
	CalculateStep();
	free (buf);

	GL_EndRenderToTexture();
}

void Project::CreateHTMLPieceList(FILE* f, int nStep, bool bImages, const char* ext)
{
	int* ColorsUsed = new int[gColorList.GetSize()];
	memset(ColorsUsed, 0, sizeof(ColorsUsed[0]) * gColorList.GetSize());
	int* PiecesUsed = new int[gColorList.GetSize()];
	int NumColors = 0;

	for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		if ((pPiece->GetStepShow() == nStep) || (nStep == 0))
			ColorsUsed[pPiece->mColorIndex]++;
	}

	fputs("<br><table border=1><tr><td><center>Piece</center></td>\n",f);

	for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
	{
		if (ColorsUsed[ColorIdx])
		{
			ColorsUsed[ColorIdx] = NumColors;
			NumColors++;
			fprintf(f, "<td><center>%s</center></td>\n", gColorList[ColorIdx].Name);
		}
	}
	NumColors++;
	fputs("</tr>\n",f);

	PieceInfo* pInfo;
	for (int j = 0; j < lcGetPiecesLibrary()->mPieces.GetSize(); j++)
	{
		bool Add = false;
		memset(PiecesUsed, 0, sizeof(PiecesUsed[0]) * gColorList.GetSize());
		pInfo = lcGetPiecesLibrary()->mPieces[j];

		for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		{
			if ((pPiece->mPieceInfo == pInfo) && ((pPiece->GetStepShow() == nStep) || (nStep == 0)))
			{
				PiecesUsed[pPiece->mColorIndex]++;
				Add = true;
			}
		}

		if (Add)
		{
			if (bImages)
				fprintf(f, "<tr><td><IMG SRC=\"%s%s\" ALT=\"%s\"></td>\n", pInfo->m_strName, ext, pInfo->m_strDescription);
			else
					fprintf(f, "<tr><td>%s</td>\n", pInfo->m_strDescription);

			int curcol = 1;
			for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
			{
				if (PiecesUsed[ColorIdx])
				{
					while (curcol != ColorsUsed[ColorIdx] + 1)
					{
						fputs("<td><center>-</center></td>\n", f);
						curcol++;
					}

					fprintf(f, "<td><center>%d</center></td>\n", PiecesUsed[ColorIdx]);
					curcol++;
				}
			}

			while (curcol != NumColors)
			{
				fputs("<td><center>-</center></td>\n", f);
				curcol++;
			}

			fputs("</tr>\n", f);
		}
	}
	fputs("</table>\n<br>", f);

	delete[] PiecesUsed;
	delete[] ColorsUsed;
}

void Project::Export3DStudio()
{
	/*
	if (!m_pPieces)
	{
		gMainWindow->DoMessageBox("Nothing to export.", LC_MB_OK | LC_MB_ICONINFORMATION);
		return;
	}

	char FileName[LC_MAXPATH];
	memset(FileName, 0, sizeof(FileName));

	if (!gMainWindow->DoDialog(LC_DIALOG_EXPORT_3DSTUDIO, FileName))
		return;

	lcDiskFile File;

	if (!File.Open(FileName, "wb"))
	{
		gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK | LC_MB_ICONERROR);
		return;
	}

	long M3DStart = File.GetPosition();
	File.WriteU16(0x4D4D); // CHK_M3DMAGIC
	File.WriteU32(0);

	File.WriteU16(0x0002); // CHK_M3D_VERSION
	File.WriteU32(10);
	File.WriteU32(3);

	long MDataStart = File.GetPosition();
	File.WriteU16(0x3D3D); // CHK_MDATA
	File.WriteU32(0);

	File.WriteU16(0x3D3E); // CHK_MESH_VERSION
	File.WriteU32(10);
	File.WriteU32(3);

	const int MaterialNameLength = 11;
	char MaterialName[32];

	for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
	{
		lcColor* Color = &gColorList[ColorIdx];

		sprintf(MaterialName, "Material%03d", ColorIdx);

		long MaterialStart = File.GetPosition();
		File.WriteU16(0xAFFF); // CHK_MAT_ENTRY
		File.WriteU32(0);

		File.WriteU16(0xA000); // CHK_MAT_NAME
		File.WriteU32(6 + MaterialNameLength + 1);
		File.WriteBuffer(MaterialName, MaterialNameLength + 1);

		File.WriteU16(0xA010); // CHK_MAT_AMBIENT
		File.WriteU32(24);

		File.WriteU16(0x0011); // CHK_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * Color->Value[0] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[1] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[2] + 0.5));

		File.WriteU16(0x0012); // CHK_LIN_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * Color->Value[0] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[1] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[2] + 0.5));

		File.WriteU16(0xA020); // CHK_MAT_AMBIENT
		File.WriteU32(24);

		File.WriteU16(0x0011); // CHK_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * Color->Value[0] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[1] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[2] + 0.5));

		File.WriteU16(0x0012); // CHK_LIN_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * Color->Value[0] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[1] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[2] + 0.5));

		File.WriteU16(0xA030); // CHK_MAT_SPECULAR
		File.WriteU32(24);

		File.WriteU16(0x0011); // CHK_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * 0.9f + 0.5));
		File.WriteU8(floor(255.0 * 0.9f + 0.5));
		File.WriteU8(floor(255.0 * 0.9f + 0.5));

		File.WriteU16(0x0012); // CHK_LIN_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * 0.9f + 0.5));
		File.WriteU8(floor(255.0 * 0.9f + 0.5));
		File.WriteU8(floor(255.0 * 0.9f + 0.5));

		File.WriteU16(0xA040); // CHK_MAT_SHININESS
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * 0.25 + 0.5));

		File.WriteU16(0xA041); // CHK_MAT_SHIN2PCT
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * 0.05 + 0.5));

		File.WriteU16(0xA050); // CHK_MAT_TRANSPARENCY
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * (1.0f - Color->Value[3]) + 0.5));

		File.WriteU16(0xA052); // CHK_MAT_XPFALL
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * 0.0 + 0.5));

		File.WriteU16(0xA053); // CHK_MAT_REFBLUR
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * 0.2 + 0.5));

		File.WriteU16(0xA100); // CHK_MAT_SHADING
		File.WriteU32(8);

		File.WriteS16(3);

		File.WriteU16(0xA084); // CHK_MAT_SELF_ILPCT
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * 0.0 + 0.5));

		File.WriteU16(0xA081); // CHK_MAT_TWO_SIDE
		File.WriteU32(6);

		File.WriteU16(0xA087); // CHK_MAT_WIRE_SIZE
		File.WriteU32(10);

		File.WriteFloat(1.0f);

		long MaterialEnd = File.GetPosition();
		File.Seek(MaterialStart + 2, SEEK_SET);
		File.WriteU32(MaterialEnd - MaterialStart);
		File.Seek(MaterialEnd, SEEK_SET);
	}

	File.WriteU16(0x0100); // CHK_MASTER_SCALE
	File.WriteU32(10);

	File.WriteFloat(1.0f);

	File.WriteU16(0x1400); // CHK_LO_SHADOW_BIAS
	File.WriteU32(10);

	File.WriteFloat(1.0f);

	File.WriteU16(0x1420); // CHK_SHADOW_MAP_SIZE
	File.WriteU32(8);

	File.WriteS16(512);

	File.WriteU16(0x1450); // CHK_SHADOW_FILTER
	File.WriteU32(10);

	File.WriteFloat(3.0f);

	File.WriteU16(0x1460); // CHK_RAY_BIAS
	File.WriteU32(10);

	File.WriteFloat(1.0f);

	File.WriteU16(0x1500); // CHK_O_CONSTS
	File.WriteU32(18);

	File.WriteFloat(0.0f);
	File.WriteFloat(0.0f);
	File.WriteFloat(0.0f);

	File.WriteU16(0x2100); // CHK_AMBIENT_LIGHT
	File.WriteU32(42);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fAmbient[0]);
	File.WriteFloat(m_fAmbient[1]);
	File.WriteFloat(m_fAmbient[2]);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fAmbient[0]);
	File.WriteFloat(m_fAmbient[1]);
	File.WriteFloat(m_fAmbient[2]);

	File.WriteU16(0x1200); // CHK_SOLID_BGND
	File.WriteU32(42);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fBackground[0]);
	File.WriteFloat(m_fBackground[1]);
	File.WriteFloat(m_fBackground[2]);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fBackground[0]);
	File.WriteFloat(m_fBackground[1]);
	File.WriteFloat(m_fBackground[2]);

	File.WriteU16(0x1100); // CHK_BIT_MAP
	File.WriteU32(6 + 1 + strlen(m_strBackground));
	File.WriteBuffer(m_strBackground, strlen(m_strBackground) + 1);

	File.WriteU16(0x1300); // CHK_V_GRADIENT
	File.WriteU32(118);

	File.WriteFloat(1.0f);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fGradient1[0]);
	File.WriteFloat(m_fGradient1[1]);
	File.WriteFloat(m_fGradient1[2]);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fGradient1[0]);
	File.WriteFloat(m_fGradient1[1]);
	File.WriteFloat(m_fGradient1[2]);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloat((m_fGradient1[0] + m_fGradient2[0]) / 2.0f);
	File.WriteFloat((m_fGradient1[1] + m_fGradient2[1]) / 2.0f);
	File.WriteFloat((m_fGradient1[2] + m_fGradient2[2]) / 2.0f);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloat((m_fGradient1[0] + m_fGradient2[0]) / 2.0f);
	File.WriteFloat((m_fGradient1[1] + m_fGradient2[1]) / 2.0f);
	File.WriteFloat((m_fGradient1[2] + m_fGradient2[2]) / 2.0f);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fGradient2[0]);
	File.WriteFloat(m_fGradient2[1]);
	File.WriteFloat(m_fGradient2[2]);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fGradient2[0]);
	File.WriteFloat(m_fGradient2[1]);
	File.WriteFloat(m_fGradient2[2]);

	if (m_nScene & LC_SCENE_GRADIENT)
	{
		File.WriteU16(0x1301); // LIB3DS_USE_V_GRADIENT
		File.WriteU32(6);
	}
	else if (m_nScene & LC_SCENE_BG)
	{
		File.WriteU16(0x1101); // LIB3DS_USE_BIT_MAP
		File.WriteU32(6);
	}
	else
	{
		File.WriteU16(0x1201); // LIB3DS_USE_SOLID_BGND
		File.WriteU32(6);
	}

	File.WriteU16(0x2200); // CHK_FOG
	File.WriteU32(46);

	File.WriteFloat(0.0f);
	File.WriteFloat(0.0f);
	File.WriteFloat(1000.0f);
	File.WriteFloat(100.0f);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fFogColor[0]);
	File.WriteFloat(m_fFogColor[1]);
	File.WriteFloat(m_fFogColor[2]);

	File.WriteU16(0x2210); // CHK_FOG_BGND
	File.WriteU32(6);

	File.WriteU16(0x2302); // CHK_LAYER_FOG
	File.WriteU32(40);

	File.WriteFloat(0.0f);
	File.WriteFloat(100.0f);
	File.WriteFloat(50.0f);
	File.WriteU32(0x00100000);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fFogColor[0]);
	File.WriteFloat(m_fFogColor[1]);
	File.WriteFloat(m_fFogColor[2]);

	File.WriteU16(0x2300); // CHK_DISTANCE_CUE
	File.WriteU32(28);

	File.WriteFloat(0.0f);
	File.WriteFloat(0.0f);
	File.WriteFloat(1000.0f);
	File.WriteFloat(100.0f);

	File.WriteU16(0x2310); // CHK_DICHK_DCUE_BGNDSTANCE_CUE
	File.WriteU32(6);

	int NumPieces = 0;
	for (Piece* piece = m_pPieces; piece; piece = piece->m_pNext)
	{
		PieceInfo* Info = piece->mPieceInfo;
		lcMesh* Mesh = Info->mMesh;

		if (Mesh->mIndexType == GL_UNSIGNED_INT)
			continue;

		long NamedObjectStart = File.GetPosition();
		File.WriteU16(0x4000); // CHK_NAMED_OBJECT
		File.WriteU32(0);

		char Name[32];
		sprintf(Name, "Piece%.3d", NumPieces);
		NumPieces++;
		File.WriteBuffer(Name, strlen(Name) + 1);

		long TriObjectStart = File.GetPosition();
		File.WriteU16(0x4100); // CHK_N_TRI_OBJECT
		File.WriteU32(0);

		File.WriteU16(0x4110); // CHK_POINT_ARRAY
		File.WriteU32(8 + 12 * Mesh->mNumVertices);

		File.WriteU16(Mesh->mNumVertices);

		float* Verts = (float*)Mesh->mVertexBuffer.mData;
		const lcMatrix44& ModelWorld = piece->mModelWorld;

		for (int VertexIdx = 0; VertexIdx < Mesh->mNumVertices; VertexIdx++)
		{
			lcVector3 Pos(Verts[VertexIdx * 3], Verts[VertexIdx * 3 + 1], Verts[VertexIdx * 3 + 2]);
			Pos = lcMul31(Pos, ModelWorld);
			File.WriteFloat(Pos[0]);
			File.WriteFloat(Pos[1]);
			File.WriteFloat(Pos[2]);
		}

		File.WriteU16(0x4160); // CHK_MESH_MATRIX
		File.WriteU32(54);

		lcMatrix44 Matrix = lcMatrix44Identity();
		File.WriteFloats(Matrix[0], 3);
		File.WriteFloats(Matrix[1], 3);
		File.WriteFloats(Matrix[2], 3);
		File.WriteFloats(Matrix[3], 3);

		File.WriteU16(0x4165); // CHK_MESH_COLOR
		File.WriteU32(7);

		File.WriteU8(0);

		long FaceArrayStart = File.GetPosition();
		File.WriteU16(0x4120); // CHK_FACE_ARRAY
		File.WriteU32(0);

		int NumTriangles = 0;

		for (int SectionIdx = 0; SectionIdx < Mesh->mNumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mSections[SectionIdx];

			if (Section->PrimitiveType != GL_TRIANGLES)
				continue;

			NumTriangles += Section->NumIndices / 3;
		}

		File.WriteU16(NumTriangles);

		for (int SectionIdx = 0; SectionIdx < Mesh->mNumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mSections[SectionIdx];

			if (Section->PrimitiveType != GL_TRIANGLES)
				continue;

			lcuint16* Indices = (lcuint16*)Mesh->mIndexBuffer.mData + Section->IndexOffset / sizeof(lcuint16);

			for (int IndexIdx = 0; IndexIdx < Section->NumIndices; IndexIdx += 3)
			{
				File.WriteU16(Indices[IndexIdx + 0]);
				File.WriteU16(Indices[IndexIdx + 1]);
				File.WriteU16(Indices[IndexIdx + 2]);
				File.WriteU16(7);
			}
		}

		NumTriangles = 0;

		for (int SectionIdx = 0; SectionIdx < Mesh->mNumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mSections[SectionIdx];

			if (Section->PrimitiveType != GL_TRIANGLES)
				continue;

			int MaterialIndex = Section->ColorIndex == gDefaultColor ? piece->mColorIndex : Section->ColorIndex;

			File.WriteU16(0x4130); // CHK_MSH_MAT_GROUP
			File.WriteU32(6 + MaterialNameLength + 1 + 2 + 2 * Section->NumIndices / 3);

			sprintf(MaterialName, "Material%03d", MaterialIndex);
			File.WriteBuffer(MaterialName, MaterialNameLength + 1);

			File.WriteU16(Section->NumIndices / 3);

			for (int IndexIdx = 0; IndexIdx < Section->NumIndices; IndexIdx += 3)
				File.WriteU16(NumTriangles++);
		}

		long FaceArrayEnd = File.GetPosition();
		File.Seek(FaceArrayStart + 2, SEEK_SET);
		File.WriteU32(FaceArrayEnd - FaceArrayStart);
		File.Seek(FaceArrayEnd, SEEK_SET);

		long TriObjectEnd = File.GetPosition();
		File.Seek(TriObjectStart + 2, SEEK_SET);
		File.WriteU32(TriObjectEnd - TriObjectStart);
		File.Seek(TriObjectEnd, SEEK_SET);

		long NamedObjectEnd = File.GetPosition();
		File.Seek(NamedObjectStart + 2, SEEK_SET);
		File.WriteU32(NamedObjectEnd - NamedObjectStart);
		File.Seek(NamedObjectEnd, SEEK_SET);
	}

	long MDataEnd = File.GetPosition();
	File.Seek(MDataStart + 2, SEEK_SET);
	File.WriteU32(MDataEnd - MDataStart);
	File.Seek(MDataEnd, SEEK_SET);

	long KFDataStart = File.GetPosition();
	File.WriteU16(0xB000); // CHK_KFDATA
	File.WriteU32(0);

	File.WriteU16(0xB00A); // LIB3DS_KFHDR
	File.WriteU32(6 + 2 + 1 + 4);

	File.WriteS16(5);
	File.WriteU8(0);
	File.WriteS32(100);

	long KFDataEnd = File.GetPosition();
	File.Seek(KFDataStart + 2, SEEK_SET);
	File.WriteU32(KFDataEnd - KFDataStart);
	File.Seek(KFDataEnd, SEEK_SET);

	long M3DEnd = File.GetPosition();
	File.Seek(M3DStart + 2, SEEK_SET);
	File.WriteU32(M3DEnd - M3DStart);
	File.Seek(M3DEnd, SEEK_SET);
	*/
}

void Project::ExportBrickLink()
{
	lcArray<lcObjectParts> PartsUsed;
	mActiveModel->GetPartsUsed(PartsUsed);

	if (!PartsUsed.GetSize())
	{
		gMainWindow->DoMessageBox("Nothing to export.", LC_MB_OK | LC_MB_ICONINFORMATION);
		return;
	}

	char FileName[LC_MAXPATH];
	memset(FileName, 0, sizeof(FileName));

	if (!gMainWindow->DoDialog(LC_DIALOG_EXPORT_BRICKLINK, FileName))
		return;

	lcDiskFile BrickLinkFile;
	char Line[1024];

	if (!BrickLinkFile.Open(FileName, "wt"))
	{
		gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK | LC_MB_ICONERROR);
		return;
	}

	const char* OldLocale = setlocale(LC_NUMERIC, "C");
	BrickLinkFile.WriteLine("<INVENTORY>\n");

	for (int PartIdx = 0; PartIdx < PartsUsed.GetSize(); PartIdx++)
	{
		BrickLinkFile.WriteLine("  <ITEM>\n");
		BrickLinkFile.WriteLine("    <ITEMTYPE>P</ITEMTYPE>\n");

		sprintf(Line, "    <ITEMID>%s</ITEMID>\n", PartsUsed[PartIdx].Info->m_strName);
		BrickLinkFile.WriteLine(Line);

		int Count = PartsUsed[PartIdx].Count;
		if (Count > 1)
		{
			sprintf(Line, "    <MINQTY>%d</MINQTY>\n", Count);
			BrickLinkFile.WriteLine(Line);
		}

		int Color = lcGetBrickLinkColor(PartsUsed[PartIdx].ColorIndex);
		if (Color)
		{
			sprintf(Line, "    <COLOR>%d</COLOR>\n", Color);
			BrickLinkFile.WriteLine(Line);
		}

		BrickLinkFile.WriteLine("  </ITEM>\n");
	}

	BrickLinkFile.WriteLine("</INVENTORY>\n");

	setlocale(LC_NUMERIC, OldLocale);
}

void Project::ExportCSV()
{
	lcArray<lcObjectParts> PartsUsed;
	mActiveModel->GetPartsUsed(PartsUsed);

	if (!PartsUsed.GetSize())
	{
		gMainWindow->DoMessageBox("Nothing to export.", LC_MB_OK | LC_MB_ICONINFORMATION);
		return;
	}

	char FileName[LC_MAXPATH];
	memset(FileName, 0, sizeof(FileName));

	if (!gMainWindow->DoDialog(LC_DIALOG_EXPORT_CSV, FileName))
		return;

	lcDiskFile CSVFile;
	char Line[1024];

	if (!CSVFile.Open(FileName, "wt"))
	{
		gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK | LC_MB_ICONERROR);
		return;
	}

	const char* OldLocale = setlocale(LC_NUMERIC, "C");
	CSVFile.WriteLine("Part Name,Color,Quantity,Part ID,Color Code\n");

	for (int PartIdx = 0; PartIdx < PartsUsed.GetSize(); PartIdx++)
	{
		sprintf(Line, "\"%s\",\"%s\",%d,%s,%d\n", PartsUsed[PartIdx].Info->m_strDescription, gColorList[PartsUsed[PartIdx].ColorIndex].Name,
				PartsUsed[PartIdx].Count, PartsUsed[PartIdx].Info->m_strName, gColorList[PartsUsed[PartIdx].ColorIndex].Code);
		CSVFile.WriteLine(Line);
	}

	setlocale(LC_NUMERIC, OldLocale);
}

void Project::ExportPOVRay(lcFile& POVFile)
{
	/*
	char Line[1024];

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	char* PieceTable = new char[Library->mPieces.GetSize() * LC_PIECE_NAME_LEN];
	int* PieceFlags = new int[Library->mPieces.GetSize()];
	int NumColors = gColorList.GetSize();
	char* ColorTable = new char[NumColors * LC_MAX_COLOR_NAME];

	memset(PieceTable, 0, Library->mPieces.GetSize() * LC_PIECE_NAME_LEN);
	memset(PieceFlags, 0, Library->mPieces.GetSize() * sizeof(int));
	memset(ColorTable, 0, NumColors * LC_MAX_COLOR_NAME);

	enum
	{
		LGEO_PIECE_LGEO  = 0x01,
		LGEO_PIECE_AR    = 0x02,
		LGEO_PIECE_SLOPE = 0x04
	};

	enum
	{
		LGEO_COLOR_SOLID       = 0x01,
		LGEO_COLOR_TRANSPARENT = 0x02,
		LGEO_COLOR_CHROME      = 0x04,
		LGEO_COLOR_PEARL       = 0x08,
		LGEO_COLOR_METALLIC    = 0x10,
		LGEO_COLOR_RUBBER      = 0x20,
		LGEO_COLOR_GLITTER     = 0x40
	};

	char LGEOPath[LC_MAXPATH];
	strcpy(LGEOPath, lcGetProfileString(LC_PROFILE_POVRAY_LGEO_PATH));

	// Parse LGEO tables.
	if (LGEOPath[0])
	{
		lcDiskFile TableFile, ColorFile;
		char Filename[LC_MAXPATH];

		int Length = strlen(LGEOPath);

		if ((LGEOPath[Length - 1] != '\\') && (LGEOPath[Length - 1] != '/'))
			strcat(LGEOPath, "/");

		strcpy(Filename, LGEOPath);
		strcat(Filename, "lg_elements.lst");

		if (!TableFile.Open(Filename, "rt"))
		{
			delete[] PieceTable;
			delete[] PieceFlags;
			gMainWindow->DoMessageBox("Could not find LGEO files.", LC_MB_OK | LC_MB_ICONERROR);
			return;
		}

		while (TableFile.ReadLine(Line, sizeof(Line)))
		{
			char Src[1024], Dst[1024], Flags[1024];

			if (*Line == ';')
				continue;

			if (sscanf(Line,"%s%s%s", Src, Dst, Flags) != 3)
				continue;

			strupr(Src);

			PieceInfo* Info = Library->FindPiece(Src, false);
			if (!Info)
				continue;

			int Index = Library->mPieces.FindIndex(Info);

			if (strchr(Flags, 'L'))
			{
				PieceFlags[Index] |= LGEO_PIECE_LGEO;
				sprintf(PieceTable + Index * LC_PIECE_NAME_LEN, "lg_%s", Dst);
			}

			if (strchr(Flags, 'A'))
			{
				PieceFlags[Index] |= LGEO_PIECE_AR;
				sprintf(PieceTable + Index * LC_PIECE_NAME_LEN, "ar_%s", Dst);
			}

			if (strchr(Flags, 'S'))
				PieceFlags[Index] |= LGEO_PIECE_SLOPE;
		}

		strcpy(Filename, LGEOPath);
		strcat(Filename, "lg_colors.lst");

		if (!ColorFile.Open(Filename, "rt"))
		{
			delete[] PieceTable;
			delete[] PieceFlags;
			gMainWindow->DoMessageBox("Could not find LGEO files.", LC_MB_OK | LC_MB_ICONERROR);
			return;
		}

		while (ColorFile.ReadLine(Line, sizeof(Line)))
		{
			char Name[1024], Flags[1024];
			int Code;

			if (*Line == ';')
				continue;

			if (sscanf(Line,"%d%s%s", &Code, Name, Flags) != 3)
				continue;

			int Color = lcGetColorIndex(Code);
			if (Color >= NumColors)
				continue;

			strcpy(&ColorTable[Color * LC_MAX_COLOR_NAME], Name);
		}
	}

	const char* OldLocale = setlocale(LC_NUMERIC, "C");

	// Add includes.
	if (LGEOPath[0])
	{
		POVFile.WriteLine("#include \"lg_defs.inc\"\n#include \"lg_color.inc\"\n\n");

		for (Piece* piece = m_pPieces; piece; piece = piece->m_pNext)
		{
			PieceInfo* Info = piece->mPieceInfo;

			for (Piece* FirstPiece = m_pPieces; FirstPiece; FirstPiece = FirstPiece->m_pNext)
			{
				if (FirstPiece->mPieceInfo != Info)
					continue;

				if (FirstPiece != piece)
					break;

				int Index = Library->mPieces.FindIndex(Info);

				if (PieceTable[Index * LC_PIECE_NAME_LEN])
				{
					sprintf(Line, "#include \"%s.inc\"\n", PieceTable + Index * LC_PIECE_NAME_LEN);
					POVFile.WriteLine(Line);
				}

				break;
			}
		}

		POVFile.WriteLine("\n");
	}
	else
		POVFile.WriteLine("#include \"colors.inc\"\n\n");

	// Add color definitions.
	for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
	{
		lcColor* Color = &gColorList[ColorIdx];

		if (lcIsColorTranslucent(ColorIdx))
		{
			sprintf(Line, "#declare lc_%s = texture { pigment { rgb <%f, %f, %f> filter 0.9 } finish { ambient 0.3 diffuse 0.2 reflection 0.25 phong 0.3 phong_size 60 } }\n",
					Color->SafeName, Color->Value[0], Color->Value[1], Color->Value[2]);
		}
		else
		{
			sprintf(Line, "#declare lc_%s = texture { pigment { rgb <%f, %f, %f> } finish { ambient 0.1 phong 0.2 phong_size 20 } }\n",
				   Color->SafeName, Color->Value[0], Color->Value[1], Color->Value[2]);
		}

		POVFile.WriteLine(Line);

		if (!ColorTable[ColorIdx * LC_MAX_COLOR_NAME])
			sprintf(&ColorTable[ColorIdx * LC_MAX_COLOR_NAME], "lc_%s", Color->SafeName);
	}

	POVFile.WriteLine("\n");

	// Add pieces not included in LGEO.
	for (Piece* piece = m_pPieces; piece; piece = piece->m_pNext)
	{
		PieceInfo* Info = piece->mPieceInfo;
		int Index = Library->mPieces.FindIndex(Info);

		if (PieceTable[Index * LC_PIECE_NAME_LEN])
			continue;

		char Name[LC_PIECE_NAME_LEN];
		char* Ptr;

		strcpy(Name, Info->m_strName);
		while ((Ptr = strchr(Name, '-')))
			*Ptr = '_';

		sprintf(PieceTable + Index * LC_PIECE_NAME_LEN, "lc_%s", Name);

		Info->mMesh->ExportPOVRay(POVFile, Name, ColorTable);

		POVFile.WriteLine("}\n\n");

		sprintf(Line, "#declare lc_%s_clear = lc_%s\n\n", Name, Name);
		POVFile.WriteLine(Line);
	}

	lcCamera* camera = gMainWindow->mActiveView->mCamera;
	const lcVector3& Position = camera->mPosition;
	const lcVector3& Target = camera->mTargetPosition;
	const lcVector3& Up = camera->mUpVector;

	sprintf(Line, "camera {\n  sky<%1g,%1g,%1g>\n  location <%1g, %1g, %1g>\n  look_at <%1g, %1g, %1g>\n  angle %.0f\n}\n\n",
			Up[0], Up[1], Up[2], Position[1], Position[0], Position[2], Target[1], Target[0], Target[2], camera->mFOV);
	POVFile.WriteLine(Line);
	sprintf(Line, "background { color rgb <%1g, %1g, %1g> }\n\nlight_source { <0, 0, 20> White shadowless }\n\n",
			m_fBackground[0], m_fBackground[1], m_fBackground[2]);
	POVFile.WriteLine(Line);

	for (Piece* piece = m_pPieces; piece; piece = piece->m_pNext)
	{
		int Index = Library->mPieces.FindIndex(piece->mPieceInfo);
		int Color;

		Color = piece->mColorIndex;
		const char* Suffix = lcIsColorTranslucent(Color) ? "_clear" : "";

		const float* f = piece->mModelWorld;

		if (PieceFlags[Index] & LGEO_PIECE_SLOPE)
		{
			sprintf(Line, "merge {\n object {\n  %s%s\n  texture { %s }\n }\n"
					" object {\n  %s_slope\n  texture { %s normal { bumps 0.3 scale 0.02 } }\n }\n"
					" matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
					PieceTable + Index * LC_PIECE_NAME_LEN, Suffix, &ColorTable[Color * LC_MAX_COLOR_NAME], PieceTable + Index * LC_PIECE_NAME_LEN, &ColorTable[Color * LC_MAX_COLOR_NAME],
				   -f[5], -f[4], -f[6], -f[1], -f[0], -f[2], f[9], f[8], f[10], f[13], f[12], f[14]);
		}
		else
		{
			sprintf(Line, "object {\n %s%s\n texture { %s }\n matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
					PieceTable + Index * LC_PIECE_NAME_LEN, Suffix, &ColorTable[Color * LC_MAX_COLOR_NAME], -f[5], -f[4], -f[6], -f[1], -f[0], -f[2], f[9], f[8], f[10], f[13], f[12], f[14]);
		}

		POVFile.WriteLine(Line);
	}

	delete[] PieceTable;
	delete[] PieceFlags;
	setlocale(LC_NUMERIC, OldLocale);
	POVFile.Close();
	*/
}

void Project::HandleCommand(LC_COMMANDS id)
{
	switch (id)
	{
		case LC_FILE_NEW:
		{
			if (!SaveModified())
				return;  // leave the original one

			OnNewDocument();
			gMainWindow->UpdateAllViews();
		} break;

		case LC_FILE_OPEN:
		{
			char FileName[LC_MAXPATH];

			if (m_strPathName[0])
				strcpy(FileName, m_strPathName);
			else
				strcpy(FileName, lcGetProfileString(LC_PROFILE_PROJECTS_PATH));

			if (gMainWindow->DoDialog(LC_DIALOG_OPEN_PROJECT, FileName))
				OpenProject(FileName);
		} break;

		case LC_FILE_MERGE:
		{
			char FileName[LC_MAXPATH];

			if (m_strPathName[0])
				strcpy(FileName, m_strPathName);
			else
				strcpy(FileName, lcGetProfileString(LC_PROFILE_PROJECTS_PATH));

			if (gMainWindow->DoDialog(LC_DIALOG_MERGE_PROJECT, FileName))
			{
				lcDiskFile file;
				if (file.Open(FileName, "rb"))
				{
					if (file.GetLength() != 0)
					{
						FileLoad(&file, false, true);
						CheckPoint("Merging");
					}
					file.Close();
				}
			}
		} break;

		case LC_FILE_SAVE:
		{
			DoSave(m_strPathName);
		} break;

		case LC_FILE_SAVEAS:
		{
			DoSave(NULL);
		} break;

		case LC_FILE_SAVE_IMAGE:
		{
			lcImageDialogOptions Options;

			int ImageOptions = lcGetProfileInt(LC_PROFILE_IMAGE_OPTIONS);

			Options.Format = (LC_IMAGE_FORMAT)(ImageOptions & ~(LC_IMAGE_MASK));
			Options.Transparent = (ImageOptions & LC_IMAGE_TRANSPARENT) != 0;
			Options.Width = lcGetProfileInt(LC_PROFILE_IMAGE_WIDTH);
			Options.Height = lcGetProfileInt(LC_PROFILE_IMAGE_HEIGHT);

			if (m_strPathName[0])
				strcpy(Options.FileName, m_strPathName);
			else if (m_strTitle[0])
				strcpy(Options.FileName, m_strTitle);
			else
				strcpy(Options.FileName, "Image");

			if (Options.FileName[0])
			{
				char* ext = strrchr(Options.FileName, '.');

				if (ext && (!stricmp(ext, ".lcd") || !stricmp(ext, ".dat") || !stricmp(ext, ".ldr")))
					*ext = 0;

				switch (Options.Format)
				{
				case LC_IMAGE_BMP: strcat(Options.FileName, ".bmp");
					break;
				case LC_IMAGE_JPG: strcat(Options.FileName, ".jpg");
					break;
				default:
				case LC_IMAGE_PNG: strcat(Options.FileName, ".png");
					break;
				}
			}

			if (m_bAnimation)
			{
				Options.Start = 1;
				Options.End = m_nTotalFrames;
			}
			else
			{
				Options.Start = m_nCurStep;
				Options.End = m_nCurStep;
			}

			if (!gMainWindow->DoDialog(LC_DIALOG_SAVE_IMAGE, &Options))
				break;

			ImageOptions = Options.Format;

			if (Options.Transparent)
				ImageOptions |= LC_IMAGE_TRANSPARENT;

			lcSetProfileInt(LC_PROFILE_IMAGE_OPTIONS, ImageOptions);
			lcSetProfileInt(LC_PROFILE_IMAGE_WIDTH, Options.Width);
			lcSetProfileInt(LC_PROFILE_IMAGE_HEIGHT, Options.Height);

			if (!Options.FileName[0])
				strcpy(Options.FileName, "Image");

			char* Ext = strrchr(Options.FileName, '.');
			if (Ext)
			{
				if (!strcmp(Ext, ".jpg") || !strcmp(Ext, ".jpeg") || !strcmp(Ext, ".bmp") || !strcmp(Ext, ".png"))
					*Ext = 0;
			}

			const char* ext;
			switch (Options.Format)
			{
			case LC_IMAGE_BMP: ext = ".bmp";
				break;
			case LC_IMAGE_JPG: ext = ".jpg";
				break;
			default:
			case LC_IMAGE_PNG: ext = ".png";
				break;
			}

			if (m_bAnimation)
				Options.End = lcMin(Options.End, m_nTotalFrames);
			else
				Options.End = lcMin(Options.End, 255);
			Options.Start = lcMax(1, Options.Start);

			if (Options.Start > Options.End)
			{
				if (Options.Start > Options.End)
				{
					int Temp = Options.Start;
					Options.Start = Options.End;
					Options.End = Temp;
				}
			}

			Image* images = new Image[Options.End - Options.Start + 1];
			CreateImages(images, Options.Width, Options.Height, Options.Start, Options.End, false);

			for (int i = 0; i <= Options.End - Options.Start; i++)
			{
				char filename[LC_MAXPATH];

				if (Options.Start != Options.End)
				{
					sprintf(filename, "%s%02d%s", Options.FileName, i+1, ext);
				}
				else
				{
					strcat(Options.FileName, ext);
					strcpy(filename, Options.FileName);
				}

				images[i].FileSave(filename, Options.Format, Options.Transparent);
			}
			delete []images;
		} break;

		case LC_FILE_EXPORT_3DS:
			Export3DStudio();
			break;

		case LC_FILE_EXPORT_HTML:
		{
			lcHTMLDialogOptions Options;

			strcpy(Options.PathName, m_strPathName);

			if (Options.PathName[0] != 0)
			{
				char* Slash = strrchr(Options.PathName, '/');

				if (Slash == NULL)
					Slash = strrchr(Options.PathName, '\\');

				if (Slash)
					{
					Slash++;
					*Slash = 0;
				}
			}

			int ImageOptions = lcGetProfileInt(LC_PROFILE_HTML_IMAGE_OPTIONS);
			int HTMLOptions = lcGetProfileInt(LC_PROFILE_HTML_OPTIONS);

			Options.ImageFormat = (LC_IMAGE_FORMAT)(ImageOptions & ~(LC_IMAGE_MASK));
			Options.TransparentImages = (ImageOptions & LC_IMAGE_TRANSPARENT) != 0;
			Options.SinglePage = (HTMLOptions & LC_HTML_SINGLEPAGE) != 0;
			Options.IndexPage = (HTMLOptions & LC_HTML_INDEX) != 0;
			Options.StepImagesWidth = lcGetProfileInt(LC_PROFILE_HTML_IMAGE_WIDTH);
			Options.StepImagesHeight = lcGetProfileInt(LC_PROFILE_HTML_IMAGE_HEIGHT);
			Options.HighlightNewParts = (HTMLOptions & LC_HTML_HIGHLIGHT) != 0;
			Options.PartsListStep = (HTMLOptions & LC_HTML_LISTSTEP) != 0;
			Options.PartsListEnd = (HTMLOptions & LC_HTML_LISTEND) != 0;
			Options.PartsListImages = (HTMLOptions & LC_HTML_IMAGES) != 0;
			Options.PartImagesColor = lcGetColorIndex(lcGetProfileInt(LC_PROFILE_HTML_PARTS_COLOR));
			Options.PartImagesWidth = lcGetProfileInt(LC_PROFILE_HTML_PARTS_WIDTH);
			Options.PartImagesHeight = lcGetProfileInt(LC_PROFILE_HTML_PARTS_HEIGHT);

			if (!gMainWindow->DoDialog(LC_DIALOG_EXPORT_HTML, &Options))
				break;

			HTMLOptions = 0;

			if (Options.SinglePage)
				HTMLOptions |= LC_HTML_SINGLEPAGE;
			if (Options.IndexPage)
				HTMLOptions |= LC_HTML_INDEX;
			if (Options.HighlightNewParts)
				HTMLOptions |= LC_HTML_HIGHLIGHT;
			if (Options.PartsListStep)
				HTMLOptions |= LC_HTML_LISTSTEP;
			if (Options.PartsListEnd)
				HTMLOptions |= LC_HTML_LISTEND;
			if (Options.PartsListImages)
				HTMLOptions |= LC_HTML_IMAGES;

			ImageOptions = Options.ImageFormat;

			if (Options.TransparentImages)
				ImageOptions |= LC_IMAGE_TRANSPARENT;

			lcSetProfileInt(LC_PROFILE_HTML_IMAGE_OPTIONS, ImageOptions);
			lcSetProfileInt(LC_PROFILE_HTML_OPTIONS, HTMLOptions);
			lcSetProfileInt(LC_PROFILE_HTML_IMAGE_WIDTH, Options.StepImagesWidth);
			lcSetProfileInt(LC_PROFILE_HTML_IMAGE_HEIGHT, Options.StepImagesHeight);
			lcSetProfileInt(LC_PROFILE_HTML_PARTS_COLOR, lcGetColorCode(Options.PartImagesColor));
			lcSetProfileInt(LC_PROFILE_HTML_PARTS_WIDTH, Options.PartImagesWidth);
			lcSetProfileInt(LC_PROFILE_HTML_PARTS_HEIGHT, Options.PartImagesHeight);

			int PathLength = strlen(Options.PathName);
			if (PathLength && Options.PathName[PathLength] != '/' && Options.PathName[PathLength] != '\\')
				strcat(Options.PathName, "/");

			// TODO: create directory

			// TODO: Move to its own function
			{
				FILE* f;
				const char *ext, *htmlext;
				char fn[LC_MAXPATH];
				int i;
				unsigned short last = GetLastStep();

				switch (Options.ImageFormat)
				{
				case LC_IMAGE_BMP: ext = ".bmp";
					break;
				case LC_IMAGE_JPG: ext = ".jpg";
					break;
				default:
				case LC_IMAGE_PNG: ext = ".png";
					break;
				}

				htmlext = ".html";

				if (Options.SinglePage)
				{
					strcpy(fn, Options.PathName);
					strcat(fn, m_strTitle);
					strcat(fn, htmlext);
					f = fopen (fn, "wt");

					if (!f)
					{
						gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK | LC_MB_ICONERROR);
						break;
					}

					fprintf (f, "<HTML>\n<HEAD>\n<TITLE>Instructions for %s</TITLE>\n</HEAD>\n<BR>\n<CENTER>\n", m_strTitle);

					for (i = 1; i <= last; i++)
					{
						fprintf(f, "<IMG SRC=\"%s-%02d%s\" ALT=\"Step %02d\" WIDTH=%d HEIGHT=%d><BR><BR>\n",
							m_strTitle, i, ext, i, Options.StepImagesWidth, Options.StepImagesHeight);

						if (Options.PartsListStep)
							CreateHTMLPieceList(f, i, Options.PartsListImages, ext);
					}

					if (Options.PartsListEnd)
						CreateHTMLPieceList(f, 0, Options.PartsListImages, ext);

					fputs("</CENTER>\n<BR><HR><BR><B><I>Created by <A HREF=\"http://www.leocad.org\">LeoCAD</A></B></I><BR></HTML>\n", f);
					fclose(f);
				}
				else
				{
					if (Options.IndexPage)
					{
						strcpy(fn, Options.PathName);
						strcat (fn, m_strTitle);
						strcat (fn, "-index");
						strcat (fn, htmlext);
						f = fopen (fn, "wt");

						if (!f)
						{
							gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK | LC_MB_ICONERROR);
							break;
						}

						fprintf(f, "<HTML>\n<HEAD>\n<TITLE>Instructions for %s</TITLE>\n</HEAD>\n<BR>\n<CENTER>\n", m_strTitle);

						for (i = 1; i <= last; i++)
							fprintf(f, "<A HREF=\"%s-%02d%s\">Step %d<BR>\n</A>", m_strTitle, i, htmlext, i);

						if (Options.PartsListEnd)
							fprintf(f, "<A HREF=\"%s-pieces%s\">Pieces Used</A><BR>\n", m_strTitle, htmlext);

						fputs("</CENTER>\n<BR><HR><BR><B><I>Created by <A HREF=\"http://www.leocad.org\">LeoCAD</A></B></I><BR></HTML>\n", f);
						fclose(f);
					}

					// Create each step
					for (i = 1; i <= last; i++)
					{
						sprintf(fn, "%s%s-%02d%s", Options.PathName, m_strTitle, i, htmlext);
						f = fopen(fn, "wt");

						if (!f)
						{
							gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK | LC_MB_ICONERROR);
							break;
						}

						fprintf(f, "<HTML>\n<HEAD>\n<TITLE>%s - Step %02d</TITLE>\n</HEAD>\n<BR>\n<CENTER>\n", m_strTitle, i);
						fprintf(f, "<IMG SRC=\"%s-%02d%s\" ALT=\"Step %02d\" WIDTH=%d HEIGHT=%d><BR><BR>\n",
							m_strTitle, i, ext, i, Options.StepImagesWidth, Options.StepImagesHeight);

						if (Options.PartsListStep)
							CreateHTMLPieceList(f, i, Options.PartsListImages, ext);

						fputs("</CENTER>\n<BR><HR><BR>", f);
						if (i != 1)
							fprintf(f, "<A HREF=\"%s-%02d%s\">Previous</A> ", m_strTitle, i-1, htmlext);

						if (Options.IndexPage)
							fprintf(f, "<A HREF=\"%s-index%s\">Index</A> ", m_strTitle, htmlext);

						if (i != last)
							fprintf(f, "<A HREF=\"%s-%02d%s\">Next</A>", m_strTitle, i+1, htmlext);
						else if (Options.PartsListEnd)
								fprintf(f, "<A HREF=\"%s-pieces%s\">Pieces Used</A>", m_strTitle, htmlext);

						fputs("<BR></HTML>\n",f);
						fclose(f);
					}

					if (Options.PartsListEnd)
					{
						strcpy(fn, Options.PathName);
						strcat (fn, m_strTitle);
						strcat (fn, "-pieces");
						strcat (fn, htmlext);
						f = fopen (fn, "wt");

						if (!f)
						{
							gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK | LC_MB_ICONERROR);
							break;
						}

						fprintf (f, "<HTML>\n<HEAD>\n<TITLE>Pieces used by %s</TITLE>\n</HEAD>\n<BR>\n<CENTER>\n", m_strTitle);

						CreateHTMLPieceList(f, 0, Options.PartsListImages, ext);

						fputs("</CENTER>\n<BR><HR><BR>", f);
						fprintf(f, "<A HREF=\"%s-%02d%s\">Previous</A> ", m_strTitle, i-1, htmlext);

						if (Options.IndexPage)
							fprintf(f, "<A HREF=\"%s-index%s\">Index</A> ", m_strTitle, htmlext);

						fputs("<BR></HTML>\n",f);
						fclose(f);
					}
				}

				// Save step pictures
				Image* images = new Image[last];
				CreateImages(images, Options.StepImagesWidth, Options.StepImagesHeight, 1, last, Options.HighlightNewParts);

				for (i = 0; i < last; i++)
				{
					sprintf(fn, "%s%s-%02d%s", Options.PathName, m_strTitle, i+1, ext);
					images[i].FileSave(fn, Options.ImageFormat, Options.TransparentImages);
				}
				delete []images;

				if (Options.PartsListImages)
				{
					int cx = Options.PartImagesWidth, cy = Options.PartImagesHeight;
					if (!GL_BeginRenderToTexture(cx, cy))
					{
						gMainWindow->DoMessageBox("Error creating images.", LC_MB_ICONERROR | LC_MB_OK);
						break;
					}

					float aspect = (float)cx/(float)cy;
					glViewport(0, 0, cx, cy);

					Piece *p1, *p2;
					PieceInfo* pInfo;
					for (p1 = m_pPieces; p1; p1 = p1->m_pNext)
					{
						bool bSkip = false;
						pInfo = p1->mPieceInfo;

						for (p2 = m_pPieces; p2; p2 = p2->m_pNext)
						{
							if (p2 == p1)
								break;

							if (p2->mPieceInfo == pInfo)
							{
								bSkip = true;
								break;
							}
						}

						if (bSkip)
							continue;

						glDepthFunc(GL_LEQUAL);
						glEnable(GL_DEPTH_TEST);
						glEnable(GL_POLYGON_OFFSET_FILL);
						glPolygonOffset(0.5f, 0.1f);
						glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
						pInfo->ZoomExtents(30.0f, aspect);
						pInfo->RenderPiece(Options.PartImagesColor);
						glFinish();

						Image image;
						image.FromOpenGL (cx, cy);

						sprintf(fn, "%s%s%s", Options.PathName, pInfo->m_strName, ext);
						image.FileSave(fn, Options.ImageFormat, Options.TransparentImages);
					}
					GL_EndRenderToTexture();
				}
			}
		} break;

	case LC_FILE_EXPORT_BRICKLINK:
		ExportBrickLink();
		break;

	case LC_FILE_EXPORT_CSV:
		ExportCSV();
		break;

		case LC_FILE_EXPORT_POVRAY:
		{
			lcPOVRayDialogOptions Options;

			memset(Options.FileName, 0, sizeof(Options.FileName));
			strcpy(Options.POVRayPath, lcGetProfileString(LC_PROFILE_POVRAY_PATH));
			strcpy(Options.LGEOPath, lcGetProfileString(LC_PROFILE_POVRAY_LGEO_PATH));
			Options.Render = lcGetProfileInt(LC_PROFILE_POVRAY_RENDER);

			if (!gMainWindow->DoDialog(LC_DIALOG_EXPORT_POVRAY, &Options))
				break;

			lcSetProfileString(LC_PROFILE_POVRAY_PATH, Options.POVRayPath);
			lcSetProfileString(LC_PROFILE_POVRAY_LGEO_PATH, Options.LGEOPath);
			lcSetProfileInt(LC_PROFILE_POVRAY_RENDER, Options.Render);

			lcDiskFile POVFile;

			if (!POVFile.Open(Options.FileName, "wt"))
			{
				gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK|LC_MB_ICONERROR);
				break;
			}

			ExportPOVRay(POVFile);

			if (Options.Render)
			{
				lcArray<String> Arguments;
				char Argument[LC_MAXPATH + 32];

				sprintf(Argument, "+I%s", Options.FileName);
				Arguments.Add(Argument);

				if (Options.LGEOPath[0])
				{
					sprintf(Argument, "+L%slg/", Options.LGEOPath);
					Arguments.Add(Argument);
					sprintf(Argument, "+L%sar/", Options.LGEOPath);
					Arguments.Add(Argument);
				}

				sprintf(Argument, "+o%s", Options.FileName);
				char* Slash1 = strrchr(Argument, '\\');
				char* Slash2 = strrchr(Argument, '/');
				if (Slash1 || Slash2)
				{
					if (Slash1 > Slash2)
						*(Slash1 + 1) = 0;
					else
						*(Slash2 + 1) = 0;

					Arguments.Add(Argument);
				}

				g_App->RunProcess(Options.POVRayPath, Arguments);
			}
		} break;

		case LC_FILE_EXPORT_WAVEFRONT:
		{
			char FileName[LC_MAXPATH];
			memset(FileName, 0, sizeof(FileName));

			if (!gMainWindow->DoDialog(LC_DIALOG_EXPORT_WAVEFRONT, FileName))
				break;

			lcDiskFile OBJFile;
			char Line[1024];

			if (!OBJFile.Open(FileName, "wt"))
			{
				gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK|LC_MB_ICONERROR);
				break;
			}

			char buf[LC_MAXPATH], *ptr;
			lcuint32 vert = 1;
			Piece* pPiece;

			const char* OldLocale = setlocale(LC_NUMERIC, "C");
			strcpy(buf, m_strPathName);
			ptr = strrchr(buf, '\\');
			if (ptr)
				ptr++;
			else
			{
				ptr = strrchr(buf, '/');
				if (ptr)
					ptr++;
				else
					ptr = buf;
			}

			OBJFile.WriteLine("# Model exported from LeoCAD\n");

			if (strlen(buf) != 0)
			{
				sprintf(Line, "# Original name: %s\n", ptr);
				OBJFile.WriteLine(Line);
			}

			if (strlen(m_strAuthor))
			{
				sprintf(Line, "# Author: %s\n", m_strAuthor);
				OBJFile.WriteLine(Line);
			}

			strcpy(buf, FileName);
			ptr = strrchr(buf, '.');
			if (ptr)
				*ptr = 0;

			strcat(buf, ".mtl");
			ptr = strrchr(buf, '\\');
			if (ptr)
				ptr++;
			else
			{
				ptr = strrchr(buf, '/');
				if (ptr)
					ptr++;
				else
					ptr = buf;
			}

			sprintf(Line, "#\n\nmtllib %s\n\n", ptr);
			OBJFile.WriteLine(Line);

			FILE* mat = fopen(buf, "wt");
			fputs("# Colors used by LeoCAD\n# You need to add transparency values\n#\n\n", mat);
			for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
			{
				lcColor* Color = &gColorList[ColorIdx];
				fprintf(mat, "newmtl %s\nKd %.2f %.2f %.2f\n\n", Color->SafeName, Color->Value[0], Color->Value[1], Color->Value[2]);
			}
			fclose(mat);

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				const lcMatrix44& ModelWorld = pPiece->mModelWorld;
				PieceInfo* pInfo = pPiece->mPieceInfo;
				float* Verts = (float*)pInfo->mMesh->mVertexBuffer.mData;

				for (int i = 0; i < pInfo->mMesh->mNumVertices * 3; i += 3)
				{
					lcVector3 Vertex = lcMul31(lcVector3(Verts[i], Verts[i+1], Verts[i+2]), ModelWorld);
					sprintf(Line, "v %.2f %.2f %.2f\n", Vertex[0], Vertex[1], Vertex[2]);
					OBJFile.WriteLine(Line);
				}

				OBJFile.WriteLine("#\n\n");
			}

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				PieceInfo* Info = pPiece->mPieceInfo;

				strcpy(buf, pPiece->GetName());
				for (unsigned int i = 0; i < strlen(buf); i++)
					if ((buf[i] == '#') || (buf[i] == ' '))
						buf[i] = '_';

				sprintf(Line, "g %s\n", buf);
				OBJFile.WriteLine(Line);

				Info->mMesh->ExportWavefrontIndices(OBJFile, pPiece->mColorCode, vert);
				vert += Info->mMesh->mNumVertices;
			}

			setlocale(LC_NUMERIC, OldLocale);
		} break;

	case LC_FILE_PRINT_PREVIEW:
		gMainWindow->TogglePrintPreview();
		break;

	case LC_FILE_PRINT:
		gMainWindow->DoDialog(LC_DIALOG_PRINT, NULL);
		break;

		// TODO: printing
		case LC_FILE_PRINT_BOM:
			break;

		case LC_FILE_TERRAIN_EDITOR:
		{
			// TODO: decide what to do with the terrain editor
//			Terrain temp = *m_pTerrain;

//			if (SystemDoDialog(LC_DLG_TERRAIN, temp))
//			{
//				*m_pTerrain = temp;
//				m_pTerrain->LoadTexture();
//			}
		} break;

	case LC_FILE_RECENT1:
	case LC_FILE_RECENT2:
	case LC_FILE_RECENT3:
	case LC_FILE_RECENT4:
		if (!OpenProject(gMainWindow->mRecentFiles[id - LC_FILE_RECENT1]))
			gMainWindow->RemoveRecentFile(id - LC_FILE_RECENT1);
		break;

	case LC_FILE_EXIT:
		gMainWindow->Close();
		break;

	case LC_EDIT_UNDO:
		mActiveModel->UndoCheckpoint();
		break;

	case LC_EDIT_REDO:
		mActiveModel->RedoCheckpoint();
		break;

	case LC_EDIT_COPY:
		mActiveModel->CopyToClipboard();
		break;

	case LC_EDIT_CUT:
		mActiveModel->CopyToClipboard();
		mActiveModel->RemoveObjects(mActiveModel->GetSelectedObjects());
		break;

	case LC_EDIT_PASTE:
		mActiveModel->PasteFromClipboard();
		break;

		case LC_EDIT_FIND:
			if (gMainWindow->DoDialog(LC_DIALOG_FIND, &mSearchOptions))
				FindPiece(true, true);
			break;

		case LC_EDIT_FIND_NEXT:
			FindPiece(false, true);
			break;

		case LC_EDIT_FIND_PREVIOUS:
			FindPiece(false, false);
			break;

	case LC_EDIT_SELECT_ALL:
		mActiveModel->SelectAllObjects();
		break;

	case LC_EDIT_SELECT_NONE:
		mActiveModel->ClearSelection();
		break;

	case LC_EDIT_SELECT_INVERT:
		mActiveModel->InvertSelection();
		break;

		case LC_EDIT_SELECT_BY_NAME:
		{
			lcSelectDialogOptions Options;

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				Options.Selection.Add(pPiece->IsSelected());

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				if (mCameras[CameraIdx]->IsVisible())
					Options.Selection.Add(mCameras[CameraIdx]->IsSelected());

			for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				if (pLight->IsVisible())
					Options.Selection.Add(pLight->IsSelected());

			if (Options.Selection.GetSize() == 0)
			{
				gMainWindow->DoMessageBox("Nothing to select.", LC_MB_OK | LC_MB_ICONINFORMATION);
				break;
			}

			if (!gMainWindow->DoDialog(LC_DIALOG_SELECT_BY_NAME, &Options))
				break;

			SelectAndFocusNone(false);

			int ObjectIndex = 0;
			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext, ObjectIndex++)
				if (Options.Selection[ObjectIndex])
					pPiece->Select(true, false, false);

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++, ObjectIndex++)
				if (Options.Selection[ObjectIndex])
					mCameras[CameraIdx]->Select(true, false, false);

			for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext, ObjectIndex++)
				if (Options.Selection[ObjectIndex])
					pLight->Select(true, false, false);

			UpdateSelection();
			gMainWindow->UpdateAllViews();
//	pFrame->UpdateInfo();
		} break;

	case LC_VIEW_SPLIT_HORIZONTAL:
		gMainWindow->SplitHorizontal();
		break;

	case LC_VIEW_SPLIT_VERTICAL:
		gMainWindow->SplitVertical();
		break;

	case LC_VIEW_REMOVE_VIEW:
		gMainWindow->RemoveView();
		break;

	case LC_VIEW_RESET_VIEWS:
		gMainWindow->ResetViews();
		break;

	case LC_VIEW_FULLSCREEN:
		gMainWindow->ToggleFullScreen();
		break;

	case LC_PIECE_INSERT:
		mActiveModel->AddPiece(m_pCurPiece, gMainWindow->mColorIndex);
		break;

	case LC_PIECE_DELETE:
		mActiveModel->RemoveObjects(mActiveModel->GetSelectedObjects());
		break;

	case LC_PIECE_MOVE_PLUSX:
	case LC_PIECE_MOVE_MINUSX:
	case LC_PIECE_MOVE_PLUSY:
	case LC_PIECE_MOVE_MINUSY:
	case LC_PIECE_MOVE_PLUSZ:
	case LC_PIECE_MOVE_MINUSZ:
	case LC_PIECE_ROTATE_PLUSX:
	case LC_PIECE_ROTATE_MINUSX:
	case LC_PIECE_ROTATE_PLUSY:
	case LC_PIECE_ROTATE_MINUSY:
	case LC_PIECE_ROTATE_PLUSZ:
	case LC_PIECE_ROTATE_MINUSZ:
		{
			lcVector3 axis;
			bool Rotate = id >= LC_PIECE_ROTATE_PLUSX && id <= LC_PIECE_ROTATE_MINUSZ;

			if (Rotate)
			{
				if (m_nSnap & LC_DRAW_SNAP_A)
					axis[0] = axis[1] = axis[2] = m_nAngleSnap;
				else
					axis[0] = axis[1] = axis[2] = 1;
			}
			else
			{
				float xy, z;
				GetSnapDistance(&xy, &z);

				axis[0] = axis[1] = xy;
				axis[2] = z;

				if ((m_nSnap & LC_DRAW_SNAP_X) == 0)// || bControl)
					axis[0] = 0.01f;
				if ((m_nSnap & LC_DRAW_SNAP_Y) == 0)// || bControl)
					axis[1] = 0.01f;
				if ((m_nSnap & LC_DRAW_SNAP_Z) == 0)// || bControl)
					axis[2] = 0.01f;
			}

			if (id == LC_PIECE_MOVE_PLUSX || id ==  LC_PIECE_ROTATE_PLUSX)
				axis = lcVector3(axis[0], 0, 0);
			else if (id == LC_PIECE_MOVE_MINUSX || id == LC_PIECE_ROTATE_MINUSX)
				axis = lcVector3(-axis[0], 0, 0);
			else if (id == LC_PIECE_MOVE_PLUSY || id == LC_PIECE_ROTATE_PLUSY)
				axis = lcVector3(0, axis[1], 0);
			else if (id == LC_PIECE_MOVE_MINUSY || id == LC_PIECE_ROTATE_MINUSY)
				axis = lcVector3(0, -axis[1], 0);
			else if (id == LC_PIECE_MOVE_PLUSZ || id == LC_PIECE_ROTATE_PLUSZ)
				axis = lcVector3(0, 0, axis[2]);
			else if (id == LC_PIECE_MOVE_MINUSZ || id == LC_PIECE_ROTATE_MINUSZ)
				axis = lcVector3(0, 0, -axis[2]);

			if ((m_nSnap & LC_DRAW_MOVEAXIS) == 0)
			{
				// TODO: rewrite this

				View* ActiveView = gMainWindow->mActiveView;
				int Viewport[4] = { 0, 0, ActiveView->mWidth, ActiveView->mHeight };
				float Aspect = (float)Viewport[2]/(float)Viewport[3];
				lcCamera* Camera = ActiveView->mCamera;

				const lcMatrix44& ModelView = Camera->mWorldView;
				lcMatrix44 Projection = lcMatrix44Perspective(Camera->mFOV, Aspect, Camera->mNear, Camera->mFar);

				lcVector3 Pts[3] = { lcVector3(5.0f, 5.0f, 0.1f), lcVector3(10.0f, 5.0f, 0.1f), lcVector3(5.0f, 10.0f, 0.1f) };
				lcUnprojectPoints(Pts, 3, ModelView, Projection, Viewport);

				float ax, ay;
				lcVector3 vx((Pts[1][0] - Pts[0][0]), (Pts[1][1] - Pts[0][1]), 0);//Pts[1][2] - Pts[0][2] };
				vx.Normalize();
				lcVector3 x(1, 0, 0);
				ax = acosf(lcDot(vx, x));

				lcVector3 vy((Pts[2][0] - Pts[0][0]), (Pts[2][1] - Pts[0][1]), 0);//Pts[2][2] - Pts[0][2] };
				vy.Normalize();
				lcVector3 y(0, -1, 0);
				ay = acosf(lcDot(vy, y));

				if (ax > 135)
					axis[0] = -axis[0];

				if (ay < 45)
					axis[1] = -axis[1];

				if (ax >= 45 && ax <= 135)
				{
					float tmp = axis[0];

					ax = acosf(lcDot(vx, y));
					if (ax > 90)
					{
						axis[0] = -axis[1];
						axis[1] = tmp;
					}
					else
					{
						axis[0] = axis[1];
						axis[1] = -tmp;
					}
				}
			}

			if (Rotate)
			{
				lcVector3 tmp;
				RotateSelectedObjects(axis, tmp, true, true);
			}
			else
			{
				mActiveModel->BeginMoveTool();
				mActiveModel->UpdateMoveTool(GetMoveDistance(axis, false, true), gMainWindow->mAddKeys);
				mActiveModel->EndMoveTool(true);
			}

			gMainWindow->UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint(Rotate ? "Rotating" : "Moving");
			gMainWindow->UpdateFocusObject();
		} break;

		case LC_PIECE_MINIFIG_WIZARD:
		{
			lcMinifig Minifig;
			int i;

			if (!gMainWindow->DoDialog(LC_DIALOG_MINIFIG, &Minifig))
				break;

				SelectAndFocusNone(false);

				for (i = 0; i < LC_MFW_NUMITEMS; i++)
				{
					if (Minifig.Parts[i] == NULL)
						continue;

					Piece* pPiece = new Piece(Minifig.Parts[i]);

					lcVector4& Position = Minifig.Matrices[i][3];
					lcVector4 Rotation = lcMatrix44ToAxisAngle(Minifig.Matrices[i]);
					Rotation[3] *= LC_RTOD;
					pPiece->Initialize(Position[0], Position[1], Position[2], m_nCurStep, m_nCurStep);
					pPiece->SetColorIndex(Minifig.Colors[i]);
					pPiece->CreateName(m_pPieces);
					AddPiece(pPiece);
					pPiece->Select(true, false, false);

					pPiece->ChangeKey(1, false, false, Rotation, LC_PK_ROTATION);
					pPiece->ChangeKey(1, true, false, Rotation, LC_PK_ROTATION);
					pPiece->UpdatePosition(m_nCurStep, m_bAnimation);

					SystemPieceComboAdd(Minifig.Parts[i]->m_strDescription);
				}

				float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
				int max = 0;

				Group* pGroup;
				for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
					if (strncmp (pGroup->m_strName, "Minifig #", 9) == 0)
						if (sscanf(pGroup->m_strName, "Minifig #%d", &i) == 1)
							if (i > max)
								max = i;
				pGroup = new Group;
				sprintf(pGroup->m_strName, "Minifig #%.2d", max+1);

				pGroup->m_pNext = m_pGroups;
				m_pGroups = pGroup;

				for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
					if (pPiece->IsSelected())
					{
						pPiece->SetGroup(pGroup);
						pPiece->CompareBoundingBox(bs);
					}

				pGroup->m_fCenter[0] = (bs[0]+bs[3])/2;
				pGroup->m_fCenter[1] = (bs[1]+bs[4])/2;
				pGroup->m_fCenter[2] = (bs[2]+bs[5])/2;

				gMainWindow->UpdateFocusObject();
				UpdateSelection();
				gMainWindow->UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Minifig");
		} break;

		case LC_PIECE_ARRAY:
		{
			Piece *pPiece, *pFirst = NULL, *pLast = NULL;
			float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
			int sel = 0;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if (pPiece->IsSelected())
			{
					pPiece->CompareBoundingBox(bs);
					sel++;
				}
			}

			if (!sel)
				{
				gMainWindow->DoMessageBox("No pieces selected.", LC_MB_OK | LC_MB_ICONINFORMATION);
					break;
				}

			lcArrayDialogOptions Options;

			memset(&Options, 0, sizeof(Options));
			Options.Counts[0] = 10;
			Options.Counts[1] = 1;
			Options.Counts[2] = 1;

			if (!gMainWindow->DoDialog(LC_DIALOG_PIECE_ARRAY, &Options))
				break;

			if (Options.Counts[0] * Options.Counts[1] * Options.Counts[2] < 2)
			{
				gMainWindow->DoMessageBox("Array only has 1 element or less, no pieces added.", LC_MB_OK | LC_MB_ICONINFORMATION);
				break;
			}

			ConvertFromUserUnits(Options.Offsets[0]);
			ConvertFromUserUnits(Options.Offsets[1]);
			ConvertFromUserUnits(Options.Offsets[2]);

			for (int Step1 = 0; Step1 < Options.Counts[0]; Step1++)
			{
				for (int Step2 = 0; Step2 < Options.Counts[1]; Step2++)
				{
					for (int Step3 = 0; Step3 < Options.Counts[2]; Step3++)
					{
						if (Step1 == 0 && Step2 == 0 && Step3 == 0)
							continue;

						lcMatrix44 ModelWorld;
						lcVector3 Position;

						lcVector3 RotationAngles = Options.Rotations[0] * Step1 + Options.Rotations[1] * Step2 + Options.Rotations[2] * Step3;
						lcVector3 Offset = Options.Offsets[0] * Step1 + Options.Offsets[1] * Step2 + Options.Offsets[2] * Step3;

				for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				{
					if (!pPiece->IsSelected())
						continue;

						if (sel == 1)
						{
								ModelWorld = lcMul(pPiece->mModelWorld, lcMatrix44RotationX(RotationAngles[0] * LC_DTOR));
								ModelWorld = lcMul(ModelWorld, lcMatrix44RotationY(RotationAngles[1] * LC_DTOR));
								ModelWorld = lcMul(ModelWorld, lcMatrix44RotationZ(RotationAngles[2] * LC_DTOR));

							Position = pPiece->mPosition;
						}
						else
						{
							lcVector4 Center((bs[0] + bs[3]) / 2, (bs[1] + bs[4]) / 2, (bs[2] + bs[5]) / 2, 0.0f);
							ModelWorld = pPiece->mModelWorld;

							ModelWorld.r[3] -= Center;
								ModelWorld = lcMul(ModelWorld, lcMatrix44RotationX(RotationAngles[0] * LC_DTOR));
								ModelWorld = lcMul(ModelWorld, lcMatrix44RotationY(RotationAngles[1] * LC_DTOR));
								ModelWorld = lcMul(ModelWorld, lcMatrix44RotationZ(RotationAngles[2] * LC_DTOR));
							ModelWorld.r[3] += Center;

							Position = lcVector3(ModelWorld.r[3].x, ModelWorld.r[3].y, ModelWorld.r[3].z);
						}

						lcVector4 AxisAngle = lcMatrix44ToAxisAngle(ModelWorld);
						AxisAngle[3] *= LC_RTOD;

								if (pLast)
								{
									pLast->m_pNext = new Piece(pPiece->mPieceInfo);
									pLast = pLast->m_pNext;
								}
								else
									pLast = pFirst = new Piece(pPiece->mPieceInfo);

							pLast->Initialize(Position[0] + Offset[0], Position[1] + Offset[1], Position[2] + Offset[2], m_nCurStep, m_nCurStep);
								pLast->SetColorIndex(pPiece->mColorIndex);
								pLast->ChangeKey(1, false, false, AxisAngle, LC_PK_ROTATION);
								pLast->ChangeKey(1, true, false, AxisAngle, LC_PK_ROTATION);
							}
						}
					}
				}

				while (pFirst)
				{
					pPiece = pFirst->m_pNext;
					pFirst->CreateName(m_pPieces);
					pFirst->UpdatePosition(m_nCurStep, m_bAnimation);
					AddPiece(pFirst);
					pFirst = pPiece;
				}

				SelectAndFocusNone(true);
//			gMainWindow->UpdateFocusObject();
				UpdateSelection();
				gMainWindow->UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Array");
		} break;

	case LC_PIECE_GROUP:
//		mActiveModel->GroupSelectedObjects();
		break;
/*
		{
			Group* pGroup;
			int i, max = 0;
			char name[65];
			int Selected = 0;

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if (pPiece->IsSelected())
				{
					Selected++;

					if (Selected > 1)
						break;
				}
			}

			if (!Selected)
			{
				gMainWindow->DoMessageBox("No pieces selected.", LC_MB_OK | LC_MB_ICONINFORMATION);
				break;
			}

			for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
				if (strncmp (pGroup->m_strName, "Group #", 7) == 0)
					if (sscanf(pGroup->m_strName, "Group #%d", &i) == 1)
						if (i > max)
							max = i;
			sprintf(name, "Group #%.2d", max+1);

			if (!gMainWindow->DoDialog(LC_DIALOG_PIECE_GROUP, name))
				break;

				pGroup = new Group();
				strcpy(pGroup->m_strName, name);
				pGroup->m_pNext = m_pGroups;
				m_pGroups = pGroup;
				float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };

				for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
					if (pPiece->IsSelected())
					{
						pPiece->DoGroup(pGroup);
						pPiece->CompareBoundingBox(bs);
					}

				pGroup->m_fCenter[0] = (bs[0]+bs[3])/2;
				pGroup->m_fCenter[1] = (bs[1]+bs[4])/2;
				pGroup->m_fCenter[2] = (bs[2]+bs[5])/2;

				RemoveEmptyGroups();
				SetModifiedFlag(true);
				CheckPoint("Grouping");
		} break;
*/
		case LC_PIECE_UNGROUP:
		{
			Group* pList = NULL;
			Group* pGroup;
			Group* tmp;
			Piece* pPiece;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
				{
					pGroup = pPiece->GetTopGroup();

					// Check if we already removed the group
					for (tmp = pList; tmp; tmp = tmp->m_pNext)
						if (pGroup == tmp)
							pGroup = NULL;

					if (pGroup != NULL)
					{
						// First remove the group from the array
						for (tmp = m_pGroups; tmp->m_pNext; tmp = tmp->m_pNext)
							if (tmp->m_pNext == pGroup)
							{
								tmp->m_pNext = pGroup->m_pNext;
								break;
							}

						if (pGroup == m_pGroups)
							m_pGroups = pGroup->m_pNext;

						// Now add it to the list of top groups
						pGroup->m_pNext = pList;
						pList = pGroup;
					}
				}

			while (pList)
			{
				for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
					if (pPiece->IsSelected())
						pPiece->UnGroup(pList);

				pGroup = pList;
				pList = pList->m_pNext;
				delete pGroup;
			}

			RemoveEmptyGroups();
			SetModifiedFlag(true);
			CheckPoint("Ungrouping");
		} break;

		case LC_PIECE_GROUP_ADD:
		{
			Group* pGroup = NULL;
			Piece* pPiece;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
				{
					pGroup = pPiece->GetTopGroup();
					if (pGroup != NULL)
						break;
				}

			if (pGroup != NULL)
			{
				for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
					if (pPiece->IsFocused())
					{
						pPiece->SetGroup(pGroup);
						break;
					}
			}

			RemoveEmptyGroups();
			SetModifiedFlag(true);
			CheckPoint("Grouping");
		} break;

		case LC_PIECE_GROUP_REMOVE:
		{
			Piece* pPiece;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsFocused())
				{
					pPiece->UnGroup(NULL);
					break;
				}

			RemoveEmptyGroups();
			SetModifiedFlag(true);
			CheckPoint("Ungrouping");
		} break;

		case LC_PIECE_GROUP_EDIT:
		{
			lcEditGroupsDialogOptions Options;

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				Options.PieceParents.Add(pPiece->GetGroup());

			for (Group* pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
				Options.GroupParents.Add(pGroup->m_pGroup);

			if (!gMainWindow->DoDialog(LC_DIALOG_EDIT_GROUPS, &Options))
				break;

			int PieceIdx = 0;
			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				pPiece->SetGroup(Options.PieceParents[PieceIdx++]);

			int GroupIdx = 0;
			for (Group* pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
				pGroup->m_pGroup = Options.GroupParents[GroupIdx++];

				RemoveEmptyGroups();
				SelectAndFocusNone(false);
			gMainWindow->UpdateFocusObject();
				UpdateSelection();
				gMainWindow->UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Editing");
		} break;

	case LC_PIECE_HIDE_SELECTED:
		mActiveModel->HideSelectedObjects();
		break;

	case LC_PIECE_HIDE_UNSELECTED:
		mActiveModel->HideUnselectedObjects();
		break;

	case LC_PIECE_UNHIDE_ALL:
		mActiveModel->UnhideAllObjects();
		break;

		case LC_PIECE_SHOW_EARLIER:
		{
			bool redraw = false;

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
				{
					if (m_bAnimation)
					{
						unsigned short t = pPiece->GetFrameShow();
						if (t > 1)
						{
							redraw = true;
							pPiece->SetFrameShow(t-1);
						}
					}
					else
					{
						unsigned char t = pPiece->GetStepShow();
						if (t > 1)
						{
							redraw = true;
							pPiece->SetStepShow(t-1);
						}
					}
				}

			if (redraw)
			{
				SetModifiedFlag(true);
				CheckPoint("Modifying");
				gMainWindow->UpdateAllViews();
			}
		} break;

		case LC_PIECE_SHOW_LATER:
		{
			bool redraw = false;

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
				{
					unsigned char t = pPiece->GetStepShow();
					if (t < 255)
					{
						redraw = true;
						pPiece->SetStepShow(t+1);

						if (pPiece->IsSelected () && t == m_nCurStep)
							pPiece->Select (false, false, false);
					}
				}

			if (redraw)
			{
				SetModifiedFlag(true);
				CheckPoint("Modifying");
				gMainWindow->UpdateAllViews();
				UpdateSelection ();
			}
		} break;

	case LC_VIEW_PREFERENCES:
		g_App->ShowPreferencesDialog();
		break;

	case LC_VIEW_ZOOM_IN:
		ZoomActiveView(-1);
		break;

	case LC_VIEW_ZOOM_OUT:
		ZoomActiveView(1);
		break;

		case LC_VIEW_ZOOM_EXTENTS:
		{
			int FirstView, LastView;

			// TODO: Find a way to let users zoom extents all views
//			if (Sys_KeyDown(KEY_CONTROL))
//			{
//				FirstView = 0;
//				LastView = gMainWindow->mViews.GetSize();
//			}
//			else
			{
				FirstView = gMainWindow->mViews.FindIndex(gMainWindow->mActiveView);
				LastView = FirstView + 1;
			}

			ZoomExtents(FirstView, LastView);
		} break;

	case LC_VIEW_TIME_NEXT:
		mActiveModel->SetCurrentTime(mActiveModel->GetCurrentTime() + 1);
		break;

	case LC_VIEW_TIME_PREVIOUS:
		mActiveModel->SetCurrentTime(mActiveModel->GetCurrentTime() - 1);
		break;

	case LC_VIEW_TIME_FIRST:
		mActiveModel->SetCurrentTime(1);
		break;

	case LC_VIEW_TIME_LAST:
		mActiveModel->SetCurrentTime(mActiveModel->GetTotalTime());
		break;

		case LC_VIEW_TIME_INSERT:
		{
			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				pPiece->InsertTime(m_nCurStep, m_bAnimation, 1);

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				mCameras[CameraIdx]->InsertTime(m_nCurStep, m_bAnimation, 1);

			for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				pLight->InsertTime(m_nCurStep, m_bAnimation, 1);

			SetModifiedFlag(true);
			CheckPoint("Adding Step");
			CalculateStep();
			gMainWindow->UpdateAllViews();
			UpdateSelection();
		} break;

		case LC_VIEW_TIME_DELETE:
		{
			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				pPiece->RemoveTime(m_nCurStep, m_bAnimation, 1);

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				mCameras[CameraIdx]->RemoveTime(m_nCurStep, m_bAnimation, 1);

			for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				pLight->RemoveTime(m_nCurStep, m_bAnimation, 1);

			SetModifiedFlag (true);
			CheckPoint("Removing Step");
			CalculateStep();
			gMainWindow->UpdateAllViews();
			UpdateSelection();
		} break;

	case LC_VIEW_VIEWPOINT_FRONT:
	case LC_VIEW_VIEWPOINT_BACK:
	case LC_VIEW_VIEWPOINT_TOP:
	case LC_VIEW_VIEWPOINT_BOTTOM:
	case LC_VIEW_VIEWPOINT_LEFT:
	case LC_VIEW_VIEWPOINT_RIGHT:
	case LC_VIEW_VIEWPOINT_HOME:
		{
			View* ActiveView = gMainWindow->mActiveView;

			if (!ActiveView)
				break;

			ActiveView->SetDefaultCamera();
			ActiveView->mCamera->SetViewpoint(lcViewpoint(LC_VIEWPOINT_FRONT + id - LC_VIEW_VIEWPOINT_FRONT));

			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
		}
		break;

	case LC_VIEW_CAMERA_NONE:
	case LC_VIEW_CAMERA1:
	case LC_VIEW_CAMERA2:
	case LC_VIEW_CAMERA3:
	case LC_VIEW_CAMERA4:
	case LC_VIEW_CAMERA5:
	case LC_VIEW_CAMERA6:
	case LC_VIEW_CAMERA7:
	case LC_VIEW_CAMERA8:
	case LC_VIEW_CAMERA9:
	case LC_VIEW_CAMERA10:
	case LC_VIEW_CAMERA11:
	case LC_VIEW_CAMERA12:
	case LC_VIEW_CAMERA13:
	case LC_VIEW_CAMERA14:
	case LC_VIEW_CAMERA15:
	case LC_VIEW_CAMERA16:
		{
			View* ActiveView = gMainWindow->mActiveView;
			lcCamera* Camera = NULL;

			if (!ActiveView)
				break;

			if (id == LC_VIEW_CAMERA_NONE)
			{
				Camera = ActiveView->mCamera;

				if (!Camera->IsSimple())
				{
					ActiveView->SetCamera(Camera, true);
					Camera = ActiveView->mCamera;
				}
			}
			else
			{
				Camera = mActiveModel->GetCamera(id - LC_VIEW_CAMERA1);
				if (Camera)
					ActiveView->SetCamera(Camera, false);
			}

			gMainWindow->UpdateCameraMenu();
			gMainWindow->UpdateAllViews();
		}
		break;

		case LC_VIEW_CAMERA_RESET:
		{
			for (int ViewIdx = 0; ViewIdx < gMainWindow->mViews.GetSize(); ViewIdx++)
				gMainWindow->mViews[ViewIdx]->SetDefaultCamera();

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				delete mCameras[CameraIdx];
			mCameras.RemoveAll();

			gMainWindow->UpdateCameraMenu();
			gMainWindow->UpdateFocusObject();
			gMainWindow->UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint("Reset Cameras");
		} break;

	case LC_MODEL_PROPERTIES:
		mActiveModel->ShowPropertiesDialog();
		break;

	case LC_MODEL_NEW:
		{
			lcPropertiesDialogOptions Options;

			const char* Prefix = "Model ";
			const int PrefixLength = strlen(Prefix);
			int Index, MaxIndex = 0;

			for (int ModelIdx = 0; ModelIdx < mModels.GetSize(); ModelIdx++)
			{
				lcModel* Model = mModels[ModelIdx];

				if (strncmp(Model->mProperties.mName, Prefix, PrefixLength))
					continue;

				if (sscanf((const char*)Model->mProperties.mName + PrefixLength, "%d", &Index) != 1)
					continue;

				if (Index > MaxIndex)
					MaxIndex = Index;
			}

			sprintf(Options.Properties.mName.GetBuffer(PrefixLength + 10), "%s%d", Prefix, MaxIndex + 1);

			Options.Properties.LoadDefaults();
			Options.SetDefault = false;

			if (!gMainWindow->DoDialog(LC_DIALOG_PROPERTIES, &Options))
				break;

			// todo: validate name

			if (Options.SetDefault)
				Options.Properties.SaveDefaults();

			lcModel* Model = new lcModel();
			Model->mProperties = Options.Properties;
			mModels.Add(Model);

			// todo: set active model function
			// todo: fix cameras

			mActiveModel = Model;
			UpdateInterface();
		}
		break;

	case LC_MODEL_MODEL1:
	case LC_MODEL_MODEL2:
	case LC_MODEL_MODEL3:
	case LC_MODEL_MODEL4:
	case LC_MODEL_MODEL5:
	case LC_MODEL_MODEL6:
	case LC_MODEL_MODEL7:
	case LC_MODEL_MODEL8:
	case LC_MODEL_MODEL9:
	case LC_MODEL_MODEL10:
	case LC_MODEL_MODEL11:
	case LC_MODEL_MODEL12:
	case LC_MODEL_MODEL13:
	case LC_MODEL_MODEL14:
	case LC_MODEL_MODEL15:
	case LC_MODEL_MODEL16:
		{
			lcModel* Model = mModels[id - LC_MODEL_FIRST];

			// todo: set active model function
			// todo: fix cameras

			mActiveModel = Model;
			UpdateInterface();
		}
		break;

	case LC_HELP_HOMEPAGE:
		g_App->OpenURL("http://www.leocad.org/");
		break;

	case LC_HELP_EMAIL:
		g_App->OpenURL("mailto:leozide@gmail.com?subject=LeoCAD");
		break;

	case LC_HELP_UPDATES:
		gMainWindow->DoDialog(LC_DIALOG_CHECK_UPDATES, NULL);
		break;

	case LC_HELP_ABOUT:
		{
			String Info;
			char Text[256];

			gMainWindow->mActiveView->MakeCurrent();

			GLint Red, Green, Blue, Alpha, Depth, Stencil;
			GLboolean DoubleBuffer, RGBA;

			glGetIntegerv(GL_RED_BITS, &Red);
			glGetIntegerv(GL_GREEN_BITS, &Green);
			glGetIntegerv(GL_BLUE_BITS, &Blue);
			glGetIntegerv(GL_ALPHA_BITS, &Alpha);
			glGetIntegerv(GL_DEPTH_BITS, &Depth);
			glGetIntegerv(GL_STENCIL_BITS, &Stencil);
			glGetBooleanv(GL_DOUBLEBUFFER, &DoubleBuffer);
			glGetBooleanv(GL_RGBA_MODE, &RGBA);

			Info = "OpenGL Version ";
			Info += (const char*)glGetString(GL_VERSION);
			Info += "\n";
			Info += (const char*)glGetString(GL_RENDERER);
			Info += " - ";
			Info += (const char*)glGetString(GL_VENDOR);
			sprintf(Text, "\n\nColor Buffer: %d bits %s %s", Red + Green + Blue + Alpha, RGBA ? "RGBA" : "indexed", DoubleBuffer ? "double buffered" : "");
			Info += Text;
			sprintf(Text, "\nDepth Buffer: %d bits", Depth);
			Info += Text;
			sprintf(Text, "\nStencil Buffer: %d bits", Stencil);
			Info += Text;
			Info += "\nGL_ARB_vertex_buffer_object extension: ";
			Info += GL_HasVertexBufferObject() ? "supported" : "not supported";
			Info += "\nGL_ARB_framebuffer_object extension: ";
			Info += GL_HasFramebufferObject() ? "supported" : "not supported";
			Info += "\nGL_EXT_texture_filter_anisotropic extension: ";
			if (GL_SupportsAnisotropic)
			{
				sprintf(Text, "supported (max %d)", (int)GL_MaxAnisotropy);
				Info += Text;
			}
			else
				Info += "not supported";

			gMainWindow->DoDialog(LC_DIALOG_ABOUT, (char*)Info);
		}
		break;

	case LC_VIEW_TIME_ADD_KEYS:
		gMainWindow->SetAddKeys(!gMainWindow->mAddKeys);
		break;

		case LC_EDIT_SNAP_X:
				if (m_nSnap & LC_DRAW_SNAP_X)
					m_nSnap &= ~LC_DRAW_SNAP_X;
				else
					m_nSnap |= LC_DRAW_SNAP_X;
			gMainWindow->UpdateLockSnap(m_nSnap);
				break;

		case LC_EDIT_SNAP_Y:
				if (m_nSnap & LC_DRAW_SNAP_Y)
					m_nSnap &= ~LC_DRAW_SNAP_Y;
				else
					m_nSnap |= LC_DRAW_SNAP_Y;
			gMainWindow->UpdateLockSnap(m_nSnap);
				break;

		case LC_EDIT_SNAP_Z:
				if (m_nSnap & LC_DRAW_SNAP_Z)
					m_nSnap &= ~LC_DRAW_SNAP_Z;
				else
					m_nSnap |= LC_DRAW_SNAP_Z;
			gMainWindow->UpdateLockSnap(m_nSnap);
				break;

		case LC_EDIT_SNAP_ALL:
				m_nSnap |= LC_DRAW_SNAP_XYZ;
			gMainWindow->UpdateLockSnap(m_nSnap);
				break;

		case LC_EDIT_SNAP_NONE:
				m_nSnap &= ~LC_DRAW_SNAP_XYZ;
			gMainWindow->UpdateLockSnap(m_nSnap);
				break;

		case LC_EDIT_SNAP_TOGGLE:
				if ((m_nSnap & LC_DRAW_SNAP_XYZ) == LC_DRAW_SNAP_XYZ)
					m_nSnap &= ~LC_DRAW_SNAP_XYZ;
				else
					m_nSnap |= LC_DRAW_SNAP_XYZ;
			gMainWindow->UpdateLockSnap(m_nSnap);
				break;

		case LC_EDIT_LOCK_X:
				if (m_nSnap & LC_DRAW_LOCK_X)
					m_nSnap &= ~LC_DRAW_LOCK_X;
				else
					m_nSnap |= LC_DRAW_LOCK_X;
			gMainWindow->UpdateLockSnap(m_nSnap);
				break;

		case LC_EDIT_LOCK_Y:
				if (m_nSnap & LC_DRAW_LOCK_Y)
					m_nSnap &= ~LC_DRAW_LOCK_Y;
				else
					m_nSnap |= LC_DRAW_LOCK_Y;
			gMainWindow->UpdateLockSnap(m_nSnap);
				break;

		case LC_EDIT_LOCK_Z:
				if (m_nSnap & LC_DRAW_LOCK_Z)
					m_nSnap &= ~LC_DRAW_LOCK_Z;
				else
					m_nSnap |= LC_DRAW_LOCK_Z;
			gMainWindow->UpdateLockSnap(m_nSnap);
				break;

		case LC_EDIT_LOCK_NONE:
				m_nSnap &= ~LC_DRAW_LOCK_XYZ;
			gMainWindow->UpdateLockSnap(m_nSnap);
				break;

		case LC_EDIT_LOCK_TOGGLE:
				if ((m_nSnap & LC_DRAW_LOCK_XYZ) == LC_DRAW_LOCK_XYZ)
					m_nSnap &= ~LC_DRAW_LOCK_XYZ;
				else
					m_nSnap |= LC_DRAW_LOCK_XYZ;
			gMainWindow->UpdateLockSnap(m_nSnap);
				break;

		case LC_EDIT_SNAP_MOVE_XY0:
		case LC_EDIT_SNAP_MOVE_XY1:
		case LC_EDIT_SNAP_MOVE_XY2:
		case LC_EDIT_SNAP_MOVE_XY3:
		case LC_EDIT_SNAP_MOVE_XY4:
		case LC_EDIT_SNAP_MOVE_XY5:
		case LC_EDIT_SNAP_MOVE_XY6:
		case LC_EDIT_SNAP_MOVE_XY7:
		case LC_EDIT_SNAP_MOVE_XY8:
		case LC_EDIT_SNAP_MOVE_XY9:
		{
			m_nMoveSnap = (id - LC_EDIT_SNAP_MOVE_XY0) | (m_nMoveSnap & ~0xff);
			if (id != LC_EDIT_SNAP_MOVE_XY0)
				m_nSnap |= LC_DRAW_SNAP_X | LC_DRAW_SNAP_Y;
			else
				m_nSnap &= ~(LC_DRAW_SNAP_X | LC_DRAW_SNAP_Y);
			gMainWindow->UpdateSnap();
		} break;

		case LC_EDIT_SNAP_MOVE_Z0:
		case LC_EDIT_SNAP_MOVE_Z1:
		case LC_EDIT_SNAP_MOVE_Z2:
		case LC_EDIT_SNAP_MOVE_Z3:
		case LC_EDIT_SNAP_MOVE_Z4:
		case LC_EDIT_SNAP_MOVE_Z5:
		case LC_EDIT_SNAP_MOVE_Z6:
		case LC_EDIT_SNAP_MOVE_Z7:
		case LC_EDIT_SNAP_MOVE_Z8:
		case LC_EDIT_SNAP_MOVE_Z9:
		{
			m_nMoveSnap = (((id - LC_EDIT_SNAP_MOVE_Z0) << 8) | (m_nMoveSnap & ~0xff00));
			if (id != LC_EDIT_SNAP_MOVE_Z0)
				m_nSnap |= LC_DRAW_SNAP_Z;
			else
				m_nSnap &= ~LC_DRAW_SNAP_Z;
			gMainWindow->UpdateSnap();
		} break;

		case LC_EDIT_SNAP_ANGLE:
			if (m_nSnap & LC_DRAW_SNAP_A)
				m_nSnap &= ~LC_DRAW_SNAP_A;
			else
		{
				m_nSnap |= LC_DRAW_SNAP_A;
				m_nAngleSnap = lcMax(1, m_nAngleSnap);
			}
			gMainWindow->UpdateSnap();
			break;

		case LC_EDIT_SNAP_ANGLE0:
		case LC_EDIT_SNAP_ANGLE1:
		case LC_EDIT_SNAP_ANGLE2:
		case LC_EDIT_SNAP_ANGLE3:
		case LC_EDIT_SNAP_ANGLE4:
		case LC_EDIT_SNAP_ANGLE5:
		case LC_EDIT_SNAP_ANGLE6:
		case LC_EDIT_SNAP_ANGLE7:
		case LC_EDIT_SNAP_ANGLE8:
		case LC_EDIT_SNAP_ANGLE9:
		{
			const int Angles[] = { 0, 1, 5, 10, 15, 30, 45, 60, 90, 180 };

			if (id == LC_EDIT_SNAP_ANGLE0)
				m_nSnap &= ~LC_DRAW_SNAP_A;
			else
			{
				m_nSnap |= LC_DRAW_SNAP_A;
				m_nAngleSnap = Angles[id - LC_EDIT_SNAP_ANGLE0];
			}
			gMainWindow->UpdateSnap();
		} break;

	case LC_EDIT_TRANSFORM:
//		TransformSelectedObjects((LC_TRANSFORM_TYPE)mTransformType, gMainWindow->GetTransformAmount());
		break;

	case LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION:
	case LC_EDIT_TRANSFORM_RELATIVE_TRANSLATION:
	case LC_EDIT_TRANSFORM_ABSOLUTE_ROTATION:
	case LC_EDIT_TRANSFORM_RELATIVE_ROTATION:
		gMainWindow->SetTransformMode((lcTransformMode)(id - LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION));
		break;

	case LC_EDIT_ACTION_SELECT:
	case LC_EDIT_ACTION_INSERT:
	case LC_EDIT_ACTION_LIGHT:
	case LC_EDIT_ACTION_SPOTLIGHT:
	case LC_EDIT_ACTION_CAMERA:
	case LC_EDIT_ACTION_MOVE:
	case LC_EDIT_ACTION_ROTATE:
	case LC_EDIT_ACTION_DELETE:
	case LC_EDIT_ACTION_PAINT:
	case LC_EDIT_ACTION_ZOOM:
	case LC_EDIT_ACTION_ZOOM_REGION:
	case LC_EDIT_ACTION_PAN:
	case LC_EDIT_ACTION_ROTATE_VIEW:
	case LC_EDIT_ACTION_ROLL:
		gMainWindow->SetCurrentTool((lcTool)(id - LC_EDIT_ACTION_FIRST));
		break;

	case LC_EDIT_CANCEL:
		if (gMainWindow->mActiveView->IsTracking())
			gMainWindow->mActiveView->StopTracking(false);
		else
			mActiveModel->ClearSelection();
		break;

	case LC_NUM_COMMANDS:
		break;
	}
}

// Remove unused groups
void Project::RemoveEmptyGroups()
{
	bool recurse = false;
	Group *g1, *g2;
	Piece* pPiece;
	int ref;

	for (g1 = m_pGroups; g1;)
	{
		ref = 0;

		for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			if (pPiece->GetGroup() == g1)
				ref++;

		for (g2 = m_pGroups; g2; g2 = g2->m_pNext)
			if (g2->m_pGroup == g1)
				ref++;

		if (ref < 2)
		{
			if (ref != 0)
			{
				for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
					if (pPiece->GetGroup() == g1)
						pPiece->SetGroup(g1->m_pGroup);

				for (g2 = m_pGroups; g2; g2 = g2->m_pNext)
					if (g2->m_pGroup == g1)
						g2->m_pGroup = g1->m_pGroup;
			}

			if (g1 == m_pGroups)
			{
				m_pGroups = g1->m_pNext;
				delete g1;
				g1 = m_pGroups;
			}
			else
			{
				for (g2 = m_pGroups; g2; g2 = g2->m_pNext)
					if (g2->m_pNext == g1)
					{
						g2->m_pNext = g1->m_pNext;
						break;
					}

				delete g1;
				g1 = g2->m_pNext;
			}

			recurse = true;
		}
		else
			g1 = g1->m_pNext;
	}

	if (recurse)
		RemoveEmptyGroups();
}

Group* Project::AddGroup (const char* name, Group* pParent, float x, float y, float z)
{
  Group *pNewGroup = new Group();

  if (name == NULL)
  {
    int i, max = 0;
    char str[65];

    for (Group *pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
      if (strncmp (pGroup->m_strName, "Group #", 7) == 0)
        if (sscanf(pGroup->m_strName, "Group #%d", &i) == 1)
          if (i > max)
            max = i;
    sprintf (str, "Group #%.2d", max+1);

    strcpy (pNewGroup->m_strName, str);
  }
  else
    strcpy (pNewGroup->m_strName, name);

  pNewGroup->m_pNext = m_pGroups;
  m_pGroups = pNewGroup;

  pNewGroup->m_fCenter[0] = x;
  pNewGroup->m_fCenter[1] = y;
  pNewGroup->m_fCenter[2] = z;
  pNewGroup->m_pGroup = pParent;

  return pNewGroup;
}

void Project::SelectAndFocusNone(bool bFocusOnly)
{
	Piece* pPiece;
	Light* pLight;

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		pPiece->Select(false, bFocusOnly, false);

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		Camera* pCamera = mCameras[CameraIdx];

		pCamera->Select (false, bFocusOnly, false);
		pCamera->GetTarget()->Select (false, bFocusOnly, false);
	}

	for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
	{
		pLight->Select(false, bFocusOnly, false);
		if (pLight->GetTarget())
			pLight->GetTarget()->Select (false, bFocusOnly, false);
	}
//	AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, NULL, OT_PIECE);
}

bool Project::GetSelectionCenter(lcVector3& Center) const
{
	float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
	bool Selected = false;

	for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		if (pPiece->IsSelected())
		{
			pPiece->CompareBoundingBox(bs);
			Selected = true;
		}
	}

	Center = lcVector3((bs[0] + bs[3]) * 0.5f, (bs[1] + bs[4]) * 0.5f, (bs[2] + bs[5]) * 0.5f);

	return Selected;
}

void Project::ConvertToUserUnits(lcVector3& Value) const
{
	if ((m_nSnap & LC_DRAW_CM_UNITS) == 0)
		Value /= 0.04f;
}

void Project::ConvertFromUserUnits(lcVector3& Value) const
{
	if ((m_nSnap & LC_DRAW_CM_UNITS) == 0)
		Value *= 0.04f;
}

// Returns the object that currently has focus.
Object* Project::GetFocusObject() const
{
	for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		if (pPiece->IsFocused())
			return pPiece;
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		Camera* pCamera = mCameras[CameraIdx];

		if (pCamera->IsEyeFocused() || pCamera->IsTargetFocused())
			return pCamera;
	}

	// TODO: light

	return NULL;
}

void Project::GetSnapIndex(int* SnapXY, int* SnapZ, int* SnapAngle) const
{
	if (SnapXY)
		*SnapXY = (m_nMoveSnap & 0xff);

	if (SnapZ)
		*SnapZ = ((m_nMoveSnap >> 8) & 0xff);

	if (SnapAngle)
	{
		if (m_nSnap & LC_DRAW_SNAP_A)
		{
			int Angles[] = { 0, 1, 5, 10, 15, 30, 45, 60, 90, 180 };
			*SnapAngle = -1;

			for (unsigned int i = 0; i < sizeof(Angles)/sizeof(Angles[0]); i++)
			{
				if (m_nAngleSnap == Angles[i])
				{
					*SnapAngle = i;
					break;
				}
			}
		}
		else
			*SnapAngle = 0;
	}
}

void Project::GetSnapDistance(float* SnapXY, float* SnapZ) const
{
	const float SnapXYTable[] = { 0.01f, 0.04f, 0.2f, 0.32f, 0.4f, 0.8f, 1.6f, 2.4f, 3.2f, 6.4f };
	const float SnapZTable[] = { 0.01f, 0.04f, 0.2f, 0.32f, 0.4f, 0.8f, 0.96f, 1.92f, 3.84f, 7.68f };

	int SXY, SZ;
	GetSnapIndex(&SXY, &SZ, NULL);

	SXY = lcMin(SXY, 9);
	SZ = lcMin(SZ, 9);

	*SnapXY = SnapXYTable[SXY];
	*SnapZ = SnapZTable[SZ];
}

void Project::GetSnapText(char* SnapXY, char* SnapZ, char* SnapAngle) const
{
	if (m_nSnap & LC_DRAW_CM_UNITS)
	{
		float xy, z;

		GetSnapDistance(&xy, &z);

		sprintf(SnapXY, "%.2f", xy);
		sprintf(SnapZ, "%.2f", z);
	}
	else
	{
		const char* SnapXYText[] = { "0", "1/20S", "1/4S", "1F", "1/2S", "1S", "2S", "3S", "4S", "8S" };
		const char* SnapZText[] = { "0", "1/20S", "1/4S", "1F", "1/2S", "1S", "1B", "2B", "4B", "8B" };
		const char* SnapAngleText[] = { "0", "1", "5", "10", "15", "30", "45", "60", "90", "180" };

		int SXY, SZ, SA;
		GetSnapIndex(&SXY, &SZ, &SA);

		SXY = lcMin(SXY, 9);
		SZ = lcMin(SZ, 9);

		strcpy(SnapXY, SnapXYText[SXY]);
		strcpy(SnapZ, SnapZText[SZ]);
		strcpy(SnapAngle, SnapAngleText[SA]);
	}
}

lcVector3 Project::SnapVector(const lcVector3& Distance) const
{
	float SnapXY, SnapZ;
	GetSnapDistance(&SnapXY, &SnapZ);

	lcVector3 NewDistance(Distance);

	if (m_nSnap & LC_DRAW_SNAP_X)
	{
		int i = (int)(Distance[0] / SnapXY);
		float Leftover = Distance[0] - (SnapXY * i);

		if (Leftover > SnapXY / 2)
			i++;
		else if (Leftover < -SnapXY / 2)
			i--;

		NewDistance[0] = SnapXY * i;
	}

	if (m_nSnap & LC_DRAW_SNAP_Y)
	{
		int i = (int)(Distance[1] / SnapXY);
		float Leftover = Distance[1] - (SnapXY * i);

		if (Leftover > SnapXY / 2)
			i++;
		else if (Leftover < -SnapXY / 2)
			i--;

		NewDistance[1] = SnapXY * i;
	}

	if (m_nSnap & LC_DRAW_SNAP_Z)
	{
		int i = (int)(Distance[2] / SnapZ);
		float Leftover = Distance[2] - (SnapZ * i);

		if (Leftover > SnapZ / 2)
			i++;
		else if (Leftover < -SnapZ / 2)
			i--;

		NewDistance[2] = SnapZ * i;
	}

	return NewDistance;
}

void Project::SnapRotationVector(lcVector3& Delta, lcVector3& Leftover) const
{
	if (m_nSnap & LC_DRAW_SNAP_A)
	{
		int Snap[3];

		for (int i = 0; i < 3; i++)
		{
			Snap[i] = (int)(Delta[i] / (float)m_nAngleSnap);
		}

		lcVector3 NewDelta((float)(m_nAngleSnap * Snap[0]), (float)(m_nAngleSnap * Snap[1]), (float)(m_nAngleSnap * Snap[2]));
		Leftover = Delta - NewDelta;
		Delta = NewDelta;
	}
}

lcVector3 Project::GetMoveDistance(const lcVector3& Distance, bool Snap, bool Lock)
{
	lcVector3 NewDistance(Distance);

	if (Lock)
	{
		if (m_nSnap & LC_DRAW_LOCK_X)
			NewDistance[0] = 0;

		if (m_nSnap & LC_DRAW_LOCK_Y)
			NewDistance[1] = 0;

		if (m_nSnap & LC_DRAW_LOCK_Z)
			NewDistance[2] = 0;
	}

	if (Snap)
		NewDistance = SnapVector(NewDistance);

	if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
	{
		Object* Focus = GetFocusObject();

		if ((Focus != NULL) && Focus->IsPiece())
			NewDistance = lcMul30(NewDistance, ((Piece*)Focus)->mModelWorld);
	}

	return NewDistance;
}

bool Project::RotateSelectedObjects(lcVector3& Delta, lcVector3& Remainder, bool Snap, bool Lock)
{
	// Don't move along locked directions.
	if (Lock)
	{
		if (m_nSnap & LC_DRAW_LOCK_X)
			Delta[0] = 0;

		if (m_nSnap & LC_DRAW_LOCK_Y)
			Delta[1] = 0;

		if (m_nSnap & LC_DRAW_LOCK_Z)
			Delta[2] = 0;
	}

	// Snap.
	if (Snap)
		SnapRotationVector(Delta, Remainder);

	if (Delta.LengthSquared() < 0.001f)
		return false;

	float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
	lcVector3 pos;
	lcVector4 rot;
	int nSel = 0;
	Piece *pPiece, *pFocus = NULL;

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		if (pPiece->IsSelected())
		{
			if (pPiece->IsFocused())
				pFocus = pPiece;

			pPiece->CompareBoundingBox(bs);
			nSel++;
		}
	}

	if (pFocus != NULL)
	{
		pos = pFocus->mPosition;
		bs[0] = bs[3] = pos[0];
		bs[1] = bs[4] = pos[1];
		bs[2] = bs[5] = pos[2];
	}

	lcVector3 Center((bs[0]+bs[3])/2, (bs[1]+bs[4])/2, (bs[2]+bs[5])/2);

	// Create the rotation matrix.
	lcVector4 RotationQuaternion(0, 0, 0, 1);
	lcVector4 WorldToFocusQuaternion, FocusToWorldQuaternion;

	if (!(m_nSnap & LC_DRAW_LOCK_X) && (Delta[0] != 0.0f))
	{
		lcVector4 q = lcQuaternionRotationX(Delta[0] * LC_DTOR);
		RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
	}

	if (!(m_nSnap & LC_DRAW_LOCK_Y) && (Delta[1] != 0.0f))
	{
		lcVector4 q = lcQuaternionRotationY(Delta[1] * LC_DTOR);
		RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
	}

	if (!(m_nSnap & LC_DRAW_LOCK_Z) && (Delta[2] != 0.0f))
	{
		lcVector4 q = lcQuaternionRotationZ(Delta[2] * LC_DTOR);
		RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
	}

	// Transform the rotation relative to the focused piece.
	if (m_nSnap & LC_DRAW_GLOBAL_SNAP)
		pFocus = NULL;

	if (pFocus != NULL)
	{
		lcVector4 Rot;
		Rot = ((Piece*)pFocus)->mRotation;

		WorldToFocusQuaternion = lcQuaternionFromAxisAngle(lcVector4(Rot[0], Rot[1], Rot[2], -Rot[3] * LC_DTOR));
		FocusToWorldQuaternion = lcQuaternionFromAxisAngle(lcVector4(Rot[0], Rot[1], Rot[2], Rot[3] * LC_DTOR));

		RotationQuaternion = lcQuaternionMultiply(FocusToWorldQuaternion, RotationQuaternion);
	}

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		if (!pPiece->IsSelected())
			continue;

		pos = pPiece->mPosition;
		rot = pPiece->mRotation;

		lcVector4 NewRotation;

		if ((nSel == 1) && (pFocus == pPiece))
		{
			lcVector4 LocalToWorldQuaternion;
			LocalToWorldQuaternion = lcQuaternionFromAxisAngle(lcVector4(rot[0], rot[1], rot[2], rot[3] * LC_DTOR));

			lcVector4 NewLocalToWorldQuaternion;

			if (pFocus != NULL)
			{
				lcVector4 LocalToFocusQuaternion = lcQuaternionMultiply(WorldToFocusQuaternion, LocalToWorldQuaternion);
				NewLocalToWorldQuaternion = lcQuaternionMultiply(LocalToFocusQuaternion, RotationQuaternion);
			}
			else
			{
				NewLocalToWorldQuaternion = lcQuaternionMultiply(RotationQuaternion, LocalToWorldQuaternion);
			}

			NewRotation = lcQuaternionToAxisAngle(NewLocalToWorldQuaternion);
		}
		else
		{
			lcVector3 Distance = lcVector3(pos[0], pos[1], pos[2]) - Center;

			lcVector4 LocalToWorldQuaternion = lcQuaternionFromAxisAngle(lcVector4(rot[0], rot[1], rot[2], rot[3] * LC_DTOR));

			lcVector4 NewLocalToWorldQuaternion;

			if (pFocus != NULL)
			{
				lcVector4 LocalToFocusQuaternion = lcQuaternionMultiply(WorldToFocusQuaternion, LocalToWorldQuaternion);
				NewLocalToWorldQuaternion = lcQuaternionMultiply(RotationQuaternion, LocalToFocusQuaternion);

				lcVector4 WorldToLocalQuaternion = lcQuaternionFromAxisAngle(lcVector4(rot[0], rot[1], rot[2], -rot[3] * LC_DTOR));

				Distance = lcQuaternionMul(Distance, WorldToLocalQuaternion);
				Distance = lcQuaternionMul(Distance, NewLocalToWorldQuaternion);
			}
			else
			{
				NewLocalToWorldQuaternion = lcQuaternionMultiply(RotationQuaternion, LocalToWorldQuaternion);

				Distance = lcQuaternionMul(Distance, RotationQuaternion);
			}

			NewRotation = lcQuaternionToAxisAngle(NewLocalToWorldQuaternion);

			pos[0] = Center[0] + Distance[0];
			pos[1] = Center[1] + Distance[1];
			pos[2] = Center[2] + Distance[2];

			pPiece->ChangeKey(m_nCurStep, m_bAnimation, gMainWindow->mAddKeys, pos, LC_PK_POSITION);
		}

		rot[0] = NewRotation[0];
		rot[1] = NewRotation[1];
		rot[2] = NewRotation[2];
		rot[3] = NewRotation[3] * LC_RTOD;

		pPiece->ChangeKey(m_nCurStep, m_bAnimation, gMainWindow->mAddKeys, rot, LC_PK_ROTATION);
		pPiece->UpdatePosition(m_nCurStep, m_bAnimation);
	}

	return true;
}
/*
void Project::TransformSelectedObjects(LC_TRANSFORM_TYPE Type, const lcVector3& Transform)
{
	if (!mActiveModel->GetSelectedObjects().GetSize())
		return;

	switch (Type)
	{
	case LC_TRANSFORM_ABSOLUTE_TRANSLATION:
		{
			float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
			lcVector3 Center;
			int nSel = 0;
			Piece *pPiece, *pFocus = NULL;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if (pPiece->IsSelected())
				{
					if (pPiece->IsFocused())
						pFocus = pPiece;

					pPiece->CompareBoundingBox(bs);
					nSel++;
				}
			}

			if (pFocus != NULL)
				Center = pFocus->mPosition;
			else
				Center = lcVector3((bs[0]+bs[3])/2, (bs[1]+bs[4])/2, (bs[2]+bs[5])/2);

			lcVector3 Offset = Transform - Center;

			Light* pLight;

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
			{
				Camera* pCamera = mCameras[CameraIdx];

				if (pCamera->IsSelected())
				{
					pCamera->Move(m_nCurStep, m_bAnimation, m_bAddKeys, Offset.x, Offset.y, Offset.z);
					pCamera->UpdatePosition(m_nCurStep, m_bAnimation);
				}
			}

			for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
			{
				if (pLight->IsSelected())
				{
					pLight->Move(m_nCurStep, m_bAnimation, m_bAddKeys, Offset.x, Offset.y, Offset.z);
					pLight->UpdatePosition (m_nCurStep, m_bAnimation);
				}
			}

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if (pPiece->IsSelected())
				{
					pPiece->Move(m_nCurStep, m_bAnimation, m_bAddKeys, Offset.x, Offset.y, Offset.z);
					pPiece->UpdatePosition(m_nCurStep, m_bAnimation);
				}
			}

			if (nSel)
			{
				gMainWindow->UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Moving");
				gMainWindow->UpdateFocusObject();
			}
		} break;

	case LC_TRANSFORM_RELATIVE_TRANSLATION:
		{
			mActiveModel->BeginMoveTool();
			mActiveModel->UpdateMoveTool(GetMoveDistance(Transform, false, false), m_bAddKeys);
			mActiveModel->EndMoveTool(true);
		} break;

	case LC_TRANSFORM_ABSOLUTE_ROTATION:
		{
			// Create the rotation matrix.
			lcVector4 RotationQuaternion(0, 0, 0, 1);

			if (Transform[0] != 0.0f)
			{
				lcVector4 q = lcQuaternionRotationX(Transform[0] * LC_DTOR);
				RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
			}

			if (Transform[1] != 0.0f)
			{
				lcVector4 q = lcQuaternionRotationY(Transform[1] * LC_DTOR);
				RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
			}

			if (Transform[2] != 0.0f)
			{
				lcVector4 q = lcQuaternionRotationZ(Transform[2] * LC_DTOR);
				RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
			}

			lcVector4 NewRotation = lcQuaternionToAxisAngle(RotationQuaternion);
			NewRotation[3] *= LC_RTOD;

			Piece *pPiece;
			int nSel = 0;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if (pPiece->IsSelected())
				{
					pPiece->ChangeKey(m_nCurStep, m_bAnimation, m_bAddKeys, NewRotation, LC_PK_ROTATION);
					pPiece->UpdatePosition(m_nCurStep, m_bAnimation);
					nSel++;
				}
			}

			if (nSel)
			{
				gMainWindow->UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Rotating");
				gMainWindow->UpdateFocusObject();
			}
		} break;

	case LC_TRANSFORM_RELATIVE_ROTATION:
		{
			lcVector3 Rotate(Transform), Remainder;

			if (RotateSelectedObjects(Rotate, Remainder, false, false))
			{
				gMainWindow->UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Rotating");
				gMainWindow->UpdateFocusObject();
			}
		} break;
	}
}
*/
void Project::ModifyObject(Object* Object, lcObjectProperty Property, void* Value)
{
	const char* CheckPointString = NULL;

	switch (Property)
	{
	case LC_PROPERTY_PART_POSITION:
		{
			const lcVector3& Position = *(lcVector3*)Value;
			Piece* Part = (Piece*)Object;

			if (Part->mPosition != Position)
		{
				Part->ChangeKey(m_nCurStep, m_bAnimation, gMainWindow->mAddKeys, Position, LC_PK_POSITION);
				Part->UpdatePosition(m_nCurStep, m_bAnimation);

				CheckPointString = "Moving";
			}
		} break;

	case LC_PROPERTY_PART_ROTATION:
		{
			const lcVector4& Rotation = *(lcVector4*)Value;
			Piece* Part = (Piece*)Object;

			if (Rotation != Part->mRotation)
		{
				Part->ChangeKey(m_nCurStep, m_bAnimation, gMainWindow->mAddKeys, Rotation, LC_PK_ROTATION);
				Part->UpdatePosition(m_nCurStep, m_bAnimation);

				CheckPointString = "Rotating";
			}
		} break;

	case LC_PROPERTY_PART_SHOW:
		{
			lcuint32 Show = *(lcuint32*)Value;
			Piece* Part = (Piece*)Object;

			if (m_bAnimation)
			{
				if (Show != Part->GetFrameShow())
				{
					Part->SetFrameShow(Show);

					CheckPointString = "Show";
				}
				}
				else
				{
				if (Show != Part->GetStepShow())
						{
					Part->SetStepShow(Show);

					CheckPointString = "Show";
						}
				}
		} break;

	case LC_PROPERTY_PART_HIDE:
			{
			lcuint32 Hide = *(lcuint32*)Value;
			Piece* Part = (Piece*)Object;

			if (m_bAnimation)
				{
				if (Hide != Part->GetFrameHide())
					{
					Part->SetFrameHide(Hide);

					CheckPointString = "Hide";
						}
					}
				else
				{
				if (Hide != Part->GetStepHide())
							{
					Part->SetStepHide(Hide);

					CheckPointString = "Hide";
				}
			}
		} break;

	case LC_PROPERTY_PART_COLOR:
			{
			int ColorIndex = *(int*)Value;
			Piece* Part = (Piece*)Object;

			if (ColorIndex != Part->mColorIndex)
        {
				Part->SetColorIndex(ColorIndex);

				CheckPointString = "Color";
			}
		} break;

	case LC_PROPERTY_PART_ID:
		{
			Piece* Part = (Piece*)Object;
			PieceInfo* Info = (PieceInfo*)Value;

			if (Info != Part->mPieceInfo)
			{
				Part->mPieceInfo->Release();
				Part->mPieceInfo = Info;
				Part->mPieceInfo->AddRef();

				CheckPointString = "Part";
			}
		} break;

	case LC_PROPERTY_CAMERA_POSITION:
			{
			const lcVector3& Position = *(lcVector3*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->mPosition != Position)
{
				camera->ChangeKey(m_nCurStep, m_bAnimation, gMainWindow->mAddKeys, Position, LC_CK_EYE);
				camera->UpdatePosition(m_nCurStep, m_bAnimation);

				CheckPointString = "Camera";
			}
		} break;

	case LC_PROPERTY_CAMERA_TARGET:
			{
			const lcVector3& TargetPosition = *(lcVector3*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->mTargetPosition != TargetPosition)
				{
				camera->ChangeKey(m_nCurStep, m_bAnimation, gMainWindow->mAddKeys, TargetPosition, LC_CK_TARGET);
				camera->UpdatePosition(m_nCurStep, m_bAnimation);

				CheckPointString = "Camera";
				}
		} break;

	case LC_PROPERTY_CAMERA_UP:
				{
			const lcVector3& Up = *(lcVector3*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->mUpVector != Up)
					{
				camera->ChangeKey(m_nCurStep, m_bAnimation, gMainWindow->mAddKeys, Up, LC_CK_UP);
				camera->UpdatePosition(m_nCurStep, m_bAnimation);

				CheckPointString = "Camera";
			}
		} break;

	case LC_PROPERTY_CAMERA_FOV:
		{
			float FOV = *(float*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->m_fovy != FOV)
		{
				camera->m_fovy = FOV;
				camera->UpdatePosition(m_nCurStep, m_bAnimation);

				CheckPointString = "Camera";
					}
		} break;

	case LC_PROPERTY_CAMERA_NEAR:
				{
			float Near = *(float*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->m_zNear != Near)
			{
				camera->m_zNear= Near;
				camera->UpdatePosition(m_nCurStep, m_bAnimation);

				CheckPointString = "Camera";
					}
		} break;

	case LC_PROPERTY_CAMERA_FAR:
		{
			float Far = *(float*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->m_zFar != Far)
		{
				camera->m_zFar = Far;
				camera->UpdatePosition(m_nCurStep, m_bAnimation);

				CheckPointString = "Camera";
			}
		} break;

	case LC_PROPERTY_CAMERA_NAME:
		{
			const char* Name = (const char*)Value;
			Camera* camera = (Camera*)Object;

			if (strcmp(camera->m_strName, Name))
			{
				strncpy(camera->m_strName, Name, sizeof(camera->m_strName));
				camera->m_strName[sizeof(camera->m_strName) - 1] = 0;

				gMainWindow->UpdateCameraMenu();

				CheckPointString = "Camera";
			}
		}
	}

	if (CheckPointString)
	{
		SetModifiedFlag(true);
		CheckPoint(CheckPointString);
		gMainWindow->UpdateFocusObject();
		gMainWindow->UpdateAllViews();
	}
}

void Project::ZoomActiveView(int Amount)
{
	const float MouseSensitivity = 1.0f / (21.0f - g_App->mPreferences->mMouseSensitivity);

	mActiveModel->BeginEditCameraTool(LC_ACTION_ZOOM_CAMERA, lcVector3(0.0f, 0.0f, 0.0f));
	mActiveModel->UpdateEditCameraTool(LC_ACTION_ZOOM_CAMERA, 2.0f * Amount * MouseSensitivity, 0.0f, gMainWindow->mAddKeys);
	mActiveModel->EndEditCameraTool(LC_ACTION_ZOOM_CAMERA, true);
}

void Project::BeginColorDrop()
{
//	StartTracking(LC_TRACK_LEFT);
//	SetAction(LC_TOOL_PAINT);
}