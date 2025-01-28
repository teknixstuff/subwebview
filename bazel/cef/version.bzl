# Copyright (c) 2024 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

#
# THIS FILE IS AUTO-GENERATED. DO NOT EDIT BY HAND.
#
# Use the following command to update version information:
# % python3 ./tools/bazel/version_updater.py --version=<version> [--url=<url>]
#
# Specify a fully-qualified CEF version. Optionally override the default
# download URL.
#
# CEF binary distribution file names are expected to take the form
# "cef_binary_<version>_<platform>.tar.bz2". These files must exist for each
# supported platform at the download URL location. Sha256 hashes must also
# exist for each file at "<file_name>.sha256".
#

CEF_DOWNLOAD_URL = "https://cef-builds.spotifycdn.com/"

CEF_VERSION = "130.1.14+gde32089+chromium-130.0.6723.92"

# Map of supported platform to sha256 for binary distribution download.
CEF_FILE_SHA256 = {
    "windows32": "4516a867e2bf9ec9ad483aa0dfbd34f82562cf8bda2852f25f814a0c6b4bd4d7",
    "windows64": "b79f626005b15b0f4d74ee77f7f9d3e753a2b0c9582a038937ad1c86838c8a4a",
    "windowsarm64": "6ee4f73b358e36704f8666558572bb8768e9f5bf6dda14a5197c1937619d44c8",
    "macosx64": "fe3dd09fb3aafdc497f9276e63576c0ac35a6ab22a4c6c3e72778657b128298c",
    "macosarm64": "5da447cb72e9d51fc316ae7275788ef15515625b14a8f99633a997ec558f3a19",
    "linux64": "07601c40026df62bf6b74a201e714db6abf0d1cc2b576a86a681b28cdbb79365",
    "linuxarm64": "a14bb94fe3f0fbbd67496a0f8a6306261978e5ddd9220f56ad7585c8f59f3f8d",
    "linuxarm": "435d105af6208ade4b2d4b2d7fcf252f9cf93b96000f627c06d78d26df6c6f3e",
}
