// Copyright (c) 2017 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_EXAMPLES_SCHEME_HANDLER_SCHEME_STRINGS_H_
#define CEF_EXAMPLES_SCHEME_HANDLER_SCHEME_STRINGS_H_

#include "include/internal/cef_types.h"

namespace scheme_handler {

inline constexpr char kScheme[] = "client";
inline constexpr char kDomain[] = "tests";
inline constexpr char kFileName[] = "scheme_handler.html";

// Used to register a custom scheme as standard and secure.
inline constexpr int kSchemeRegistrationOptions =
    CEF_SCHEME_OPTION_STANDARD | CEF_SCHEME_OPTION_SECURE |
    CEF_SCHEME_OPTION_CORS_ENABLED | CEF_SCHEME_OPTION_FETCH_ENABLED;

}  // namespace scheme_handler

#endif  // CEF_EXAMPLES_SCHEME_HANDLER_SCHEME_STRINGS_H_
