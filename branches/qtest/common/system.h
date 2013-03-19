#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "typedefs.h"
#include "array.h"

// Assert macros.
#ifdef LC_DEBUG

extern bool lcAssert(const char* FileName, int Line, const char* Expression, const char* Description);

#define LC_ASSERT(Expr, Desc) \
do \
{ \
	static bool Ignore = false; \
	if (!Expr && !Ignore) \
		Ignore = lcAssert(__FILE__, __LINE__, #Expr, Desc); \
} while (0)

#define LC_ASSERT_FALSE(Desc) LC_ASSERT(0, Desc)

#else

#define LC_ASSERT(...) do { } while(0)

#define LC_ASSERT_FALSE(Desc) LC_ASSERT(0, Desc)

#endif

#if _MSC_VER >= 1600
#define LC_CASSERT(x) static_assert(x, "Assertion failed: " #x)
#else
#define LC_CASSERT_CONCAT1(e, l) extern int LC_CASSERT_##l[(int)(e) ? 1 : -1]
#define LC_CASSERT_CONCAT2(e, l) LC_CASSERT_CONCAT1(e, l)
#define LC_CASSERT(e) LC_CASSERT_CONCAT2(e, __LINE__)
#endif

// Memory render
void* Sys_StartMemoryRender (int width, int height);
void Sys_FinishMemoryRender (void* param);

// FIXME: moved to basewnd, remove

// Message Box
#define LC_OK           1
#define LC_CANCEL       2
#define LC_ABORT        3
#define LC_RETRY        4
#define LC_IGNORE       5
#define LC_YES          6
#define LC_NO           7
 
#define LC_MB_OK                 0x000
#define LC_MB_OKCANCEL           0x001
#define LC_MB_YESNOCANCEL        0x003
#define LC_MB_YESNO              0x004

#define LC_MB_ICONERROR          0x010
#define LC_MB_ICONQUESTION       0x020
#define LC_MB_ICONWARNING        0x030
#define LC_MB_ICONINFORMATION    0x040
 
#define LC_MB_TYPEMASK           0x00F
#define LC_MB_ICONMASK           0x0F0

int Sys_MessageBox (const char* text, const char* caption="LeoCAD", int type=LC_MB_OK|LC_MB_ICONINFORMATION);

// Misc stuff
bool Sys_KeyDown (int key);
void Sys_GetFileList(const char* Path, ObjArray<String>& FileList);

// User Interface
void SystemUpdateColorList(int nNew);
void SystemUpdateSelected(unsigned long flags, int SelectedCount, class Object* Focus);
void SystemUpdatePlay(bool play, bool stop);
void SystemUpdateCategories(bool SearchOnly);

void SystemInit();
void SystemFinish();
bool SystemDoDialog(int nMode, void* param);
void SystemDoPopupMenu(int nMenu, int x, int y);

void SystemPieceComboAdd(char* name);

void SystemExportClipboard(lcFile* clip);
lcFile* SystemImportClipboard();

void SystemPumpMessages();
long SystemGetTicks();

void SystemStartProgressBar(int nLower, int nUpper, int nStep, const char* Text);
void SytemEndProgressBar();
void SytemStepProgressBar();

#endif // _SYSTEM_H_
