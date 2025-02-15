#include "examples/shared/main.h"
#include "examples/shared/browser_util.h"
#include "examples/minimal/client_minimal.h"
#include "include/cef_sandbox_win.h"
#include "include/base/cef_callback.h"
#include "include/wrapper/cef_closure_task.h"
#include "examples/shared/app_factory.h"
#include "examples/shared/client_manager.h"
#include "examples/shared/main_util.h"

#include "utils/StrUtil.cpp"
#include "utils/FileUtil.cpp"
#include "Version.h"
#include "utils/WinUtil.cpp"
#ifndef _WINDOWS
#define _WINDOWS
#endif
#include "npapi/npfunctions.h"

HANDLE hWebViewThread;

#define WM_CEF_INVOKE_POPUP (WM_USER + 0x0001)
#define WM_CEF_SET_TITLE (WM_USER + 0x0002)

#define DLLEXPORT extern "C"
#ifdef _WIN64
#pragma comment(linker, "/EXPORT:NP_GetEntryPoints=NP_GetEntryPoints,PRIVATE")
#pragma comment(linker, "/EXPORT:NP_Initialize=NP_Initialize,PRIVATE")
#pragma comment(linker, "/EXPORT:NP_Shutdown=NP_Shutdown,PRIVATE")
#pragma comment(linker, "/EXPORT:DllRegisterServer=DllRegisterServer,PRIVATE")
#pragma comment(linker, "/EXPORT:DllUnregisterServer=DllUnregisterServer,PRIVATE")
#else
#pragma comment(linker, "/EXPORT:NP_GetEntryPoints=_NP_GetEntryPoints@4,PRIVATE")
#pragma comment(linker, "/EXPORT:NP_Initialize=_NP_Initialize@4,PRIVATE")
#pragma comment(linker, "/EXPORT:NP_Shutdown=_NP_Shutdown@0,PRIVATE")
#pragma comment(linker, "/EXPORT:DllRegisterServer=_DllRegisterServer@0,PRIVATE")
#pragma comment(linker, "/EXPORT:DllUnregisterServer=_DllUnregisterServer@0,PRIVATE")
#endif

#if NOLOG == 0
const char *DllMainReason(DWORD reason)
{
    if (DLL_PROCESS_ATTACH == reason)
        return "DLL_PROCESS_ATTACH";
    if (DLL_PROCESS_DETACH == reason)
        return "DLL_PROCESS_DETACH";
    if (DLL_THREAD_ATTACH == reason)
        return "DLL_THREAD_ATTACH";
    if (DLL_THREAD_DETACH == reason)
        return "DLL_THREAD_DETACH";
    return "UNKNOWN";
}
#endif

NPNetscapeFuncs gNPNFuncs;
HINSTANCE g_hInstance = NULL;
#ifndef _WIN64
const WCHAR *g_lpRegKey = L"Software\\MozillaPlugins\\@subwebview.teknixstuff.com/npWebView";
#else
const WCHAR *g_lpRegKey = L"Software\\MozillaPlugins\\@subwebview.teknixstuff.com/npWebView_x64";
#endif
int gTranslationIdx = 0;

DWORD cefInit = 0;
std::string* profileDir;

DWORD WINAPI CEFMainThread(LPVOID) {
    void* sandbox_info = nullptr;

#if defined(CEF_USE_SANDBOX)
    // Manage the life span of the sandbox information object. This is necessary
    // for sandbox support on Windows. See cef_sandbox_win.h for complete
    // details.
    CefScopedSandboxInfo scoped_sandbox;
    sandbox_info = scoped_sandbox.sandbox_info();
#endif

    // Provide CEF with command-line arguments.
    CefMainArgs main_args;

    // Create a temporary CommandLine object.
    CefRefPtr<CefCommandLine> command_line =
        shared::CreateCommandLine(main_args);

    // Create a CefApp of the correct process type.
    CefRefPtr<CefApp> app = shared::CreateBrowserProcessApp();

    // Create the singleton manager instance.
    shared::ClientManager manager;

    // Specify CEF global settings here.
    CefSettings settings;

#if !defined(CEF_USE_SANDBOX)
    settings.no_sandbox = true;
#endif

    CefString(&settings.root_cache_path).FromString(*profileDir + "\\SubWebView");

    // Initialize the CEF browser process. The first browser instance will be
    // created in CefBrowserProcessHandler::OnContextInitialized() after CEF has
    // been initialized. May return false if initialization fails or if early
    // exit is desired (for example, due to process singleton relaunch
    // behavior).
    if (!CefInitialize(main_args, settings, app, sandbox_info)) {
        return 1;
    }

    cefInit = 2;

    // Run the CEF message loop. This will block until CefQuitMessageLoop() is
    // called.
    CefRunMessageLoop();

    // Shut down CEF.
    CefShutdown();

    return 0;
}

/* ::::: DLL Exports ::::: */

BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    g_hInstance = hInstance;
    return TRUE;
}

DLLEXPORT NPError WINAPI NP_GetEntryPoints(NPPluginFuncs *pFuncs)
{
    if (!pFuncs || pFuncs->size < sizeof(NPPluginFuncs))
        return NPERR_INVALID_FUNCTABLE_ERROR;
    
    pFuncs->size = sizeof(NPPluginFuncs);
    pFuncs->version = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
    pFuncs->newp = NPP_New;
    pFuncs->destroy = NPP_Destroy;
    pFuncs->setwindow = NPP_SetWindow;
    pFuncs->newstream = NPP_NewStream;
    pFuncs->destroystream = NPP_DestroyStream;
    pFuncs->asfile = NPP_StreamAsFile;
    pFuncs->writeready = NPP_WriteReady;
    pFuncs->write = NPP_Write;
    pFuncs->print = NPP_Print;
    pFuncs->event = NULL;
    pFuncs->urlnotify = NULL;
    pFuncs->javaClass = NULL;
    pFuncs->getvalue = NULL;
    pFuncs->setvalue = NULL;
    
    return NPERR_NO_ERROR;
}

DLLEXPORT NPError WINAPI NP_Initialize(NPNetscapeFuncs *pFuncs)
{
    if (!pFuncs || pFuncs->size < sizeof(NPNetscapeFuncs))
    {
        return NPERR_INVALID_FUNCTABLE_ERROR;
    }
    if (HIBYTE(pFuncs->version) > NP_VERSION_MAJOR)
    {
        return NPERR_INCOMPATIBLE_VERSION_ERROR;
    }
    
    gNPNFuncs = *pFuncs;
    
    return NPERR_NO_ERROR;
}

DLLEXPORT NPError WINAPI NP_Shutdown(void)
{
    if (cefInit != 0) {
        cefInit = 1;
        CefPostTask(TID_UI, base::BindOnce(CefQuitMessageLoop));
        WaitForSingleObject(hWebViewThread, INFINITE);
        delete profileDir;
    }
    return NPERR_NO_ERROR;
}

DLLEXPORT STDAPI DllRegisterServer(VOID)
{
    HKEY hkey = HKEY_LOCAL_MACHINE;
    if (!CreateRegKey(hkey, g_lpRegKey))
    {
        hkey = HKEY_CURRENT_USER;
        if (!CreateRegKey(hkey, g_lpRegKey))
            return E_UNEXPECTED;
    }
    
    WCHAR szPath[MAX_PATH];
    GetModuleFileName(g_hInstance, szPath, MAX_PATH);
    if (!WriteRegStr(hkey, g_lpRegKey, L"Description", L"View web pages using WebView inside another browser") ||
        !WriteRegStr(hkey, g_lpRegKey, L"Path", szPath) ||
        !WriteRegStr(hkey, g_lpRegKey, L"Version", CURR_VERSION_STR) ||
        !WriteRegStr(hkey, g_lpRegKey, L"ProductName", L"SubWebView"))
    {
        return E_UNEXPECTED;
    }
    
    static const WCHAR *mimeTypes[] = {
        L"application/x-subwebview"
    };
    for (int i = 0; i < dimof(mimeTypes); i++)
    {
        ScopedMem<WCHAR> keyName(str::Join(g_lpRegKey, L"\\MimeTypes\\", mimeTypes[i]));
        CreateRegKey(hkey, keyName);
    }
    
    return S_OK;
}

DLLEXPORT STDAPI DllUnregisterServer(VOID)
{
    DeleteRegKey(HKEY_LOCAL_MACHINE, g_lpRegKey);
    DeleteRegKey(HKEY_CURRENT_USER, g_lpRegKey);
    
    return S_OK;
}

/* ::::: Auxiliary Methods ::::: */

bool GetExePath(WCHAR *lpPath, DWORD len)
{
    GetModuleFileName(g_hInstance, lpPath, len - 2);
    str::BufSet((WCHAR *)path::GetBaseName(lpPath), len - 2 - (path::GetBaseName(lpPath) - lpPath), L"SubWebView\\SubWebView.dll");
    return true;
}

int cmpWcharPtrs(const void *a, const void *b)
{
    return wcscmp(*(const WCHAR **)a, *(const WCHAR **)b);
}

/* ::::: Plugin Window Procedure ::::: */

struct InstanceData {
    NPWindow *  npwin;
    LPCWSTR     message;
};

#define COL_WINDOW_BG RGB(0xcc, 0xcc, 0xcc)

void LaunchSubWebView(InstanceData *data, const char *url_utf8, NPP npp)
{
    ScopedMem<WCHAR> url(str::conv::FromUtf8(url_utf8));
    // escape quotation marks and backslashes for CmdLineParser.cpp's ParseQuoted
    if (str::FindChar(url, '"')) {
        WStrVec parts;
        parts.Split(url, L"\"");
        url.Set(parts.Join(L"%22"));
    }
    if (str::EndsWith(url, L"\\")) {
        url[str::Len(url) - 1] = '\0';
        url.Set(str::Join(url, L"%5c"));
    }

    if (cefInit == 0) {
        cefInit = 1;
        NPObject* window = NULL;
        gNPNFuncs.getvalue(npp, NPNVWindowNPObject, &window);
        NPIdentifier profileId = gNPNFuncs.getstringidentifier("subWebViewProfile");
        NPVariant profileVar;
        gNPNFuncs.getproperty(npp, window, profileId, &profileVar);
        NPString profile = NPVARIANT_TO_STRING(profileVar);
        profileDir = new std::string(profile.utf8characters, profile.utf8length);
        hWebViewThread = CreateThread(NULL, 0, CEFMainThread, NULL, 0, NULL);
    }

    data->message = L"Waiting for SubWebView...";
    while (cefInit != 2) {
        Sleep(100);
    }

    // Create the browser window.
    auto client = new minimal::Client();
    client->hPluginWnd = (HWND)data->npwin->window;
    client->profileDir = profileDir;
    CefWindowInfo window_info;
    window_info.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
    CefRect rect;
    rect.Set(0, 0, data->npwin->width, data->npwin->height);
    window_info.SetAsChild((HWND)data->npwin->window, rect);
    CefBrowserHost::CreateBrowser(window_info, client, url.Get(),
                                  CefBrowserSettings(), nullptr,
                                  nullptr);

    data->message = L"Launched SubWebView";
    
    /*
    // prevent overlong URLs from making LaunchProcess fail
    if (str::Len(url) > 4096)
        url.Set(NULL);

    ScopedMem<WCHAR> cmdLine(str::Format(L"\"%s\" --npwebview-startup-url=\"%s\" --npwebview-parent-hwnd=%d",
        data->exepath, url, (HWND)data->npwin->window));
    data->hProcess = LaunchProcess(cmdLine);
    if (!data->hProcess)
    {
        data->message = L"Error: Couldn't run SubWebView!";
    }
    */
}

LRESULT CALLBACK PluginWndProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    NPP instance = (NPP)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    
    if (uiMsg == WM_PAINT)
    {
        InstanceData *data = (InstanceData *)instance->pdata;
        
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hWnd, &ps);
        HBRUSH brushBg = CreateSolidBrush(COL_WINDOW_BG);
        HFONT hFont = CreateSimpleFont(hDC, L"MS Shell Dlg", 14);
        
        // set up double buffering
        RectI rcClient = ClientRect(hWnd);
        DoubleBuffer buffer(hWnd, rcClient);
        HDC hDCBuffer = buffer.GetDC();
        
        // display message centered in the window
        RECT rectClient = rcClient.ToRECT();
        FillRect(hDCBuffer, &rectClient, brushBg);
        hFont = (HFONT)SelectObject(hDCBuffer, hFont);
        SetTextColor(hDCBuffer, RGB(0, 0, 0));
        SetBkMode(hDCBuffer, TRANSPARENT);
        DrawCenteredText(hDCBuffer, rcClient, data->message, FALSE);
        
        // draw the buffer on screen
        buffer.Flush(hDC);
        
        DeleteObject(SelectObject(hDCBuffer, hFont));
        DeleteObject(brushBg);
        EndPaint(hWnd, &ps);
        
        HWND hChild = FindWindowEx(hWnd, NULL, NULL, NULL);
        if (hChild)
            InvalidateRect(hChild, NULL, FALSE);
    }
    else if (uiMsg == WM_SIZE)
    {
        HWND hChild = FindWindowEx(hWnd, NULL, NULL, NULL);
        if (hChild)
        {
            ClientRect rcClient(hWnd);
            MoveWindow(hChild, rcClient.x, rcClient.y, rcClient.dx, rcClient.dy, FALSE);
        }
    }
    else if (uiMsg == WM_COPYDATA)
    {
        COPYDATASTRUCT *cds = (COPYDATASTRUCT *)lParam;
        if (cds && 0x4C5255 /* URL */ == cds->dwData)
        {
            gNPNFuncs.geturl(instance, (const char *)cds->lpData, "_blank");
            return TRUE;
        }
    }
    else if (uiMsg == WM_CEF_INVOKE_POPUP) {
        auto target_url_stdstr = (std::string*)lParam;
        NPObject* window = NULL;
        gNPNFuncs.getvalue(instance, NPNVWindowNPObject, &window);
        NPIdentifier openId = gNPNFuncs.getstringidentifier("open");
        NPVariant args[1];
        NPString args_uri;
        args_uri.utf8characters = target_url_stdstr->c_str();
        args_uri.utf8length = (uint32_t)target_url_stdstr->length();
        args[0].type = NPVariantType_String;
        args[0].value.stringValue = args_uri;
        NPVariant result;
        gNPNFuncs.invoke(instance, window, openId, args, ARRAYSIZE(args), &result);
        delete target_url_stdstr;
        return TRUE;
    }
    else if (uiMsg == WM_CEF_SET_TITLE) {
        auto title_std = (std::string*)lParam;
        NPObject* window = NULL;
        gNPNFuncs.getvalue(instance, NPNVWindowNPObject, &window);
        NPIdentifier docId = gNPNFuncs.getstringidentifier("document");
        NPVariant documentVar;
        gNPNFuncs.getproperty(instance, window, docId, &documentVar);
        NPObject* document = NPVARIANT_TO_OBJECT(documentVar);
        NPIdentifier titleId = gNPNFuncs.getstringidentifier("title");
        NPString title;
        title.utf8characters = title_std->c_str();
        title.utf8length = (uint32_t)title_std->length();
        NPVariant titleVar;
        titleVar.type = NPVariantType_String;
        titleVar.value.stringValue = title;
        gNPNFuncs.setproperty(instance, document, titleId, &titleVar);
        delete title_std;
        return TRUE;
    }
    
    return DefWindowProc(hWnd, uiMsg, wParam, lParam);
}

/* ::::: Plugin Methods ::::: */

NPError NP_LOADDS NPP_New(NPMIMEType pluginType, NPP instance, uint16_t mode, int16_t argc, char* argn[], char* argv[], NPSavedData* saved)
{
    if (!instance)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    instance->pdata = AllocStruct<InstanceData>();
    if (!instance->pdata)
    {
        return NPERR_OUT_OF_MEMORY_ERROR;
    }

    gNPNFuncs.setvalue(instance, NPPVpluginWindowBool, (void *)true);
    
    InstanceData *data = (InstanceData *)instance->pdata;
    data->message = L"Opening SubWebView...";
    
    return NPERR_NO_ERROR;
}

NPError NP_LOADDS NPP_SetWindow(NPP instance, NPWindow *npwin)
{
    if (!instance)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    InstanceData *data = (InstanceData *)instance->pdata;
    if (!npwin)
    {
        data->npwin = NULL;
    }
    else if (data->npwin != npwin)
    {
        HWND hWnd = (HWND)npwin->window;
        
        data->npwin = npwin;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)instance);
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)PluginWndProc);
    }
    else
    {
        // The plugin's window hasn't changed, just its size
        HWND hWnd = (HWND)npwin->window;
        HWND hChild = FindWindowEx(hWnd, NULL, NULL, NULL);
        
        if (hChild)
        {
            ClientRect rcClient(hWnd);
            MoveWindow(hChild, rcClient.x, rcClient.y, rcClient.dx, rcClient.dy, FALSE);
        }
    }
    
    return NPERR_NO_ERROR;
}

NPError NP_LOADDS NPP_NewStream(NPP instance, NPMIMEType type, NPStream* stream, NPBool seekable, uint16_t* stype)
{
    InstanceData *data = (InstanceData *)instance->pdata;
    LaunchSubWebView(data, stream->url, instance);
    gNPNFuncs.destroystream(instance, stream, NPRES_DONE);
    return NPERR_NO_ERROR;
}

int32_t NP_LOADDS NPP_WriteReady(NPP instance, NPStream* stream)
{
    return stream->end > 0 ? stream->end : INT_MAX;
}

int32_t NP_LOADDS NPP_Write(NPP instance, NPStream* stream, int32_t offset, int32_t len, void* buffer)
{
    gNPNFuncs.destroystream(instance, stream, NPRES_DONE);
    return 0;
}

void NP_LOADDS NPP_StreamAsFile(NPP instance, NPStream* stream, const char* fname)
{
}

NPError NP_LOADDS NPP_DestroyStream(NPP instance, NPStream* stream, NPReason reason)
{
    return NPERR_NO_ERROR;
}

NPError NP_LOADDS NPP_Destroy(NPP instance, NPSavedData** save)
{
    if (!instance)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    InstanceData *data = (InstanceData *)instance->pdata;

    free(data);
    
    return NPERR_NO_ERROR;
}

// Note: NPP_Print is never called by Google Chrome or Firefox 4
//       cf. http://code.google.com/p/chromium/issues/detail?id=83341
//       and https://bugzilla.mozilla.org/show_bug.cgi?id=638796

#define IDM_PRINT 403

void NP_LOADDS NPP_Print(NPP instance, NPPrint* platformPrint)
{
    if (!platformPrint)
    {
        return;
    }

    if (NP_FULL == platformPrint->mode)
    {
        InstanceData *data = (InstanceData *)instance->pdata;
        HWND hWnd = (HWND)data->npwin->window;
        HWND hChild = FindWindowEx(hWnd, NULL, NULL, NULL);
        
        if (hChild)
        {
            PostMessage(hChild, WM_COMMAND, IDM_PRINT, 0);
            platformPrint->print.fullPrint.pluginPrinted = true;
        }
    }
}
