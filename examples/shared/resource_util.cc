// Copyright (c) 2017 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "examples/shared/resource_util.h"

#include "include/cef_parser.h"
#include "include/wrapper/cef_stream_resource_handler.h"

namespace shared {

namespace {

// Returns |url| without the query or fragment components, if any.
std::string GetUrlWithoutQueryOrFragment(std::string_view url) {
  // Find the first instance of '?' or '#'.
  const size_t pos = std::min(url.find('?'), url.find('#'));
  if (pos != std::string::npos)
    return std::string(url.substr(0, pos));

  return std::string(url);
}

}  // namespace

std::optional<std::string> GetResourcePath(std::string_view url) {
  if (url.find(kTestOrigin) != 0U)
    return std::nullopt;

  const std::string url_no_query = GetUrlWithoutQueryOrFragment(url);
  return url_no_query.substr(sizeof(kTestOrigin) - 1);
}

// Determine the mime type based on the |file_path| file extension.
std::string GetMimeType(std::string_view resource_path) {
  std::string mime_type;
  size_t sep = resource_path.find_last_of(".");
  if (sep != std::string::npos) {
    mime_type = CefGetMimeType(std::string(resource_path.substr(sep + 1)));
    if (!mime_type.empty())
      return mime_type;
  }
  return "text/html";
}

CefRefPtr<CefResourceHandler> GetResourceHandler(
    std::string_view resource_path) {
  auto reader = GetResourceReader(resource_path);
  if (!reader)
    return nullptr;

  return new CefStreamResourceHandler(GetMimeType(resource_path), reader);
}

}  // namespace shared
