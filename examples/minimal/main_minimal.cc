// Copyright (c) 2017 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "examples/shared/main.h"
#include "examples/shared/browser_util.h"
#include "examples/minimal/client_minimal.h"
#include "include/cef_sandbox_win.h"
#include "examples/shared/app_factory.h"
#include "examples/shared/client_manager.h"
#include "examples/shared/main_util.h"

// Main program entry point function.
#if defined(OS_WIN)
int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPTSTR lpCmdLine,
                      int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);
  UNREFERENCED_PARAMETER(nCmdShow);
  return shared::wWinMain(hInstance);
}
#else
int main(int argc, char* argv[]) {
  return shared::main(argc, argv);
}
#endif

DWORD WINAPI ExecWebView(LPVOID) {
  // Run the CEF message loop. This will block until CefQuitMessageLoop() is
  // called.
  CefRunMessageLoop();

  // Shut down CEF.
  CefShutdown();

  return 0;
}

HANDLE hWebViewThread;

void UninitWebView() {
  CefQuitMessageLoop();
  WaitForSingleObject(hWebViewThread, INFINITE);
}

BOOL InitWebView() {
  void* sandbox_info = nullptr;

#if defined(CEF_USE_SANDBOX)
  // Manage the life span of the sandbox information object. This is necessary
  // for sandbox support on Windows. See cef_sandbox_win.h for complete details.
  CefScopedSandboxInfo scoped_sandbox;
  sandbox_info = scoped_sandbox.sandbox_info();
#endif

  // Provide CEF with command-line arguments.
  CefMainArgs main_args;

  // Create a temporary CommandLine object.
  CefRefPtr<CefCommandLine> command_line = shared::CreateCommandLine(main_args);

  // Create a CefApp of the correct process type.
  CefRefPtr<CefApp> app = shared::CreateBrowserProcessApp();

  // Create the singleton manager instance.
  shared::ClientManager manager;

  // Specify CEF global settings here.
  CefSettings settings;

#if !defined(CEF_USE_SANDBOX)
  settings.no_sandbox = true;
#endif

  // Initialize the CEF browser process. The first browser instance will be
  // created in CefBrowserProcessHandler::OnContextInitialized() after CEF has
  // been initialized. May return false if initialization fails or if early exit
  // is desired (for example, due to process singleton relaunch behavior).
  if (!CefInitialize(main_args, settings, app, sandbox_info)) {
    return FALSE;
  }

  hWebViewThread = CreateThread(NULL, 0, ExecWebView, NULL, 0, NULL);
  return TRUE;
}

void CreateWebView(LPCWSTR kStartupURL, HWND kParentHWND) {
    // Create the browser window.
    CefWindowInfo window_info;
    window_info.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
    RECT wrect;
    GetWindowRect(kParentHWND, &wrect);
    CefRect crect;
    crect.Set(0, 0, wrect.right - wrect.left, wrect.bottom - wrect.top);
    window_info.SetAsChild(kParentHWND, crect);
    CefBrowserHost::CreateBrowser(window_info, new minimal::Client(), kStartupURL, CefBrowserSettings(), nullptr, nullptr);
}
