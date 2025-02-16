/* Copyright 2014 the SumatraPDF project authors (see AUTHORS file).
   License: Simplified BSD (see COPYING.BSD) */

#ifndef WinUtil_h
#define WinUtil_h

#include <WindowsX.h>
#include <CommCtrl.h>
#include <WinSafer.h>

// the following are only defined if _WIN32_WINNT >= 0x0600 and we use 0x0500
#ifndef USER_DEFAULT_SCREEN_DPI
#define USER_DEFAULT_SCREEN_DPI 96
#endif
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

#define WIN_COL_WHITE   RGB(0xff, 0xff, 0xff)
#define WIN_COL_BLACK   RGB(0, 0, 0)

#define DRAGQUERY_NUMFILES 0xFFFFFFFF

#define Edit_SelectAll(hwnd) Edit_SetSel(hwnd, 0, -1)
#define ListBox_AppendString_NoSort(hwnd, txt) ListBox_InsertString(hwnd, -1, txt)

HMODULE SafeLoadLibrary(const WCHAR *dllName);
FARPROC LoadDllFunc(WCHAR *dllName, const char *funcName);
BOOL    SafeCloseHandle(HANDLE *h);
BOOL    SafeDestroyWindow(HWND *hwnd);
void    FillWndClassEx(WNDCLASSEX& wcex, HINSTANCE hInstance, const WCHAR *clsName, WNDPROC wndproc);
inline void MoveWindow(HWND hwnd, RectI rect) { MoveWindow(hwnd, rect.x, rect.y, rect.dx, rect.dy, TRUE); }

bool   IsAppThemed();
bool   IsRunningInWow64();
bool   IsVistaOrGreater();

void   LogLastError(DWORD err=0);
bool   RegKeyExists(HKEY keySub, const WCHAR *keyName);
WCHAR *ReadRegStr(HKEY keySub, const WCHAR *keyName, const WCHAR *valName);
bool   WriteRegStr(HKEY keySub, const WCHAR *keyName, const WCHAR *valName, const WCHAR *value);
bool   WriteRegDWORD(HKEY keySub, const WCHAR *keyName, const WCHAR *valName, DWORD value);
bool   CreateRegKey(HKEY keySub, const WCHAR *keyName);
bool   DeleteRegKey(HKEY keySub, const WCHAR *keyName, bool resetACLFirst=false);
WCHAR *GetSpecialFolder(int csidl, bool createIfMissing=false);

void   DisableDataExecution();
void   RedirectIOToConsole();
WCHAR *GetExePath();
int    FileTimeDiffInSecs(const FILETIME& ft1, const FILETIME& ft2);

WCHAR *ResolveLnk(const WCHAR *path);
bool   CreateShortcut(const WCHAR *shortcutPath, const WCHAR *exePath,
                    const WCHAR *args=NULL, const WCHAR *description=NULL,
                    int iconIndex=0);
IDataObject* GetDataObjectForFile(const WCHAR *filePath, HWND hwnd=NULL);
DWORD GetFileVersion(const WCHAR *path);

inline bool IsKeyPressed(int key)
{
    return GetKeyState(key) & 0x8000 ? true : false;
}
inline bool IsShiftPressed() { return IsKeyPressed(VK_SHIFT); }
inline bool IsAltPressed() { return IsKeyPressed(VK_MENU); }
inline bool IsCtrlPressed() { return IsKeyPressed(VK_CONTROL); }

HFONT   CreateSimpleFont(HDC hdc, const WCHAR *fontName, int fontSize);

RectI   ShiftRectToWorkArea(RectI rect, bool bFully=false);
RectI   GetWorkAreaRect(RectI rect);
RectI   GetFullscreenRect(HWND hwnd);
RectI   GetVirtualScreenRect();

bool    LaunchFile(const WCHAR *path, const WCHAR *params=NULL, const WCHAR *verb=NULL, bool hidden=false);
HANDLE  LaunchProcess(const WCHAR *cmdLine, const WCHAR *currDir=NULL, DWORD flags=0);

void    PaintRect(HDC hdc, const RectI& rect);
void    PaintLine(HDC hdc, const RectI& rect);
void    DrawCenteredText(HDC hdc, const RectI& r, const WCHAR *txt, bool isRTL=false);
SizeI   TextSizeInHwnd(HWND hwnd, const WCHAR *txt);

bool    IsCursorOverWindow(HWND hwnd);
bool    GetCursorPosInHwnd(HWND hwnd, POINT& posOut);
void    CenterDialog(HWND hDlg, HWND hParent=NULL);
WCHAR * GetDefaultPrinterName();
bool    CopyTextToClipboard(const WCHAR *text, bool appendOnly=false);
bool    CopyImageToClipboard(HBITMAP hbmp, bool appendOnly=false);
void    ToggleWindowStyle(HWND hwnd, DWORD flag, bool enable, int type=GWL_STYLE);

IStream*CreateStreamFromData(const void *data, size_t len);
void  * GetDataFromStream(IStream *stream, size_t *len, HRESULT *res_opt=NULL);
bool    ReadDataFromStream(IStream *stream, void *buffer, size_t len, size_t offset=0);
UINT    GuessTextCodepage(const char *data, size_t len, UINT def=CP_ACP);
WCHAR * NormalizeString(const WCHAR *str, int /* NORM_FORM */ form);

void CalcMD5DigestWin(const void *data, size_t byteCount, unsigned char digest[16]);
void CalcSha1DigestWin(const void *data, size_t byteCount, unsigned char digest[32]);
void ResizeHwndToClientArea(HWND hwnd, int dx, int dy, bool hasMenu);

namespace win {

inline size_t GetTextLen(HWND hwnd)
{
    return (size_t)SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
}

/* return a text in edit control represented by hwnd
   return NULL in case of error (couldn't allocate memory)
   caller needs to free() the text */
WCHAR *GetText(HWND hwnd);

inline void SetText(HWND hwnd, const WCHAR *txt)
{
    SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)txt);
}

int GetHwndDpi(HWND hwnd, float *uiDPIFactor=NULL);
int GlobalDpiAdjust(int value);
int GlobalDpiAdjust(float value);

namespace menu {

inline void SetChecked(HMENU m, UINT id, bool isChecked)
{
    CheckMenuItem(m, id, MF_BYCOMMAND | (isChecked ? MF_CHECKED : MF_UNCHECKED));
}

inline bool SetEnabled(HMENU m, UINT id, bool isEnabled)
{
    BOOL ret = EnableMenuItem(m, id, MF_BYCOMMAND | (isEnabled ? MF_ENABLED : MF_GRAYED));
    return ret != -1;
}

inline void Remove(HMENU m, UINT id)
{
    RemoveMenu(m, id, MF_BYCOMMAND);
}

inline void Empty(HMENU m)
{
    while (RemoveMenu(m, 0, MF_BYPOSITION));
}

void SetText(HMENU m, UINT id, WCHAR *s);
WCHAR *ToSafeString(const WCHAR *str);

} // namespace menu

} // namespace win

class DoubleBuffer {
    HWND hTarget;
    HDC hdcCanvas, hdcBuffer;
    HBITMAP doubleBuffer;
    RectI rect;

public:
    DoubleBuffer(HWND hwnd, RectI rect);
    ~DoubleBuffer();

    HDC GetDC() const { return hdcBuffer ? hdcBuffer : hdcCanvas; }
    void Flush(HDC hdc);
};

void    InitAllCommonControls();
SizeI   GetBitmapSize(HBITMAP hbmp);
void    UpdateBitmapColors(HBITMAP hbmp, COLORREF textColor, COLORREF bgColor);
unsigned char *SerializeBitmap(HBITMAP hbmp, size_t *bmpBytesOut);
COLORREF AdjustLightness(COLORREF c, float factor);
double  GetProcessRunningTime();

void RunNonElevated(const WCHAR *exePath);
void VariantInitBstr(VARIANT& urlVar, const WCHAR *s);
char *LoadTextResource(int resId, size_t *sizeOut=NULL);

#endif
