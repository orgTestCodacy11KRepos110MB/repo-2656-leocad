#ifndef _LC_MAINWINDOW_H_
#define _LC_MAINWINDOW_H_

#include "lc_basewindow.h"
#include "lc_array.h"
#include "project.h"

class View;
class PiecePreview;

#define LC_MAX_RECENT_FILES 4

class lcMainWindow : public lcBaseWindow
{
 public:
	lcMainWindow();
	~lcMainWindow();

	lcTool GetTool() const
	{
		return mTool;
	}

	bool GetAddKeys() const
	{
		return mAddKeys;
	}

	View* GetActiveView() const
	{
		return mActiveView;
	}

	const lcArray<View*>& GetViews()
	{
		return mViews;
	}

	void AddView(View* View);
	void RemoveView(View* View);
	void SetActiveView(View* ActiveView);
	void UpdateAllViews();

	void SetTool(lcTool Tool);
	void SetColorIndex(int ColorIndex);
	void Close();

	void AddRecentFile(const char* FileName);
	void RemoveRecentFile(int FileIndex);

	void SplitHorizontal();
	void SplitVertical();
	void RemoveView();
	void ResetViews();

	void TogglePrintPreview();
	void ToggleFullScreen();

	void UpdateFocusObject(Object* Focus);
	void UpdateSelectedObjects(int Flags, int SelectedCount, Object* Focus);
	void UpdateAction(int NewAction);
	void UpdatePaste(bool Enabled);
	void UpdateTime(int CurrentTime, int TotalTime);
	void SetAddKeys(bool AddKeys);
	void UpdateLockSnap(lcuint32 Snap);
	void UpdateSnap();
	void UpdateUndoRedo(const char* UndoText, const char* RedoText);
	void UpdateTransformType(int NewType);
	void UpdateCurrentCamera(int CameraIndex);
	void UpdatePerspective();
	void UpdateCameraMenu();
	void UpdateCategories();
	void UpdateTitle(const char* Title, bool Modified);
	void UpdateModified(bool Modified);
	void UpdateRecentFiles();
	void UpdateShortcuts();

	lcVector3 GetTransformAmount();

	char mRecentFiles[LC_MAX_RECENT_FILES][LC_MAXPATH];
	PiecePreview* mPreviewWidget;
	int mColorIndex;

protected:
	View* mActiveView;
	lcArray<View*> mViews;

	bool mAddKeys;
	lcTool mTool;
};

extern class lcMainWindow* gMainWindow;

#endif // _LC_MAINWND_H_