# Copyright (c) 2017 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

#
# Source files.
#

# Main executable sources.
set(EXAMPLE_SRCS
  webhost.cc
  npWebViewIE.cc
  )
set(EXAMPLE_SRCS_WINDOWS
  resource.h
  )

# Main executable resources.
set(EXAMPLE_RESOURCES_SRCS
  )
set(EXAMPLE_RESOURCES_SRCS_WINDOWS
  # Resources that embed the *.html and *.png files in the executable.
  npWebViewIE.rc
  )
APPEND_PLATFORM_SOURCES(EXAMPLE_RESOURCES_SRCS)


#
# Shared configuration.
#

# Target executable names.
set(EXAMPLE_TARGET "npWebViewIE")

if(OS_WINDOWS)
    # Add resources to the sources variable for convenience.
  set(EXAMPLE_SRCS
    ${EXAMPLE_SRCS}
    ${EXAMPLE_RESOURCES_SRCS}
    )

  # Create source groups for Visual Studio.
  SET_EXAMPLE_SOURCE_GROUPS("${EXAMPLE_SRCS}")

  # Executable target.
  add_library(${EXAMPLE_TARGET} SHARED ${EXAMPLE_SRCS})
  SET_EXAMPLE_DLL_TARGET_PROPERTIES(${EXAMPLE_TARGET})
endif()
