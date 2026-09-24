# Resource Manager Example Application

This directory contains the "resource_manager" target which demonstrates how to handle resource requests using [CefResourceManager](https://github.com/chromiumembedded/cef/blob/master/include/wrapper/cef_resource_manager.h).

See the [shared library](../shared) target for details common to all executable targets.

## Implementation Overview

The "resource_manager" target is implemented as follows:

 * Define the target-specific [CMake](https://cmake.org/) build configuration in the [CMakeLists.txt](CMakeLists.txt) file.
 * Call the shared [entry point functions](https://chromiumembedded.github.io/cef/general_usage#entry-point-function) that initialize, run and shut down CEF.
     * Uses the [minimal target](../minimal) implementation.
 * Implement the [shared::Create*ProcessApp](../shared/app_factory.h) functions to create a [CefApp](https://chromiumembedded.github.io/cef/general_usage#cefapp) instance appropriate to the [process type](https://chromiumembedded.github.io/cef/general_usage#processes).
     * Browser process: [app_browser_impl.cc](app_browser_impl.cc) implements the `shared::CreateBrowserProcessApp` method to return a `CefApp` instance.
     * Other processes: Uses the [minimal target](../minimal) implementation.
 * Provide a concrete [CefClient](https://chromiumembedded.github.io/cef/general_usage#cefclient) implementation in [client_impl.cc](client_impl.cc) and [client_impl.h](client_impl.h) to handle [CefBrowser](https://chromiumembedded.github.io/cef/general_usage#cefbrowser-and-cefframe) callbacks.
      * Creates a `CefResourceManager` instance to handle resource requests.
      * Defines a `RequestDumpResourceProvider` class to demonstrate custom `CefResourceManager::Provider` handling.
      * Registers the `CefResourceManager::Provider` instances with the `CefResourceManager`.
 * Windows resource loading implementation in [resource_util_win_impl.cc](resource_util_win_impl.cc) and [resource.rc](win/resource.rc).
     * Implements the [shared::GetResourceId](../shared/resource_util.h) method to map resource paths to BINARY ID values.
     * Defines a BINARY resource to include [logo.png](resources/logo.png) and [resource_manager.html](resources/resource_manager.html) in the executable.

## Configuration

See the [shared library](../shared) target for configuration details.

## Setup and Build

See the [shared library](../shared) target for setup and build instructions.
