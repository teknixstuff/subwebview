// Copyright (c) 2017 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "examples/npWebView/client_minimal.h"
#include "appfactory.h"
#include "examples/shared/browser_util.h"
#include "network_http.h"

namespace minimal {

// Minimal implementation of CefApp for the browser process.
class BrowserApp : public CefApp, public CefBrowserProcessHandler {
 public:
  BrowserApp(NPNetscapeFuncs pNPNFuncs, NPP npp) {
    this->npp = npp;
    this->pNPNFuncs = pNPNFuncs;
  }

  // CefApp methods:
  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
    return this;
  }

  void OnBeforeCommandLineProcessing(
      const CefString& process_type,
      CefRefPtr<CefCommandLine> command_line) override {
    // Command-line flags can be modified in this callback.
    // |process_type| is empty for the browser process.
    if (process_type.empty()) {
      command_line->AppendSwitch("single-process");
#if defined(OS_MACOSX)
      // Disable the macOS keychain prompt. Cookies will not be encrypted.
      command_line->AppendSwitch("use-mock-keychain");
#endif
    }
  }

  // CefBrowserProcessHandler methods:
  void OnContextInitialized() override {
    CefRefPtr<CefRequestContext> context =
        CefRequestContext::GetGlobalContext();

    if (context) {
      context->RegisterSchemeHandlerFactory("http", "", new MyHttpSchemeHandlerFactory(pNPNFuncs, npp));
      context->RegisterSchemeHandlerFactory("https", "", new MyHttpSchemeHandlerFactory(pNPNFuncs, npp));
    }
  }

 private:
  NPP npp;
  NPNetscapeFuncs pNPNFuncs;
  IMPLEMENT_REFCOUNTING(BrowserApp);
  DISALLOW_COPY_AND_ASSIGN(BrowserApp);
};

}  // namespace minimal

namespace shared {

CefRefPtr<CefApp> CreateBrowserProcessApp(NPNetscapeFuncs pNPNFuncs, NPP npp) {
  return new minimal::BrowserApp(pNPNFuncs, npp);
}

}  // namespace shared