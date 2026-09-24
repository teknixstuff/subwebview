# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a CEF (Chromium Embedded Framework) project that demonstrates how to build applications using CEF. The project provides example applications showing different aspects of CEF functionality and serves as a starting point for third-party applications.

## Build Systems

This project supports two build systems:

### CMake Build
```bash
# Create build directory
mkdir build && cd build

# Configure for your platform
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..  # Linux
cmake -G "Xcode" -DPROJECT_ARCH="arm64" ..               # macOS ARM64
cmake -G "Xcode" -DPROJECT_ARCH="x86_64" ..              # macOS x86_64
cmake -G "Visual Studio 17" -A x64 ..                   # Windows 64-bit

# Build
make -j4                    # Linux
# Use Xcode for macOS builds
# Use Visual Studio for Windows builds
```

### Bazel Build (Experimental)
Note: Bazel support is considered experimental. See https://github.com/chromiumembedded/cef/issues/3757 for status.

```bash
# Linux only: Install patchelf first
sudo apt install patchelf
# Configure platform (required before first build)
python3 ./tools/bazel/platform_updater.py

# Build examples
bazel build //examples/minimal
bazel build //examples/message_router
bazel build //examples/resource_manager
bazel build //examples/scheme_handler

# Run examples (Linux/macOS)
bazel run //examples/minimal

# Run examples (Windows)
bazel run //examples/minimal/win:minimal.exe

# Debug build (add -c dbg flag)
bazel build -c dbg //examples/minimal

# Build CEF sample applications
bazel build @cef//tests/cefsimple
bazel build @cef//tests/cefclient
```

## Development Commands

### Code Formatting
```bash
# Format a directory (recursively)
python3 tools/fix_style.py .

# Format specific files or git changes
python3 tools/fix_style.py unstaged   # Format unstaged changes
python3 tools/fix_style.py staged     # Format staged changes
python3 tools/fix_style.py <file>     # Format specific file
python3 tools/fix_style.py <git-hash> # Format files changed in a commit
```

### Version Management
```bash
# Update CEF version
python3 ./tools/bazel/version_updater.py --version=<version>

# Update platform configuration
python3 ./tools/bazel/platform_updater.py --arch=<arch>
```

## Architecture

### Core Components

1. **CEF Binary Distribution** (`third_party/cef/`): Contains the CEF framework and libraries
2. **Examples** (`examples/`): Demonstration applications showing CEF features
3. **Shared Library** (`examples/shared/`): Common functionality used by all examples
4. **Build Configuration**: CMake and Bazel files for cross-platform building

### Example Applications

- **minimal**: Demonstrates the minimal functionality required for a CEF application
- **message_router**: Shows JavaScript bindings using CefMessageRouter
- **resource_manager**: Demonstrates resource handling with CefResourceManager  
- **scheme_handler**: Shows custom scheme handling with CefSchemeHandlerFactory

### CEF Integration Pattern

Each example follows this pattern:
1. Implements entry point functions from `examples/shared/main.h`
2. Creates process-specific CefApp instances via `app_factory.h`
3. Provides a CefClient implementation for browser callbacks
4. Uses shared utilities from `examples/shared/` for common operations

## Key Files

- `CMakeLists.txt`: Main CMake configuration, sets CEF version
- `bazel/cef/version.bzl`: Bazel CEF version configuration
- `bazel/cef/platform.bzl`: Platform-specific Bazel configuration
- `.bazelversion`: Specifies required Bazel version (7.5.0)
- `examples/shared/`: Core shared library with browser utilities
- `tools/fix_style.py`: Code formatting script using clang-format and yapf

## Platform Support

- **Linux**: Debian 10+, Ubuntu 18+, GCC 7.5.0+ (Ubuntu 22.04 recommended)
- **macOS**: Xcode 13.5-16.4, macOS 12.0+ (supports x86_64 and ARM64)
- **Windows**: Visual Studio 2022, Windows 10/11 (supports x86, x64, ARM64)

## Dependencies

- Python 3.9-3.11 (required for build scripts)
- CMake 3.21+ (for CMake builds)
- Bazel 7.5.0 (for Bazel builds)
- Platform-specific toolchains (see README.md for details)