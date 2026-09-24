# Copyright (c) 2024 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

#
# THIS FILE IS AUTO-GENERATED. DO NOT EDIT BY HAND.
#
# Use the following command to update version information:
# % python3 ./tools/bazel/version_updater.py --version=<version> [--channel=<channel>] [--url=<url>]
#
# Specify a fully-qualified CEF version. Optionally override the default
# download URL.
#
# CEF binary distribution file names are expected to take the form
# "cef_binary_<version>_<platform>[_<channel>].tar.bz2". These files must
# exist for each supported platform at the download URL location. Sha256
# hashes must also exist for each file at "<file_name>.sha256".
#

CEF_DOWNLOAD_URL = "https://cef-builds.spotifycdn.com/"

CEF_VERSION = "144.0.6+g5f7e671+chromium-144.0.7559.59"
CEF_CHANNEL = ""

# Map of supported platform to sha256 for binary distribution download.
CEF_FILE_SHA256 = {
    "windows32": "c8789cf1c4b2e02899102b84312076995850f3eccedd7ed96f0e2858d44dc67f",
    "windows64": "11e5b63568a743e2e6611787ec8efc461d8eb59d6910e6cab5820402e0a92ecf",
    "windowsarm64": "97d25b723ea1e56484c7e19acd2926f09b7e07ffa46a499d0ada7374fd19d2e7",
    "macosx64": "14035345739b9429d9f1d3c3c5083db36513ba8c94b78e767270f4228c36ad7d",
    "macosarm64": "a3086919822668d571161b619f93babe10caa20f97cafbca09bcbc02a1c0ec01",
    "linux64": "edacce023b2801b477871f0da47a572201050571ceb154f943651419703c6ef0",
    "linuxarm64": "92f05f0090df0c67119b34ecfd0e3e8115158e4c0a763116b72034cfb9d7b045",
    "linuxarm": "6977fe8be94be9039f89ca62465a5183a7b5355f98499618a894f7465a62c286",
}
