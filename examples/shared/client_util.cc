// Copyright (c) 2017 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "examples/shared/client_util.h"

#include <sstream>
#include <string>

#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"

#include "examples/shared/client_manager.h"

namespace shared {

void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
  CEF_REQUIRE_UI_THREAD();

  auto browser_view = CefBrowserView::GetForBrowser(browser);
  if (browser_view) {
    // Set the title of the window using the Views framework.
    auto window = browser_view->GetWindow();
    if (window)
      window->SetTitle(title);
  } else {
    // Set the title of the window using platform APIs.
    PlatformTitleChange(browser, title);
  }
}

void OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  // Add to the list of existing browsers.
  ClientManager::GetInstance()->OnAfterCreated(browser);
}

bool DoClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  // Closing the main window requires special handling. See the DoClose()
  // documentation in the CEF header for a detailed destription of this
  // process.
  ClientManager::GetInstance()->DoClose(browser);

  // Allow the close. For windowed browsers this will result in the OS close
  // event being sent.
  return false;
}

void OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  // Remove from the list of existing browsers.
  ClientManager::GetInstance()->OnBeforeClose(browser);
}

std::string DumpRequestContents(CefRefPtr<CefRequest> request) {
  std::stringstream ss;

  ss << "URL: " << std::string(request->GetURL());
  ss << "\nMethod: " << std::string(request->GetMethod());

  CefRequest::HeaderMap headerMap;
  request->GetHeaderMap(headerMap);
  if (headerMap.size() > 0) {
    ss << "\nHeaders:";
    for (const auto& [key, value] : headerMap) {
      ss << "\n\t" << std::string(key) << ": " << std::string(value);
    }
  }

  auto postData = request->GetPostData();
  if (postData.get()) {
    CefPostData::ElementVector elements;
    postData->GetElements(elements);
    if (elements.size() > 0) {
      ss << "\nPost Data:";
      for (const auto& element : elements) {
        if (element->GetType() == PDE_TYPE_BYTES) {
          // the element is composed of bytes
          ss << "\n\tBytes: ";
          if (element->GetBytesCount() == 0) {
            ss << "(empty)";
          } else {
            // retrieve the data.
            auto size = element->GetBytesCount();
            std::vector<char> bytes(size);
            element->GetBytes(size, bytes.data());
            ss << std::string(bytes.data(), size);
          }
        } else if (element->GetType() == PDE_TYPE_FILE) {
          ss << "\n\tFile: " << std::string(element->GetFile());
        }
      }
    }
  }

  return ss.str();
}

bool IsViewsEnabled() {
  static bool enabled = []() {
    // Views is enabled by default, unless `--use-native` is specified.
    return !CefCommandLine::GetGlobalCommandLine()->HasSwitch("use-native");
  }();
  return enabled;
}

bool IsAlloyStyleEnabled() {
  static bool enabled = []() {
    // Chrome style is enabled by default, unless `--use-alloy-style` is
    // specified.
    return CefCommandLine::GetGlobalCommandLine()->HasSwitch("use-alloy-style");
  }();
  return enabled;
}

}  // namespace shared
