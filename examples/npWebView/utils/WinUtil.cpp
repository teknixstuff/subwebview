/* Copyright 2014 the SumatraPDF project authors (see AUTHORS file).
   License: Simplified BSD (see COPYING.BSD) */

#include "BaseUtil.h"
#include "FileUtil.h"
#include "WinUtil.h"
#include <io.h>
#include <fcntl.h>
#include <mlang.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "gdi32.lib")

// Loads a DLL explicitly from the system's library collection
HMODULE SafeLoadLibrary(const WCHAR *dllName)
{
    WCHAR dllPath[MAX_PATH];
    GetSystemDirectory(dllPath, dimof(dllPath));
    PathAppend(dllPath, dllName);
    return LoadLibrary(dllPath);
}

FARPROC LoadDllFunc(const WCHAR *dllName, const char *funcName)
{
    HMODULE h = SafeLoadLibrary(dllName);
    if (!h)
        return NULL;
    return GetProcAddress(h, funcName);

    // Note: we don't unload the dll. It's harmless for those that would stay
    // loaded anyway but we would crash trying to call a function that
    // was grabbed from a dll that was unloaded in the meantime
}

void InitAllCommonControls()
{
    INITCOMMONCONTROLSEX cex = { 0 };
    cex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    cex.dwICC = ICC_WIN95_CLASSES | ICC_DATE_CLASSES | ICC_USEREX_CLASSES | ICC_COOL_CLASSES ;
    InitCommonControlsEx(&cex);
}

void FillWndClassEx(WNDCLASSEX& wcex, HINSTANCE hInstance, const WCHAR *clsName, WNDPROC wndproc)
{
    ZeroMemory(&wcex, sizeof(WNDCLASSEX));
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.hInstance      = hInstance;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName  = clsName;
    wcex.lpfnWndProc    = wndproc;
}

// Return true if application is themed. Wrapper around IsAppThemed() in uxtheme.dll
// that is compatible with earlier windows versions.
bool IsAppThemed()
{
    FARPROC pIsAppThemed = LoadDllFunc(L"uxtheme.dll", "IsAppThemed");
    if (!pIsAppThemed)
        return false;
    if (pIsAppThemed())
        return true;
    return false;
}

/* Vista is major: 6, minor: 0 */
bool IsVistaOrGreater()
{
    OSVERSIONINFOEX osver = { 0 };
    ULONGLONG condMask = 0;
    osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osver.dwMajorVersion = 6;
    VER_SET_CONDITION(condMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    return VerifyVersionInfo(&osver, VER_MAJORVERSION, condMask);
}

bool IsRunningInWow64()
{
#ifndef _WIN64
    typedef BOOL (WINAPI *IsWow64ProcessProc)(HANDLE, PBOOL);
    IsWow64ProcessProc _IsWow64Process = (IsWow64ProcessProc)LoadDllFunc(L"kernel32.dll", "IsWow64Process");
    BOOL isWow = FALSE;
    if (_IsWow64Process)
        _IsWow64Process(GetCurrentProcess(), &isWow);
    return isWow;
#else
    return false;
#endif
}

// return true if a given registry key (path) exists
bool RegKeyExists(HKEY keySub, const WCHAR *keyName)
{
    HKEY hKey;
    LONG res = RegOpenKey(keySub, keyName, &hKey);
    if (ERROR_SUCCESS == res) {
        RegCloseKey(hKey);
        return true;
    }

    // return true for key that exists even if it's not
    // accessible by us
    return ERROR_ACCESS_DENIED == res;
}

// called needs to free() the result
WCHAR *ReadRegStr(HKEY keySub, const WCHAR *keyName, const WCHAR *valName)
{
    WCHAR *val = NULL;
    REGSAM access = KEY_READ;
    HKEY hKey;
TryAgainWOW64:
    LONG res = RegOpenKeyEx(keySub, keyName, 0, access, &hKey);
    if (ERROR_SUCCESS == res) {
        DWORD valLen;
        res = RegQueryValueEx(hKey, valName, NULL, NULL, NULL, &valLen);
        if (ERROR_SUCCESS == res) {
            val = AllocArray<WCHAR>(valLen / sizeof(WCHAR) + 1);
            res = RegQueryValueEx(hKey, valName, NULL, NULL, (LPBYTE)val, &valLen);
            if (ERROR_SUCCESS != res)
                str::ReplacePtr(&val, NULL);
        }
        RegCloseKey(hKey);
    }
    if (ERROR_FILE_NOT_FOUND == res && HKEY_LOCAL_MACHINE == keySub && KEY_READ == access) {
        // try the (non-)64-bit key as well, as HKLM\Software is not shared between 32-bit and
        // 64-bit applications per http://msdn.microsoft.com/en-us/library/aa384253(v=vs.85).aspx
#ifdef _WIN64
        access = KEY_READ | KEY_WOW64_32KEY;
#else
        access = KEY_READ | KEY_WOW64_64KEY;
#endif
        goto TryAgainWOW64;
    }
    return val;
}

bool WriteRegStr(HKEY keySub, const WCHAR *keyName, const WCHAR *valName, const WCHAR *value)
{
    LSTATUS res = SHSetValue(keySub, keyName, valName, REG_SZ, (const VOID *)value, (DWORD)(str::Len(value) + 1) * sizeof(WCHAR));
    return ERROR_SUCCESS == res;
}

bool WriteRegDWORD(HKEY keySub, const WCHAR *keyName, const WCHAR *valName, DWORD value)
{
    LSTATUS res = SHSetValue(keySub, keyName, valName, REG_DWORD, (const VOID *)&value, sizeof(DWORD));
    return ERROR_SUCCESS == res;
}

bool CreateRegKey(HKEY keySub, const WCHAR *keyName)
{
    HKEY hKey;
    if (RegCreateKeyEx(keySub, keyName, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return false;
    RegCloseKey(hKey);
    return true;
}

#pragma warning(push)
#pragma warning(disable: 6248) // "Setting a SECURITY_DESCRIPTOR's DACL to NULL will result in an unprotected object"
// try to remove any access restrictions on the key
// by granting everybody all access to this key (NULL DACL)
static void ResetRegKeyAcl(HKEY keySub, const WCHAR *keyName)
{
    HKEY hKey;
    LONG res = RegOpenKeyEx(keySub, keyName, 0, WRITE_DAC, &hKey);
    if (ERROR_SUCCESS != res)
        return;
    SECURITY_DESCRIPTOR secdesc;
    InitializeSecurityDescriptor(&secdesc, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&secdesc, TRUE, NULL, TRUE);
    RegSetKeySecurity(hKey, DACL_SECURITY_INFORMATION, &secdesc);
    RegCloseKey(hKey);
}
#pragma warning(pop)

bool DeleteRegKey(HKEY keySub, const WCHAR *keyName, bool resetACLFirst)
{
    if (resetACLFirst)
        ResetRegKeyAcl(keySub, keyName);

    LSTATUS res = SHDeleteKey(keySub, keyName);
    return ERROR_SUCCESS == res || ERROR_FILE_NOT_FOUND == res;
}

WCHAR *GetSpecialFolder(int csidl, bool createIfMissing)
{
    if (createIfMissing)
        csidl = csidl | CSIDL_FLAG_CREATE;
    WCHAR path[MAX_PATH];
    path[0] = '\0';
    HRESULT res = SHGetFolderPath(NULL, csidl, NULL, 0, path);
    if (S_OK != res)
        return NULL;
    return str::Dup(path);
}

#define PROCESS_EXECUTE_FLAGS 0x22

/* enable "NX" execution prevention for XP, 2003
 * cf. http://www.uninformed.org/?v=2&a=4 */
typedef HRESULT (WINAPI *_NtSetInformationProcess)(HANDLE ProcessHandle, UINT ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength);

#define MEM_EXECUTE_OPTION_DISABLE 0x1
#define MEM_EXECUTE_OPTION_ENABLE 0x2
#define MEM_EXECUTE_OPTION_PERMANENT 0x8
#define MEM_EXECUTE_OPTION_DISABLE_ATL 0x4

typedef BOOL (WINAPI* SetProcessDEPPolicyFunc)(DWORD dwFlags);
#ifndef PROCESS_DEP_ENABLE
#define PROCESS_DEP_ENABLE 0x1
#define PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION     0x2
#endif

void DisableDataExecution()
{
    // first try the documented SetProcessDEPPolicy
    SetProcessDEPPolicyFunc spdp;
    spdp = (SetProcessDEPPolicyFunc) LoadDllFunc(L"kernel32.dll", "SetProcessDEPPolicy");
    if (spdp) {
        spdp(PROCESS_DEP_ENABLE);
        return;
    }

    // now try undocumented NtSetInformationProcess
    _NtSetInformationProcess ntsip;
    DWORD depMode = MEM_EXECUTE_OPTION_DISABLE | MEM_EXECUTE_OPTION_DISABLE_ATL;

    ntsip = (_NtSetInformationProcess)LoadDllFunc(L"ntdll.dll", "NtSetInformationProcess");
    if (ntsip)
        ntsip(GetCurrentProcess(), PROCESS_EXECUTE_FLAGS, &depMode, sizeof(depMode));
}

// Code from http://www.halcyon.com/~ast/dload/guicon.htm
void RedirectIOToConsole()
{
    CONSOLE_SCREEN_BUFFER_INFO coninfo;
    int hConHandle;

    // allocate a console for this app
    AllocConsole();

    // set the screen buffer to be big enough to let us scroll text
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
    coninfo.dwSize.Y = 500;
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

    // redirect unbuffered STDOUT to the console
    hConHandle = _open_osfhandle((intptr_t)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
    *stdout = *_fdopen(hConHandle, "w");
    setvbuf(stdout, NULL, _IONBF, 0);

    // redirect unbuffered STDERR to the console
    hConHandle = _open_osfhandle((intptr_t)GetStdHandle(STD_ERROR_HANDLE), _O_TEXT);
    *stderr = *_fdopen(hConHandle, "w");
    setvbuf(stderr, NULL, _IONBF, 0);

    // redirect unbuffered STDIN to the console
    hConHandle = _open_osfhandle((intptr_t)GetStdHandle(STD_INPUT_HANDLE), _O_TEXT);
    *stdin = *_fdopen(hConHandle, "r");
    setvbuf(stdin, NULL, _IONBF, 0);
}

/* Return the full exe path of my own executable.
   Caller needs to free() the result. */
WCHAR *GetExePath()
{
    WCHAR buf[MAX_PATH];
    buf[0] = 0;
    GetModuleFileName(NULL, buf, dimof(buf));
    // TODO: is normalization needed here at all?
    return path::Normalize(buf);
}

static ULARGE_INTEGER FileTimeToLargeInteger(const FILETIME& ft)
{
    ULARGE_INTEGER res;
    res.LowPart = ft.dwLowDateTime;
    res.HighPart = ft.dwHighDateTime;
    return res;
}

/* Return <ft1> - <ft2> in seconds */
int FileTimeDiffInSecs(const FILETIME& ft1, const FILETIME& ft2)
{
    ULARGE_INTEGER t1 = FileTimeToLargeInteger(ft1);
    ULARGE_INTEGER t2 = FileTimeToLargeInteger(ft2);
    // diff is in 100 nanoseconds
    LONGLONG diff = t1.QuadPart - t2.QuadPart;
    diff = diff / (LONGLONG)10000000L;
    return (int)diff;
}

WCHAR *ResolveLnk(const WCHAR * path)
{
    ScopedMem<OLECHAR> olePath(str::Dup(path));
    if (!olePath)
        return NULL;

    ScopedComPtr<IShellLink> lnk;
    if (!lnk.Create(CLSID_ShellLink))
        return NULL;

    ScopedComQIPtr<IPersistFile> file(lnk);
    if (!file)
        return NULL;

    HRESULT hRes = file->Load(olePath, STGM_READ);
    if (FAILED(hRes))
        return NULL;

    hRes = lnk->Resolve(NULL, SLR_UPDATE);
    if (FAILED(hRes))
        return NULL;

    WCHAR newPath[MAX_PATH];
    hRes = lnk->GetPath(newPath, MAX_PATH, NULL, 0);
    if (FAILED(hRes))
        return NULL;

    return str::Dup(newPath);
}

bool CreateShortcut(const WCHAR *shortcutPath, const WCHAR *exePath,
                    const WCHAR *args, const WCHAR *description, int iconIndex)
{
    ScopedCom com;

    ScopedComPtr<IShellLink> lnk;
    if (!lnk.Create(CLSID_ShellLink))
        return false;

    ScopedComQIPtr<IPersistFile> file(lnk);
    if (!file)
        return false;

    HRESULT hr = lnk->SetPath(exePath);
    if (FAILED(hr))
        return false;

    lnk->SetWorkingDirectory(ScopedMem<WCHAR>(path::GetDir(exePath)));
    // lnk->SetShowCmd(SW_SHOWNORMAL);
    // lnk->SetHotkey(0);
    lnk->SetIconLocation(exePath, iconIndex);
    if (args)
        lnk->SetArguments(args);
    if (description)
        lnk->SetDescription(description);

    hr = file->Save(shortcutPath, TRUE);
    return SUCCEEDED(hr);
}

/* adapted from http://blogs.msdn.com/oldnewthing/archive/2004/09/20/231739.aspx */
IDataObject* GetDataObjectForFile(const WCHAR *filePath, HWND hwnd)
{
    ScopedComPtr<IShellFolder> pDesktopFolder;
    HRESULT hr = SHGetDesktopFolder(&pDesktopFolder);
    if (FAILED(hr))
        return NULL;

    IDataObject* pDataObject = NULL;
    ScopedMem<WCHAR> lpWPath(str::Dup(filePath));
    LPITEMIDLIST pidl;
    hr = pDesktopFolder->ParseDisplayName(NULL, NULL, lpWPath, NULL, &pidl, NULL);
    if (SUCCEEDED(hr)) {
        ScopedComPtr<IShellFolder> pShellFolder;
        LPCITEMIDLIST pidlChild;
        hr = SHBindToParent(pidl, IID_IShellFolder, (void**)&pShellFolder, &pidlChild);
        if (SUCCEEDED(hr))
            pShellFolder->GetUIObjectOf(hwnd, 1, &pidlChild, IID_IDataObject, NULL, (void **)&pDataObject);
        CoTaskMemFree(pidl);
    }

    return pDataObject;
}

// The result value contains major and minor version in the high resp. the low WORD
DWORD GetFileVersion(const WCHAR *path)
{
    DWORD fileVersion = 0;
    DWORD handle;
    DWORD size = GetFileVersionInfoSize(path, &handle);
    ScopedMem<void> versionInfo(malloc(size));

    if (versionInfo && GetFileVersionInfo(path, handle, size, versionInfo)) {
        VS_FIXEDFILEINFO *fileInfo;
        UINT len;
        if (VerQueryValue(versionInfo, L"\\", (LPVOID *)&fileInfo, &len))
            fileVersion = fileInfo->dwFileVersionMS;
    }

    return fileVersion;
}

bool LaunchFile(const WCHAR *path, const WCHAR *params, const WCHAR *verb, bool hidden)
{
    if (!path)
        return false;

    SHELLEXECUTEINFO sei = { 0 };
    sei.cbSize  = sizeof(sei);
    sei.fMask   = SEE_MASK_FLAG_NO_UI;
    sei.lpVerb  = verb;
    sei.lpFile  = path;
    sei.lpParameters = params;
    sei.nShow   = hidden ? SW_HIDE : SW_SHOWNORMAL;
    return ShellExecuteEx(&sei);
}

HANDLE LaunchProcess(const WCHAR *cmdLine, const WCHAR *currDir, DWORD flags)
{
    PROCESS_INFORMATION pi = { 0 };
    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);

    // CreateProcess() might modify cmd line argument, so make a copy
    // in case caller provides a read-only string
    ScopedMem<WCHAR> cmdLineCopy(str::Dup(cmdLine));
    if (!CreateProcess(NULL, cmdLineCopy, NULL, NULL, FALSE, flags, NULL, currDir, &si, &pi))
        return NULL;

    CloseHandle(pi.hThread);
    return pi.hProcess;
}

/* Ensure that the rectangle is at least partially in the work area on a
   monitor. The rectangle is shifted into the work area if necessary. */
RectI ShiftRectToWorkArea(RectI rect, bool bFully)
{
    RectI monitor = GetWorkAreaRect(rect);

    if (rect.y + rect.dy <= monitor.y || bFully && rect.y < monitor.y)
        /* Rectangle is too far above work area */
        rect.Offset(0, monitor.y - rect.y);
    else if (rect.y >= monitor.y + monitor.dy || bFully && rect.y + rect.dy > monitor.y + monitor.dy)
        /* Rectangle is too far below */
        rect.Offset(0, monitor.y - rect.y + monitor.dy - rect.dy);

    if (rect.x + rect.dx <= monitor.x || bFully && rect.x < monitor.x)
        /* Too far left */
        rect.Offset(monitor.x - rect.x, 0);
    else if (rect.x >= monitor.x + monitor.dx || bFully && rect.x + rect.dx > monitor.x + monitor.dx)
        /* Too far right */
        rect.Offset(monitor.x - rect.x + monitor.dx - rect.dx, 0);

    return rect;
}

RectI GetWorkAreaRect(RectI rect)
{
    MONITORINFO mi = { 0 };
    mi.cbSize = sizeof mi;
    RECT tmpRect = rect.ToRECT();
    HMONITOR monitor = MonitorFromRect(&tmpRect, MONITOR_DEFAULTTONEAREST);
    BOOL ok = GetMonitorInfo(monitor, &mi);
    if (!ok)
        SystemParametersInfo(SPI_GETWORKAREA, 0, &mi.rcWork, 0);
    return RectI::FromRECT(mi.rcWork);
}

// returns the dimensions the given window has to have in order to be a fullscreen window
RectI GetFullscreenRect(HWND hwnd)
{
    MONITORINFO mi = { 0 };
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi))
        return RectI::FromRECT(mi.rcMonitor);
    // fall back to the primary monitor
    return RectI(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
}

static BOOL CALLBACK GetMonitorRectProc(HMONITOR hMonitor, HDC hdc, LPRECT rcMonitor, LPARAM data)
{
    RectI *rcAll = (RectI *)data;
    *rcAll = rcAll->Union(RectI::FromRECT(*rcMonitor));
    return TRUE;
}

// returns the smallest rectangle that covers the entire virtual screen (all monitors)
RectI GetVirtualScreenRect()
{
    RectI result(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    EnumDisplayMonitors(NULL, NULL, GetMonitorRectProc, (LPARAM)&result);
    return result;
}

void PaintRect(HDC hdc, const RectI& rect)
{
    MoveToEx(hdc, rect.x, rect.y, NULL);
    LineTo(hdc, rect.x + rect.dx - 1, rect.y);
    LineTo(hdc, rect.x + rect.dx - 1, rect.y + rect.dy - 1);
    LineTo(hdc, rect.x, rect.y + rect.dy - 1);
    LineTo(hdc, rect.x, rect.y);
}

void PaintLine(HDC hdc, const RectI& rect)
{
    MoveToEx(hdc, rect.x, rect.y, NULL);
    LineTo(hdc, rect.x + rect.dx, rect.y + rect.dy);
}

void DrawCenteredText(HDC hdc, const RectI& r, const WCHAR *txt, bool isRTL)
{
    SetBkMode(hdc, TRANSPARENT);
    RECT tmpRect = r.ToRECT();
    DrawText(hdc, txt, -1, &tmpRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | (isRTL ? DT_RTLREADING : 0));
}

/* Return size of a text <txt> in a given <hwnd>, taking into account its font */
SizeI TextSizeInHwnd(HWND hwnd, const WCHAR *txt)
{
    SIZE sz;
    size_t txtLen = str::Len(txt);
    HDC dc = GetWindowDC(hwnd);
    /* GetWindowDC() returns dc with default state, so we have to first set
       window's current font into dc */
    HFONT f = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
    HGDIOBJ prev = SelectObject(dc, f);
    GetTextExtentPoint32(dc, txt, (int)txtLen, &sz);
    SelectObject(dc, prev);
    ReleaseDC(hwnd, dc);
    return SizeI(sz.cx, sz.cy);
}

bool IsCursorOverWindow(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);
    WindowRect rcWnd(hwnd);
    return rcWnd.Contains(PointI(pt.x, pt.y));
}

bool GetCursorPosInHwnd(HWND hwnd, POINT& posOut)
{
    if (!GetCursorPos(&posOut))
        return false;
    if (!ScreenToClient(hwnd, &posOut))
        return false;
    return true;
}

void CenterDialog(HWND hDlg, HWND hParent)
{
    if (!hParent)
        hParent = GetParent(hDlg);

    RectI rcDialog = WindowRect(hDlg);
    rcDialog.Offset(-rcDialog.x, -rcDialog.y);
    RectI rcOwner = WindowRect(hParent ? hParent : GetDesktopWindow());
    RectI rcRect = rcOwner;
    rcRect.Offset(-rcRect.x, -rcRect.y);

    // center dialog on its parent window
    rcDialog.Offset(rcOwner.x + (rcRect.x - rcDialog.x + rcRect.dx - rcDialog.dx) / 2,
                    rcOwner.y + (rcRect.y - rcDialog.y + rcRect.dy - rcDialog.dy) / 2);
    // ensure that the dialog is fully visible on one monitor
    rcDialog = ShiftRectToWorkArea(rcDialog, true);

    SetWindowPos(hDlg, 0, rcDialog.x, rcDialog.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

bool CopyTextToClipboard(const WCHAR *text, bool appendOnly)
{
    assert(text);
    if (!text) return false;

    if (!appendOnly) {
        if (!OpenClipboard(NULL))
            return false;
        EmptyClipboard();
    }

    HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, (str::Len(text) + 1) * sizeof(WCHAR));
    if (handle) {
        WCHAR *globalText = (WCHAR *)GlobalLock(handle);
        lstrcpy(globalText, text);
        GlobalUnlock(handle);

        SetClipboardData(CF_UNICODETEXT, handle);
    }

    if (!appendOnly)
        CloseClipboard();

    return handle != NULL;
}

bool CopyImageToClipboard(HBITMAP hbmp, bool appendOnly)
{
    if (!appendOnly) {
        if (!OpenClipboard(NULL))
            return false;
        EmptyClipboard();
    }

    bool ok = false;
    if (hbmp) {
        BITMAP bmpInfo;
        GetObject(hbmp, sizeof(BITMAP), &bmpInfo);
        if (bmpInfo.bmBits != NULL) {
            // GDI+ produced HBITMAPs are DIBs instead of DDBs which
            // aren't correctly handled by the clipboard, so create a
            // clipboard-safe clone
            ScopedGdiObj<HBITMAP> ddbBmp((HBITMAP)CopyImage(hbmp,
                IMAGE_BITMAP, bmpInfo.bmWidth, bmpInfo.bmHeight, 0));
            ok = SetClipboardData(CF_BITMAP, ddbBmp) != NULL;
        }
        else
            ok = SetClipboardData(CF_BITMAP, hbmp) != NULL;
    }

    if (!appendOnly)
        CloseClipboard();

    return ok;
}

void ToggleWindowStyle(HWND hwnd, DWORD flag, bool enable, int type)
{
    DWORD style = GetWindowLong(hwnd, type);
    if (enable)
        style = style | flag;
    else
        style = style & ~flag;
    SetWindowLong(hwnd, type, style);
}

DoubleBuffer::DoubleBuffer(HWND hwnd, RectI rect) :
    hTarget(hwnd), rect(rect), hdcBuffer(NULL), doubleBuffer(NULL)
{
    hdcCanvas = ::GetDC(hwnd);

    if (rect.IsEmpty())
        return;

    doubleBuffer = CreateCompatibleBitmap(hdcCanvas, rect.dx, rect.dy);
    if (!doubleBuffer)
        return;

    hdcBuffer = CreateCompatibleDC(hdcCanvas);
    if (!hdcBuffer)
        return;

    if (rect.x != 0 || rect.y != 0) {
        SetGraphicsMode(hdcBuffer, GM_ADVANCED);
        XFORM ctm = { 1.0, 0, 0, 1.0, (float)-rect.x, (float)-rect.y };
        SetWorldTransform(hdcBuffer, &ctm);
    }
    DeleteObject(SelectObject(hdcBuffer, doubleBuffer));
}

DoubleBuffer::~DoubleBuffer()
{
    DeleteObject(doubleBuffer);
    DeleteDC(hdcBuffer);
    ReleaseDC(hTarget, hdcCanvas);
}

void DoubleBuffer::Flush(HDC hdc)
{
    assert(hdc != hdcBuffer);
    if (hdcBuffer)
        BitBlt(hdc, rect.x, rect.y, rect.dx, rect.dy, hdcBuffer, 0, 0, SRCCOPY);
}

namespace win {
    namespace menu {

void SetText(HMENU m, UINT id, WCHAR *s)
{
    MENUITEMINFO mii = { 0 };
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING;
    mii.fType = MFT_STRING;
    mii.dwTypeData = s;
    mii.cch = (UINT)str::Len(s);
    SetMenuItemInfo(m, id, FALSE, &mii);
}

/* Make a string safe to be displayed as a menu item
   (preserving all & so that they don't get swallowed)
   Caller needs to free() the result. */
WCHAR *ToSafeString(const WCHAR *str)
{
    return str::Replace(str, L"&", L"&&");
}

    }
}

HFONT CreateSimpleFont(HDC hdc, const WCHAR *fontName, int fontSize)
{
    LOGFONT lf = { 0 };

    lf.lfWidth = 0;
    lf.lfHeight = -MulDiv(fontSize, GetDeviceCaps(hdc, LOGPIXELSY), USER_DEFAULT_SCREEN_DPI);
    lf.lfItalic = FALSE;
    lf.lfUnderline = FALSE;
    lf.lfStrikeOut = FALSE;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH;
    str::BufSet(lf.lfFaceName, dimof(lf.lfFaceName), fontName);
    lf.lfWeight = FW_DONTCARE;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;

    return CreateFontIndirect(&lf);
}

IStream *CreateStreamFromData(const void *data, size_t len)
{
    if (!data)
        return NULL;

    ScopedComPtr<IStream> stream;
    if (FAILED(CreateStreamOnHGlobal(NULL, TRUE, &stream)))
        return NULL;

    ULONG written;
    if (FAILED(stream->Write(data, (ULONG)len, &written)) || written != len)
        return NULL;

    LARGE_INTEGER zero = { 0 };
    stream->Seek(zero, STREAM_SEEK_SET, NULL);

    stream->AddRef();
    return stream;
}

static HRESULT GetDataFromStream(IStream *stream, void **data, ULONG *len)
{
    if (!stream)
        return E_INVALIDARG;

    STATSTG stat;
    HRESULT res = stream->Stat(&stat, STATFLAG_NONAME);
    if (FAILED(res))
        return res;
    assert(0 == stat.cbSize.HighPart);
    if (stat.cbSize.HighPart > 0 || stat.cbSize.LowPart > UINT_MAX - sizeof(WCHAR))
        return E_OUTOFMEMORY;

    // zero-terminate the stream's content, so that it could be
    // used directly as either a char* or a WCHAR* string
    *len = stat.cbSize.LowPart;
    *data = malloc(*len + sizeof(WCHAR));
    if (!*data)
        return E_OUTOFMEMORY;

    ULONG read;
    LARGE_INTEGER zero = { 0 };
    stream->Seek(zero, STREAM_SEEK_SET, NULL);
    res = stream->Read(*data, stat.cbSize.LowPart, &read);
    if (FAILED(res) || read != *len) {
        free(*data);
        return res;
    }

    ((char *)*data)[*len] = '\0';
    ((char *)*data)[*len + 1] = '\0';

    return S_OK;
}

void *GetDataFromStream(IStream *stream, size_t *len, HRESULT *res_opt)
{
    void *data;
    ULONG size;
    HRESULT res = GetDataFromStream(stream, &data, &size);
    if (len)
        *len = size;
    if (res_opt)
        *res_opt = res;
    if (FAILED(res))
        return NULL;
    return data;
}

bool ReadDataFromStream(IStream *stream, void *buffer, size_t len, size_t offset)
{
    LARGE_INTEGER off;
    off.QuadPart = offset;
    HRESULT res = stream->Seek(off, STREAM_SEEK_SET, NULL);
    if (FAILED(res))
        return false;
    ULONG read;
#ifdef _WIN64
    for (; len > ULONG_MAX; len -= ULONG_MAX) {
        res = stream->Read(buffer, ULONG_MAX, &read);
        if (FAILED(res) || read != ULONG_MAX)
            return false;
        len -= ULONG_MAX;
        buffer = (char *)buffer + ULONG_MAX;
    }
#endif
    res = stream->Read(buffer, (ULONG)len, &read);
    return SUCCEEDED(res) && read == len;
}

UINT GuessTextCodepage(const char *data, size_t len, UINT def)
{
    // try to guess the codepage
    ScopedComPtr<IMultiLanguage2> pMLang;
    if (!pMLang.Create(CLSID_CMultiLanguage))
        return def;

    int ilen = (int)min(len, INT_MAX);
    int count = 1;
    DetectEncodingInfo info = { 0 };
    HRESULT hr = pMLang->DetectInputCodepage(MLDETECTCP_NONE, CP_ACP, (char *)data,
                                             &ilen, &info, &count);
    if (FAILED(hr) || count != 1)
        return def;
    return info.nCodePage;
}

WCHAR *NormalizeString(const WCHAR *str, int /* NORM_FORM */ form)
{
    typedef int (WINAPI *NormalizeStringProc)(int /* NORM_FORM */, LPCWSTR, int, LPWSTR, int);
    NormalizeStringProc _NormalizeString = (NormalizeStringProc)LoadDllFunc(L"Normaliz.dll", "NormalizeString");
    if (!_NormalizeString)
        return NULL;
    int sizeEst = _NormalizeString(form, str, -1, NULL, 0);
    if (sizeEst <= 0)
        return NULL;
    // according to MSDN the estimate may be off somewhat:
    // http://msdn.microsoft.com/en-us/library/windows/desktop/dd319093(v=vs.85).aspx
    sizeEst = sizeEst * 3 / 2 + 1;
    ScopedMem<WCHAR> res(AllocArray<WCHAR>(sizeEst));
    sizeEst = _NormalizeString(form, str, -1, res, sizeEst);
    if (sizeEst <= 0)
        return NULL;
    return res.StealData();
}

namespace win {

WCHAR *GetText(HWND hwnd)
{
    size_t  cchTxtLen = GetTextLen(hwnd);
    WCHAR * txt = AllocArray<WCHAR>(cchTxtLen + 1);
    if (NULL == txt)
        return NULL;
    SendMessage(hwnd, WM_GETTEXT, cchTxtLen + 1, (LPARAM)txt);
    txt[cchTxtLen] = 0;
    return txt;
}

int GetHwndDpi(HWND hwnd, float *uiDPIFactor)
{
    HDC dc = GetDC(hwnd);
    int dpi = GetDeviceCaps(dc, LOGPIXELSY);
    // round untypical resolutions up to the nearest quarter
    if (uiDPIFactor)
        *uiDPIFactor = ceil(dpi * 4.0f / USER_DEFAULT_SCREEN_DPI) / 4.0f;
    ReleaseDC(hwnd, dc);
    return dpi;
}

int GlobalDpiAdjust(int value)
{
    static float dpiFactor = 0.f;
    if (0.f == dpiFactor) {
        win::GetHwndDpi(HWND_DESKTOP, &dpiFactor);
        if (0.f == dpiFactor)
            dpiFactor = 1.f;
    }
    return (int)(value * dpiFactor);
}

int GlobalDpiAdjust(float value)
{
    static float dpiFactor = 0.f;
    if (0.f == dpiFactor) {
        win::GetHwndDpi(HWND_DESKTOP, &dpiFactor);
        if (0.f == dpiFactor)
            dpiFactor = 1.f;
    }
    return (int)(value * dpiFactor);
}

}

SizeI GetBitmapSize(HBITMAP hbmp)
{
    BITMAP bmpInfo;
    GetObject(hbmp, sizeof(BITMAP), &bmpInfo);
    return SizeI(bmpInfo.bmWidth, bmpInfo.bmHeight);
}

// cf. fz_mul255 in fitz.h
inline int mul255(int a, int b)
{
    int x = a * b + 128;
    x += x >> 8;
    return x >> 8;
}

void UpdateBitmapColors(HBITMAP hbmp, COLORREF textColor, COLORREF bgColor)
{
    if ((textColor & 0xFFFFFF) == WIN_COL_BLACK &&
        (bgColor & 0xFFFFFF) == WIN_COL_WHITE)
        return;

    // color order in DIB is blue-green-red-alpha
    int base[4] = { GetBValueSafe(textColor), GetGValueSafe(textColor), GetRValueSafe(textColor), 0 };
    int diff[4] = {
        GetBValueSafe(bgColor) - base[0],
        GetGValueSafe(bgColor) - base[1],
        GetRValueSafe(bgColor) - base[2],
        255
    };

    HDC hDC = GetDC(NULL);
    BITMAPINFO bmi = { 0 };
    SizeI size = GetBitmapSize(hbmp);

    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = size.dx;
    bmi.bmiHeader.biHeight = size.dy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    int bmpBytes = size.dx * size.dy * 4;
    ScopedMem<unsigned char> bmpData((unsigned char *)malloc(bmpBytes));
    CrashIf(!bmpData);
    if (GetDIBits(hDC, hbmp, 0, size.dy, bmpData, &bmi, DIB_RGB_COLORS)) {
        for (int i = 0; i < bmpBytes; i++) {
            int k = i % 4;
            bmpData[i] = (uint8_t)(base[k] + mul255(bmpData[i], diff[k]));
        }
        SetDIBits(hDC, hbmp, 0, size.dy, bmpData, &bmi, DIB_RGB_COLORS);
    }

    ReleaseDC(NULL, hDC);
}

// create data for a .bmp file from this bitmap (if saved to disk, the HBITMAP
// can be deserialized with LoadImage(NULL, ..., LD_LOADFROMFILE) and its
// dimensions determined again with GetBitmapSize(...))
unsigned char *SerializeBitmap(HBITMAP hbmp, size_t *bmpBytesOut)
{
    SizeI size = GetBitmapSize(hbmp);
    DWORD bmpHeaderLen = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO);
    DWORD bmpBytes = ((size.dx * 3 + 3) / 4) * 4 * size.dy + bmpHeaderLen;
    unsigned char *bmpData = AllocArray<unsigned char>(bmpBytes);
    if (!bmpData)
        return NULL;

    BITMAPINFO *bmi = (BITMAPINFO *)(bmpData + sizeof(BITMAPFILEHEADER));
    bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
    bmi->bmiHeader.biWidth = size.dx;
    bmi->bmiHeader.biHeight = size.dy;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biBitCount = 24;
    bmi->bmiHeader.biCompression = BI_RGB;

    HDC hDC = GetDC(NULL);
    if (GetDIBits(hDC, hbmp, 0, size.dy, bmpData + bmpHeaderLen, bmi, DIB_RGB_COLORS)) {
        BITMAPFILEHEADER *bmpfh = (BITMAPFILEHEADER *)bmpData;
        bmpfh->bfType = MAKEWORD('B', 'M');
        bmpfh->bfOffBits = bmpHeaderLen;
        bmpfh->bfSize = bmpBytes;
    } else {
        free(bmpData);
        bmpData = NULL;
    }
    ReleaseDC(NULL, hDC);

    if (bmpBytesOut)
        *bmpBytesOut = bmpBytes;
    return bmpData;
}

COLORREF AdjustLightness(COLORREF c, float factor)
{
    BYTE R = GetRValueSafe(c), G = GetGValueSafe(c), B = GetBValueSafe(c);
    // cf. http://en.wikipedia.org/wiki/HSV_color_space#Hue_and_chroma
    BYTE M = max(max(R, G), B), m = min(min(R, G), B);
    if (M == m) {
        // for grayscale values, lightness is proportional to the color value
        BYTE X = (BYTE)limitValue((int)floorf(M * factor + 0.5f), 0, 255);
        return RGB(X, X, X);
    }
    BYTE C = M - m;
    BYTE Ha = (BYTE)abs(M == R ? G - B : M == G ? B - R : R - G);
    // cf. http://en.wikipedia.org/wiki/HSV_color_space#Lightness
    float L2 = (float)(M + m);
    // cf. http://en.wikipedia.org/wiki/HSV_color_space#Saturation
    float S = C / (L2 > 255.0f ? 510.0f - L2 : L2);

    L2 = limitValue(L2 * factor, 0.0f, 510.0f);
    // cf. http://en.wikipedia.org/wiki/HSV_color_space#From_HSL
    float C1 = (L2 > 255.0f ? 510.0f - L2 : L2) * S;
    float X1 = C1 * Ha / C;
    float m1 = (L2 - C1) / 2;
    R = (BYTE)floorf((M == R ? C1 : m != R ? X1 : 0) + m1 + 0.5f);
    G = (BYTE)floorf((M == G ? C1 : m != G ? X1 : 0) + m1 + 0.5f);
    B = (BYTE)floorf((M == B ? C1 : m != B ? X1 : 0) + m1 + 0.5f);
    return RGB(R, G, B);
}

// This is meant to measure program startup time from the user perspective.
// One place to measure it is at the beginning of WinMain().
// Another place is on the first run of WM_PAINT of the message loop of main window.
double GetProcessRunningTime()
{
    FILETIME currTime, startTime, d1, d2, d3;
    GetSystemTimeAsFileTime(&currTime);
    HANDLE hproc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
    double timeInMs = 0;
    if (!hproc)
        return 0;
    if (GetProcessTimes(hproc, &startTime, &d1, &d2, &d3)) {
        ULARGE_INTEGER start = FileTimeToLargeInteger(startTime);
        ULARGE_INTEGER curr = FileTimeToLargeInteger(currTime);
        ULONGLONG diff = curr.QuadPart - start.QuadPart;
        // FILETIME is in 100 ns chunks
        timeInMs = ((double)(diff * 100)) / (double)1000000;
    }
    CloseHandle(hproc);
    return timeInMs;
}

// This is just to satisfy /analyze. CloseHandle(NULL) works perfectly fine
// but /analyze complains anyway
BOOL SafeCloseHandle(HANDLE *h)
{
    if (!*h)
        return TRUE;
    BOOL ok = CloseHandle(*h);
    *h = NULL;
    return ok;
}

// This is just to satisfy /analyze. DestroyWindow(NULL) works perfectly fine
// but /analyze complains anyway
BOOL SafeDestroyWindow(HWND *hwnd)
{
    if (!hwnd || !*hwnd)
        return TRUE;
    BOOL ok = DestroyWindow(*hwnd);
    *hwnd = NULL;
    return ok;
}

// based on http://mdb-blog.blogspot.com/2013/01/nsis-lunch-program-as-user-from-uac.html
// uses $WINDIR\explorer.exe to launch cmd
// Other promising approaches:
// - http://blogs.msdn.com/b/oldnewthing/archive/2013/11/18/10468726.aspx
// - http://brandonlive.com/2008/04/27/getting-the-shell-to-run-an-application-for-you-part-2-how/
// - http://www.codeproject.com/Articles/23090/Creating-a-process-with-Medium-Integration-Level-f
// Approaches tried but didn't work:
// - http://stackoverflow.com/questions/3298611/run-my-program-asuser
// - using CreateProcessAsUser() with hand-crafted token
// It'll always run the process, might fail to run non-elevated if fails to find explorer.exe
// Also, if explorer.exe is running elevated, it'll probably run elevated as well.
void RunNonElevated(const WCHAR *exePath)
{
    ScopedMem<WCHAR> cmd, explorerPath;
    WCHAR buf[MAX_PATH] = { 0 };
    UINT res = GetWindowsDirectory(buf, dimof(buf));
    if (0 == res || res >= dimof(buf))
        goto Run;
    explorerPath.Set(path::Join(buf, L"explorer.exe"));
    if (!file::Exists(explorerPath))
        goto Run;
    cmd.Set(str::Format(L"\"%s\" \"%s\"", explorerPath.Get(), exePath));
Run:
    HANDLE h = LaunchProcess(cmd ? cmd : exePath);
    SafeCloseHandle(&h);
}

// Note: MS_ENH_RSA_AES_PROV_XP isn't defined in the SDK shipping with VS2008
#ifndef MS_ENH_RSA_AES_PROV_XP
#define MS_ENH_RSA_AES_PROV_XP L"Microsoft Enhanced RSA and AES Cryptographic Provider (Prototype)"
#endif
#ifndef PROV_RSA_AES
#define PROV_RSA_AES 24
#endif

// MD5 digest that uses Windows' CryptoAPI. It's good for code that doesn't already
// have MD5 code (smaller code) and it's probably faster than most other implementations
// TODO: could try to use CryptoNG available starting in Vista. But then again, would that be worth it?
void CalcMD5DigestWin(const void *data, size_t byteCount, unsigned char digest[16])
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;

    // http://stackoverflow.com/questions/9794745/ms-cryptoapi-doesnt-work-on-windows-xp-with-cryptacquirecontext
    BOOL ok = CryptAcquireContext(&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    if (!ok)
        ok = CryptAcquireContext(&hProv, NULL, MS_ENH_RSA_AES_PROV_XP, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);

    CrashAlwaysIf(!ok);
    ok = CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
    CrashAlwaysIf(!ok);
    CrashAlwaysIf(byteCount > UINT_MAX);
    ok = CryptHashData(hHash, (const BYTE*)data, (DWORD)byteCount, 0);
    CrashAlwaysIf(!ok);

    DWORD hashLen;
    DWORD argSize = sizeof(DWORD);
    ok = CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *)&hashLen, &argSize, 0);
    CrashIf(sizeof(DWORD) != argSize);
    CrashAlwaysIf(!ok);
    CrashAlwaysIf(16 != hashLen);
    ok = CryptGetHashParam(hHash, HP_HASHVAL, digest, &hashLen, 0);
    CrashAlwaysIf(!ok);
    CrashAlwaysIf(16 != hashLen);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv,0);
}

// SHA1 digest that uses Windows' CryptoAPI. It's good for code that doesn't already
// have SHA1 code (smaller code) and it's probably faster than most other implementations
// TODO: hasn't been tested for corectness
void CalcSha1DigestWin(void *data, size_t byteCount, unsigned char digest[32])
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;

    BOOL ok = CryptAcquireContext(&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    if (!ok) {
        // TODO: test this on XP
        ok = CryptAcquireContext(&hProv, NULL, MS_ENH_RSA_AES_PROV_XP, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
    }
    CrashAlwaysIf(!ok);
    ok = CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);
    CrashAlwaysIf(!ok);
    CrashAlwaysIf(byteCount > UINT_MAX);
    ok = CryptHashData(hHash, (const BYTE*)data, (DWORD)byteCount, 0);
    CrashAlwaysIf(!ok);

    DWORD hashLen;
    DWORD argSize = sizeof(DWORD);
    ok = CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *)&hashLen, &argSize, 0);
    CrashIf(sizeof(DWORD) != argSize);
    CrashAlwaysIf(!ok);
    CrashAlwaysIf(32 != hashLen);
    ok = CryptGetHashParam(hHash, HP_HASHVAL, digest, &hashLen, 0);
    CrashAlwaysIf(!ok);
    CrashAlwaysIf(32 != hashLen);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv,0);
}

static int RectDx(RECT& r)
{
    return r.right - r.left;
}

static int RectDy(RECT& r)
{
    return r.bottom - r.top;
}

void ResizeHwndToClientArea(HWND hwnd, int dx, int dy, bool hasMenu)
{
    WINDOWINFO wi = { 0 };
    wi.cbSize = sizeof(wi);
    GetWindowInfo(hwnd, &wi);

    RECT r = { 0 };
    r.right = dx; r.bottom = dy;
    DWORD style = wi.dwStyle;
    DWORD exStyle = wi.dwExStyle;
    AdjustWindowRectEx(&r, style, hasMenu, exStyle);
    if ((dx == RectDx(wi.rcClient)) && (dy == RectDy(wi.rcClient)))
        return;

    dx = RectDx(r); dy = RectDy(r);
    int x = wi.rcWindow.left;
    int y = wi.rcWindow.top;
    MoveWindow(hwnd, x, y, dx, dy, TRUE);
}

void VariantInitBstr(VARIANT& urlVar, const WCHAR *s)
{
    VariantInit(&urlVar);
    urlVar.vt = VT_BSTR;
    urlVar.bstrVal = SysAllocString(s);
}

char *LoadTextResource(int resId, size_t *sizeOut)
{
    HRSRC resSrc = FindResource(NULL, MAKEINTRESOURCE(resId), RT_RCDATA);
    CrashIf(!resSrc);
    HGLOBAL res = LoadResource(NULL, resSrc);
    CrashIf(!res);
    DWORD size = SizeofResource(NULL, resSrc);
    const char *resData = (const char*)LockResource(res);
    char *s = str::DupN(resData, size);
    if (sizeOut)
        *sizeOut = size;
    UnlockResource(res);
    return s;
}
