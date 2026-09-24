# Copyright (c) 2017 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

#
# Source files.
#

# Main executable sources.
set(EXAMPLE_SRCS
  app_browser_minimal.cc
  client_minimal.cc
  client_minimal.h
  npWebView.cc
  )
set(EXAMPLE_SRCS_WINDOWS
  resource.h
  )
APPEND_PLATFORM_SOURCES(EXAMPLE_SRCS)
  
if(OS_LINUX OR OS_WINDOWS)
  # On Windows and Linux the same executable is used for all processes.
  set(EXAMPLE_SRCS
    ${EXAMPLE_SRCS}
    app_other_minimal.cc
    app_renderer_minimal.cc
    )
elseif(OS_MAC)
  # On macOS a separate helper executable is used for subprocesses.
  set(EXAMPLE_HELPER_SRCS
    app_other_minimal.cc
    app_renderer_minimal.cc
    process_helper_mac_minimal.cc
    )
endif()

# Main executable resources.
set(EXAMPLE_RESOURCES_SRCS
  )
set(EXAMPLE_RESOURCES_SRCS_WINDOWS
  # Resources that embed the *.html and *.png files in the executable.
  npWebView.rc
  )
APPEND_PLATFORM_SOURCES(EXAMPLE_RESOURCES_SRCS)

if(OS_MACOSX OR OS_WINDOWS)
  # On macOS and Windows include the shared resources.
  set(EXAMPLE_RESOURCES_SRCS
    ${EXAMPLE_RESOURCES_SRCS}
    ${SHARED_RESOURCES_SRCS}
    )
endif()


#
# Shared configuration.
#

# Target executable names.
set(EXAMPLE_TARGET "npWebView")

#
# Windows configuration.
#

if(OS_WINDOWS)
    # Add resources to the sources variable for convenience.
  set(EXAMPLE_SRCS
    ${EXAMPLE_SRCS}
    ${EXAMPLE_RESOURCES_SRCS}
    )

  # Create source groups for Visual Studio.
  SET_EXAMPLE_SOURCE_GROUPS("${EXAMPLE_SRCS}")

  if(USE_SANDBOX)
    # Shared library (DLL) target.
    add_library(${EXAMPLE_TARGET} SHARED ${EXAMPLE_SRCS})
    SET_EXAMPLE_LIBRARY_TARGET_PROPERTIES(${EXAMPLE_TARGET})

    # Copy and rename the bootstrap binary.
    COPY_SINGLE_FILE(${EXAMPLE_TARGET} ${CEF_BINARY_DIR}/bootstrap.exe ${EXAMPLE_TARGET_OUT_DIR}/${EXAMPLE_TARGET}.exe)
    if (CMAKE_GENERATOR MATCHES "Visual Studio")
      # Make the bootstrap binary the default command for the Visual Studio debugger.
      set_target_properties(${EXAMPLE_TARGET} PROPERTIES
        VS_DEBUGGER_COMMAND "$<TARGET_FILE_DIR:${EXAMPLE_TARGET}>/${EXAMPLE_TARGET}.exe"
      )
    endif()
  else()
    # Executable target (sandbox disabled).
    add_library(${EXAMPLE_TARGET} SHARED ${EXAMPLE_SRCS})
    SET_EXAMPLE_DLL_TARGET_PROPERTIES(${EXAMPLE_TARGET})
  endif()

  add_dependencies(${EXAMPLE_TARGET} shared)
  target_link_libraries(${EXAMPLE_TARGET} shared)
endif()
