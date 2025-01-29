// Copyright (c) 2017 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "examples/minimal/client_minimal.h"
#include "include/internal/cef_types.h"
#include "examples/shared/client_util.h"
#define WM_CEF_INVOKE_POPUP (WM_USER + 0x0001)
#define WM_CEF_SET_TITLE (WM_USER + 0x0002)

namespace minimal {

    Client::Client() {}

    void Client::OnTitleChange(CefRefPtr<CefBrowser> browser,
                               const CefString& title) {
      // Call the default shared implementation.
      // shared::OnTitleChange(browser, title);
      // we're a plugin now, don't do that
      SendMessage(hPluginWnd, WM_CEF_SET_TITLE, NULL, (LPARAM) new std::string(title));
    }

    void Client::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
      // Call the default shared implementation.
      shared::OnAfterCreated(browser);
    }

    bool Client::DoClose(CefRefPtr<CefBrowser> browser) {
      // Call the default shared implementation.
      return shared::DoClose(browser);
    }

    void Client::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
      // Call the default shared implementation.
      return shared::OnBeforeClose(browser);
    }

    bool Client::OnBeforePopup(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        const CefString& target_url,
        const CefString& target_frame_name,
        WindowOpenDisposition target_disposition,
        bool user_gesture,
        const CefPopupFeatures& popupFeatures,
        CefWindowInfo& windowInfo,
        CefRefPtr<CefClient>& client,
        CefBrowserSettings& settings,
        CefRefPtr<CefDictionaryValue>& extra_info,
        bool* no_javascript_access)
    {
        SendMessage(hPluginWnd, WM_CEF_INVOKE_POPUP, NULL, (LPARAM)new std::string(target_url));
        return true;
    }

}  // namespace minimal
