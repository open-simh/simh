<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-refresh-toc -->
**Table of Contents**

- [CMake Maintainer Walk-through](#cmake-maintainer-walk-through)
  - [Introduction](#introduction)
  - [*CMake* Basics](#cmake-basics)
    - [`CMAKE_SOURCE_DIR` and `CMAKE_BINARY_DIR`](#cmake_source_dir-and-cmake_binary_dir)
    - [Top-level `CMakeLists.txt`](#top-level-cmakeliststxt)
    - [Build Configurations](#build-configurations)
  - [Code Roadmap](#code-roadmap)
  - [`${CMAKE_SOURCE_DIR}/CMakeLists.txt`](#cmake_source_dircmakeliststxt)
  - [The `cmake` sub-directory](#the-cmake-sub-directory)
    - [`add_simulator.cmake`](#add_simulatorcmake)
    - [`find_package` support](#find_package-support)
    - [`platform-quirks.cmake`: Platform-specific settings/tweaks](#platform-quirkscmake-platform-specific-settingstweaks)
    - [`vcpkg.cmake`: vcpkg package installer/maintainer](#vcpkgcmake-vcpkg-package-installermaintainer)
    - [`dep-locate.cmake`, `dep-link.cmake`: Dependency libraries](#dep-locatecmake-dep-linkcmake-dependency-libraries)
    - [`simh-simulators.cmake`](#simh-simulatorscmake)
    - [`generate.py`: Automagic `makefile` → CMake infrastructure](#generatepy-automagic-makefile--cmake-infrastructure)
      - [`generate.py` Internals](#generatepy-internals)
      - [Using `generate.py` outside of *open-simh*](#using-generatepy-outside-of-open-simh)
    - [CPack configuration](#cpack-configuration)

<!-- markdown-toc end -->

# CMake Maintainer Walk-through

## Introduction

This code walk-through is primarily intended for maintainers and developers interested in the
[CMake][cmake] build and packaging environment internals.

The main idea is to make adding and compiling simulators easy for the simulator developers through a
single *CMake* function, `add_simulator`. All a simulator developer should have to do is create a
simulator `CMakeLists.txt` file, invokes the `add_simulator` function and update the
`cmake/simh-simulators.cmake` file to include the new simulator sub-directory.  That's it.

The remainder of this document explains the *CMake* infrastructure machinery that supports the
`add_simulator` function.

## *CMake* Basics

If you are not familiar with *CMake*, *CMake* is a meta-build platform that supports a wide cross section
of operating systems and build systems. Typical *CMake*-based development has four phases:

- _Configure and generate_: Search for headers, dependency libraries, adjust compiler flags, add defines
  and include paths, set up build targets (static, dynamic libraries and executables), setup test case
  support and packaging, then generate the build system's input (e.g., a `makefile` for make, solution
  hierarchy for Visual Studio, `build.ninja` for Ninja, project files for Mac OS X XCode, etc.)

- _Build_: Invoke the build system (`make`, `ninja`, `msbuild`, ...) *CMake*'s build mode is a wrapper
  around the underlying build system tool. It's perfectly acceptable to change to the build directory and
  invoke the build tool directly.

- _Test_: Execute the test cases using the CTest driver and collect test status (success, failure, failure
  output logs.)

- _Package_: Create installer packages using the CPack utility.

### `CMAKE_SOURCE_DIR` and `CMAKE_BINARY_DIR`

*CMake* has two variables that identify the top-level source directory, `CMAKE_SOURCE_DIR`, and the build
artifact directory, `CMAKE_BINARY_DIR`. These two directories should be separate from each other -- you
should not try to create build artifacts in the top-level source directory (this is enforced by the
top-level `CMakeLists.txt` code.)

### Top-level `CMakeLists.txt`

The top-level `CMakeLists.txt` file, located in `CMAKE_SOURCE_DIR`, drives the configure/generate
phase. Each simulator sub-directory has its own `CMakeLists.txt` file included by top-level
`CMakeLists.txt`. These per-simulator `CMakeLists.txt` files define one or more simulator executables
along with each executable's simulator-specific defines, include paths and needed SIMH features (i.e.,
video support, 64-bit integer, 64-bit address support.)

### Build Configurations

*CMake* understands multi-configuration build system tools, notably _msbuild_ and _XCode_, versus single
configuration build system tools, such as _make_ and _ninja_.  Multi-configuration build system tools
require an explicit configuration at the build phase, e.g. `--config Release` or `--config Debug` to build
optimized vs. debug simulators. Single configuration build system tools specify `CMAKE_BUILD_TYPE` at the
configure/generate phase. If you need to change the build configuration for a single configuration tool,
you have to reconfigure. Note that if you specify a configuration during the build phase for a single
configuration build system tool, it will be silently ignored.

The SIMH *CMake* infrastructure has a strong preference for the *Release* and *Debug* build
configurations. The *MinSizeRel* and *RelDbgInfo* configurations are weakly supported; their compile flags
are not altered the way that the Release and Debug configurations are customized.

## Code Roadmap

```
open-simh                            This is CMAKE_SOURCE_DIR
+-- .github/workflows
|   + build.yml                      GitHub CI/CD driver for push, pull request
|   |                                builds. Uses cmake-builds.yml for CMake builds.
|   + cmake-builds.yml               Reusable GitHub CI/CD workflow for CMake
|   |                                builds.
|   + release.yml                    GitHub CI/CD driver for packaging and publishing
|                                    releases. Uses cmake-builds.yml to create the
|                                    packaged artifacts (simulators and docs.)
|
+-- CMakeLists.txt                   Top-level CMake configure/generate driver.
+-- cmake                            CMake support code
|   +-- CMake-Walkthrough.md         Documentation. You're reading it.
|   +-- FindEDITLINE.cmake           find_package support for libeditline
|   +-- FindPCAP.cmake               find_package support for libpcap
|   +-- FindPCRE.cmake               find_package support for PCRE
|   +-- FindPCRE2.cmake              find_package support for PCRE2
|   +-- FindPTW.cmake                Platform thread support
|   +-- FindVDE.cmake                find_package support for VDE networking
|   +-- GitHub-release.md            Pre-built binary package install instructions
|   +-- add_simulator.cmake          The add_simulator function, SIMH core libraries
|   +-- build-*                      cmake-builder script output (CMAKE_BINARY_DIR)
|   |                                directories
|   +-- build_dep_matrix.cmake       Release and Debug build scaffolding for
|   |                                external dependency libraries
|   +-- cmake-builder.ps1            PowerShell builder script
|   +-- cmake-builder.sh             Bash builder script
|   +-- cpack-setup.cmake            CPack packaging setup code
|   +-- dep-link.cmake               CMake interface library configuration from
|   |                                previously located dependency libraries
|   +-- dep-locate.cmake             Dependency library location code, e.g., find
|   |                                PCRE, SDL2, SDL2_ttf, ...
|   +-- dependencies                 External dependency library installation
|   |   |                            hierarchy, if building dep. libraries
|   |   +-- Windows-10-MSVC-19.34    (Example 32-bit Windows 10 dependencies)
|   |   +-- Windows-10-MSVC-19.34-64 (Example 64-bit Windows 10 dependencies)
|   +-- file-link-copy.cmake         CMake script to symlink, hard link or copy a
|   |                                file
|   +-- fpintrin.cmake               Unused. Detects SSE, SSE2 and SSE3.
|   +-- generate.py                  Python script that generates the simulator
|   |                                CMakeLists.txt from the makefile
|   +-- git-commit-id.cmake          CMake script to update .git-commit-id and
|   |                                .git-commit-id.h
|   +-- github_v141_xp.ps1           Visual Studio "XP toolkit" installer PowerShell
|   |                                script
|   +-- installer-customizations     Installer-specific customization files
|   +-- os-features.cmake            Operating system feature probes, e.g., -lm
|   +-- patches                      Patches applied to external dependency libs
|   +-- platform-quirks.cmake        Platform quirks: compiler flags, HomeBrew
|   |                                includes, ...
|   +-- pthreads-dep.cmake           Detect platform thread support, configure the
|   |                                thread_lib interface library
|   +-- simgen                       generate.py script support
|   +-- simh-packaging.cmake         Simulator packaging, adds simulators to
|   |                                CPack components (simulator "families")
|   +-- simh-simulators.cmake        Simulator add_subdirectory includes, variable
|   |                                definitions
|   +-- vcpkg-setup.cmake            vcpkg package manager setup code
```

## `${CMAKE_SOURCE_DIR}/CMakeLists.txt`

The top-level `CMakeLists.txt` drives CMake's configure/generate phase, which has the following flow:

- Initial sanity checks
  - Ensure *CMake*'s version is greater or equal to version 3.14. *CMake* will terminate configuration if
    the minimum version is not met.
  - Check `CMAKE_SOURCE_DIR` and `CMAKE_BINARY_DIR`, terminating with an error message if
    `CMAKE_SOURCE_DIR` and `CMAKE_BINARY_DIR` are the same directory.
  - If *CMake*'s version is below 3.21, emit a warning that creating installers will be unsuccessful. This
    doesn't prevent building simulators.  This warning is only emitted once.
  - Emit a fatal error message and terminate if Windows XP compatibility was requested via `-T v141_xp` on
    the command line and the `VCPKG_ROOT` environment variable is set.

- Set the SIMH version variables and call the `project` function to initiate project configuration.

- [Configure `vcpkg`](#vcpkgcmake-vcpkg-package-installermaintainer).

- Set [GNUInstallDirs][gnuinstalldirs] installation directory layout.

- If the build system tool only supports a single configuration, such as _make_ and _ninja_, default to a
  Release build configuration if `CMAKE_BUILD_TYPE` isn't set.

- Generate a system identifier, `SIMH_SYSTEM_ID`.

  `SIMH_SYSTEM_ID` is used as a `dependencies` subdirectory name on platforms for which missing external
  dependency libraries need to be built (Windows, exclusively.)

- Define and process options, specified on the command line with `-DFOO=BAR` arguments.
  - `NO_DEP_BUILD_OPTVAL`: This is the default value for `NO_DEP_BUILD`.
    - If `NO_DEP_BUILD` has a cached value already, leave it alone.
    - Initialize `NO_DEP_BUILD_OPTVAL` to `False` so that dependencies are never built.
    - When the detected system is Windows and NOT a MinGW variant and NOT using [vcpkg](#vcpkg),
      `NO_DEP_BUILD_OPTVAL` is set to `True` so that missing dependencies are built.
  - `MAC_UNIVERSAL_OPTVAL`: This is the default value for `MAC_UNIVERSAL`.
    - If `MAC_UNIVERSAL` has a cached value already, leave it alone.
    - Initialize `MAC_UNIVERSAL_OPTVAL` to `False` unless `MAC_UNIVERSAL` was specified as an option on
      the command line.
    - This is a placeholder for future work to support macOS universal binaries.

- Set `CMAKE_INSTALL_PREFIX` to `${CMAKE_SOURCE_DIR}/SIMH-install` as a default installation
  destination. Otherwise, a platform-specific prefix such as `/usr/local` might be used and cause
  unexpected surprises.

- Set the default `CMAKE_RUNTIME_OUTPUT_DIRECTORY` value to `SIMH_LEGACY_INSTALL`. `SIMH_LEGACY_INSTALL`
  is set to `${CMAKE_SOURCE_DIR}/BIN`, appending `Win32` if on Windows, to emulate the SIMH `makefile`'s
  executable output structure.

- Tweak CMake's library and include search paths so that *CMake* can find the externally built dependency
  libraries, if needed.

- [Deal with platform-specific quirkiness](#platform-quirkscmake-platform-specific-settingstweaks), such
  as compiler flags, optimization levels, HomeBrew include and library directories on macOS.

- Locate dependencies, create and populate the `os_features`, `thread_lib`, `simh_regexp`, `simh_video`
  and `simh_network` interface libraries.
  - *CMake* interface libraries encapsulate defines, include paths and dynamic/static link libraries as an
    integrated package.
  - `add_simulator` references the interface libraries via CMake's `target_link_libraries` function, which
    adds the interface libary's defines and include paths to the simulator compile flags.

    Note: If the interface library is empty, i.e., nothing was added to it because particular
    functionality wasn't found or undesired, the interface library adds nothing to the simulator's compile
    flags or link libraries. This permits consistency and simplicity in `add_simulator`'s implementation.
  - `os_features` (`os-features.cmake`): Operating-system specific features, such testing whether `-lrt`
    contains the definitions for `shm_open`, tests for assorted system headers, adds the `winmm` and
    Windows socket libraries on Windows.

    Note: `os-features.cmake` could potentially fold itself into `platform-quirks.cmake` in the future. It
    is separate for the time being for functional clarity.
  - `thread_lib` (`pthreads-dep.cmake`): Platform threading support. Usually empty for Linux and macOS
    platforms, adds the _PThreads4W_ external dependency for Windows native and XP platforms.
  - `simh_regexp`: Regular expression support, if the `WITH_REGEX` option is `True`.
  - `simh_video`: Graphics, input controller and sound support, based on [SDL2][libsdl2], if the
    `WITH_VIDEO` option is `True`. Empty interface library if `WITH_VIDEO` is `False`. `simh_video` also
    pulls in `libpng` and `SDL_ttf`, along with their dependencies.
  - `simh_network`: This interface library collects defines and include paths for network support when
    *VDE* networking is enabled and used, as well as *TUN* network defines and includes.

- Output a summary of dependencies and features.

- If the `NO_DEP_BUILD` option is `True` (which is usually the case) and there are missing dependency
  libraries, print a fatal error message with the missing libraries, suggest to the user how they might
  fix this (`.travis/deps.sh`) and exit.

- Initiate a "superbuild" if missing dependency libraries need to be built and `NO_DEP_BUILD` is `False`.
  - A superbuild is CMake's terminology for building external software and libraries (see *CMake's*
    [ExternalProject module][external_project]). When the superbuild successfully builds the external
    projects, it will re-execute the *CMake* configure/generate phase to re-detect them and continue
    building the SIMH simulators.
  - The superbuild normally happens only once. A superbuild can be reinitiated if the compiler's version
    changes or the dependencies subdirectory is cleaned.

- Add the simulators (`simh-simulators.cmake`)

- Configure packaging
  - `cpack-setup.cmake`: Per-packager configuration, i.e., customizations for the [Nullsoft Scriptable
    Install System][nullsoft], macOS installer, Debian `.deb` packager.
  - `simh-packaging.cmake`: Define simulator package families (CPack components), add documentation to the
    default runtime support component.

- End of file. Configuration is complete and *CMake* generates the build system's files appropriate to the
  specified generator, i.e., `makefile` for Unix Makefiles, `build.ninja` for the Ninja build system, etc.


## The `cmake` sub-directory

### `add_simulator.cmake`

*add_simulator* is the centerpiece around which the rest of the CMake-based infrastructure revolves. The
basic principle is to make simulator compiles very straightforward with a simple *CMake* function. The
function's full documentation for usage and options are in [README-CMake.md][cmake_readme].

`add_simulator.cmake` decomposes into eight (8) sections:

- Update `${CMAKE_SOURCE_DIR}/.git-commit-id` and `${CMAKE_SOURCE_DIR}/.git-commit-id.h` with the current
  Git hash identifier, if it has changed. These files are only rewritten if the hash identifier has
  changed, i.e., there has been a new commit.

- `build_simcore` function: This is the workhorse function that compiles the six (6) simulator core
  libraries: `simhcore`, `simhi64`, `simhz64` and their `_video` counterparts. Network support is always
  included in the core libraries. Networking is enabled by default, and must be explicitly disabled.

  Each core library includes the `simh_network`, `simh_regexp`, `os_features` and `thread_lib` interface
  libraries, which causes the core library to inherit the interface libraries' command line defines,
  include directories and link libraries. Consequently, a core library is a single `target_link_library`
  unit from *CMake*'s perspective and simplifies `add_simulator`.

  `build_simcore` also adds a `${core_libray}_cppcheck`, e.g., `simhcore_cppcheck`, to execute `cppcheck`
  static analysis on the library's source. `cppcheck` rules are only added if the `ENABLE_CPPCHECK` option
  is `True` (*CMake* configuration option) and the `cppcheck` executable is available.

- `simh_executable_template`: Common code used by the `add_simulator` and `add_unit_test` functions to
  compile an executable. The basic flow is:

   - `add_executable` to create the simulator's executable target in the build system, add the simulator's
     sources to get executable target.
   - Set the C dialect to C99, which causes the generator to add the necessary flags to the compile
     command line to request the C99 dialect. Note that some compilers can ignore the request.
   - Set `${CMAKE_RUNTIME_OUTPUT_DIRECTORY}` to `${SIMH_LEGACY_INSTALL}` to mimic the SIMH `makefile`'s
     binary output structure.
   - Add extra target compiler flags and linker flags; these flags are usually set or modified in
     `platform_quirks.cmake`.
   - MINGW and MSVC: Set the console subsystem linker option.
   - Add simulator-specific defines and includes, define `USE_DISPLAY` on the command line if video
     support requested.
   - Add the appropriate SIMH core library to executable's target link library list, i.e., select one of
     the six core librarys.

- `add_simulator`: The simulator developer's "compile this simulator" function.

  - Call `simh_executable_template` to create the simulator executable target.
  - Add the simulator executable target to the *CPack* installed executables list. If using *CMake* 3.21
    or later, also add the imported runtime artifacts. These are primarily Windows DLLs, but could also
    include macOS shared libraries.
  - *CTest* simulator test setup. Each simulator always executes `RegisterSanityCheck`. If the `TEST`
    option is passed to `add_simulator` **and** the simulator's `tests` subdirectory and test script
    exist, the test script will execute after `RegisterSanityCheck`.
  - Create a simulator `cppcheck` static analysis target if the `ENABLE_CPPCHECK` option is `True` and the
    `cppcheck` executable exists.
  - Add the `DONT_USE_INTERNAL_ROM` to the executable's command line if the `DONT_USE_ROMS` option is
    `True`. (Probably could move this earlier to after the call to `simh_executable_template`.)

- `add_unit_test`: This function is intended for future use to execute non-simulator C-code unit tests, to
  potentially subsume SIMH's `testlib` command. It is not currently used. These non-simulator unit tests
  are supposed to utilize the [Unity test framework][unity_framework], written in "pure" C.

- Add the SIMH core support libraries targets the `build_simcore` function.

- Add the `BuildROMs` executable, add the ROM header files as outputs from `BuildROMs`. Future **FIXME**:
  Add the ROM header files as build targets that depend on their respective ROM binary files so that
  they're automagically rebuilt. However, there is no corresponding rule or set of rules in the SIMH
  `makefile`, which is why this is a **FIXME**.

- Add the `frontpaneltest` executable. `frontpaneltest` provides its own `main` function, which prevents
  it from being linked directly with a SIMH core library, e.g., `simhcore`. It has to be its own special
  executable target.

### `find_package` support

[`find_package`][find_package] is *CMake*'s functionality to find a package and set variables for compile
defines, includes and link libraries, when found. *CMake* has [a collection][cmake_modules] of
`find_package` modules for well known, commonly used packages.  *CMake* searches `${CMAKE_MODULE_PATH}`
for modules outside of its packaged collection; SIMH adds `${CMAKE_SOURCE_DIR}/cmake` to
`${CMAKE_MODULE_PATH}`.

The `Find<Package>.cmake` modules used by SIMH and provided by *CMake* include:

- ZLIB: The zlib compression library
- Freetype: The Freetype font library
- PNG: The PNG graphics library

SIMH includes six `find_package` scripts:

- `FindEDITLINE.cmake`: Locates *libeditline*, adds *termcap* to the linker's library list. Applicable to
  non-Windows systems to provide command line history.

- `FindPCAP.cmake`: Locates the packet capture library's headers. SIMH does not need the `libpcap` packet
  capture library; `libpcap` and it's Win32 equivalent are dynamically loaded at run time. The headers are
  only needed for `libpcap`'s function prototypes.

- `FindPCRE.cmake`, `FindPCRE2.cmake`: Locate PCRE and PCRE2 libraries and headers.

- `FindPTW.cmake`: "PTW" is shorthand for the Windows PThreads4w POSIX threads compatibility library.

- `FindVDE.cmake`: Locates the VDE networking headers and library if the `WITH_VDE` option is `True`.

In addition to `Find<Package>.cmake` modules, packages can also supply *CMake* configuration
modules. SDL2 and SDL2-ttf generate and install *CMake* cofiguration files that are used in lieu of a
`find_package` module.

### `platform-quirks.cmake`: Platform-specific settings/tweaks

`platform_quirks.cmake` is the code container for managing platform-specific compiler and linker
settings. Specific "quirks":

- Set *CMake*'s libary architecture variables (`CMAKE_C_LIBRARY_ARCHITECTURE`,
  `CMAKE_LIBRARY_ARCHITECTURE`) on Linux.

- Windows compiler-specific quirks:
  - Ensure that SIMH links with the correct multi-threaded Visual C runtime.
  - Ensure that SIMH's source compiles with single byte character sets.
  - Increase warning verbosity if the `DEBUG_WALL` configuration option is `True`.
  - Make warnings fatal if the `WARNINGS_FATAL` configuration option is `True`.

- There are no specific quirks for Linux.

- Adjust the GNU and Clang compilers' `Release` build configuration flags, such as link-time optimization
  (LTO) and *-O2* optimization, fatal warnings, ...

- Construct the macOS *HomeBrew* and *MacPorts* header and library search paths so that `find_package`
  succeeds.

### `vcpkg.cmake`: vcpkg package installer/maintainer

Setting the `VCPKG_ROOT` environment variable enables [vcpkg][vcpkg] support in SIMH. _vcpkg_ is a package
management system designed primarily for Windows Visual C/C++ software development with integrated *CMake*
support. _vcpkg_ also supports [MinGW-w64][mingw-w64], Linux and macOS. The GitHub CI/CD pipeline does not
use _vcpkg_ for either the Linux or macOS builds. [MinGW-w64][mingw-w64] is a SIMH CMake-supported
platform, but is not normally targeted in a CI/CD pipeline. Also, [MinGW-w64][mingw-w64] uses the
[Pacman][pacman] package manager, which makes _vcpkg_ redundant.

_vcpkg_ does not support producing Windows XP binaries. *CMake* will emit a fatal error message if
`VCPKG_ROOT` is present __and__ the *CMake* configure/generate command line specified the Windows
XP-compatible toolset `-T vc141_xp`.

`CMakeLists.txt` sets the globally visible variable `USING_VCPKG`. This makes better shorthand than having
to use `DEFINED VCPKG_ROOT` in conditionals and makes those tests more readable.

Functional flow:
- If `USING_VCPKG` is `False`, return from `vcpkg-setup.cmake` immediately.

- Construct the _vcpkg_ "(architecture)-(platform)-(runtime)" triplet
  - Use the user's desired triplet if `VCPKG_DEFAULT_TRIPLET` exists as an environment variable.
  - Windows: Always choose a statically linked, static runtime triple for MS Visual C/C++ or Clang --
    "(x64|x86|arm|arm64)-windows-static".
  - MinGW-w64: Triplets are "(x64|x86)-mingw-dynamic". Note: Waiting for a new community triplet that
    supports the statically linked runtime.

- Set `VCPKG_CRT_LINKAGE` to "static" if the triplet ends in "-static". If not, set `VCPKG_CRT_LINKAGE` to
  "dynamic".

- Execute the _vcpkg_ *CMake* toolchain initialization. This installs the packages listed as dependencies
  in `vcpkg.json`:
  - pthreads
  - pcre
  - libpng
  - sdl2
  - sdl2-ttf

### `dep-locate.cmake`, `dep-link.cmake`: Dependency libraries

`dep-locate.cmake` is the consolidated code container for locating SIMH's dependency libraries:
*PCRE/PCRE2* regular expressions; *libpng* graphics for screen captures; *SDL2*, *SDL2-ttf*, *freetype*
video support, *VDE* network support, the *zlib* compression needed by *libpng* and *PCRE/PCRE2*,
etc. It is divided into two sections:

- Locate packages: First try `find_package` to locate required packages, falling back to `pkgconfig` if
  `pkgconfig` is available. `dep-locate.cmake` also understands how to use `vcpkg`-based packages, since
  some of the package names differ from what would normally be expected. *PCRE*, in particular, is named
  *unofficial-pcre* vs. *PCRE*.

- Superbuild setup: `dep-locate.cmake` constructs the external project builds for missing dependency
  libraries in preparation for the *superbuild*. Currently, this only applies to Windows. There is a list
  of source URL variables that immediately follows `include (ExternalProject)` that specify where the
  *superbuild*'s external projects download the dependency's source code.

  - *The source URL list needs to be periodically reviewed to bump library version numbers as new library
    versions are released.*
  - `sourceforce.net` mirrors' SSL responses are **very** quirky and requires multiple mirror URLs to
    succeed.

  **Future FIXME**: The superbuild would be entirely unnecessary if SIMH didn't have to support Windows XP
  binaries. `vcpkg` should be the future dependency library package manager for Windows, but it does not
  support the XP toolkit and likely never will.

`dep-link.cmake` is the consolidated code container that populates the `simh_regexp`, `simh_video` and
`simh_network` interface libraries. Note that the order in which dependency libraries are added to these
interface libraries is important: *SDL2-ttf* must precede *freetype* and *SDL2*, for example. This reverse
ordering is standard library linking practice, given that linkers only make a single pass through each
library to resolve symbols.

### `simh-simulators.cmake`

`simh-simulators.cmake` is the list of SIMH simulator subdirectories added as *CMake* subprojects. Each
simulator subdirectory has its own `CMakeLists.txt` file that contains calls to
`add_simulator`. Subprojects are a *CMake* idiom and maps well to build system generators, such as
Visual Studio and XCode, which assume a subdirectory-subproject organization.

This file is currently autogenerated by the `generate.py` Python script, although, this may not the the
case in the future.

### `generate.py`: Automagic `makefile` &rarr; CMake infrastructure

The SIMH `makefile` is still considered the authoritative source for simulator compiler command lines and
source code. `generate.py` was built to scrape the `makefile`'s `all` and `exp` rules, gather simulator
source code lists, simulator-specific defines, and emit the simulator subdirectory `CMakeLists.txt` and
the `simh-simulators.cmake` files. 

To synchronize the *CMake* infrastructure with the `makefile` when new simulators are added to the
`makefile`, when compiler command lines change or new simulator source code is added:

``` shell
$ (cd cmake; python3 -m generate)

## Alternatively:

$ python3 cmake/generate.py
```

Note that `generate.py` ensures that it has collected all of the `makefile`-s simulators by cross-referencing
the simulators enumerated in the `cmake/simgen/packaging.py` script to the collected simulators scraped from
the `makefile`. When the expected simulators do not match the `generate.py`-collected simulator list,
`generate.py` will list the missing simulators and exit. If you are maintaining a separate simulator source
repository, please customize your `cmake/simgen/packaging.py` script to reflect the expected simulators in
your source tree.


#### `generate.py` Internals

`generate.py` has three principal classes defined in the `cmake/simgen` subdirectory: `CMakeBuildSystem`,
`SimCollection` and `SIMHBasicSimulator`.

- `CMakeBuildSystem` (`cmake_container.py`): The top-level container for the entire SIMH simulator collection
  scraped from the   `makefile`. It's a container that maps a `SimCollection` simulator group to a subdirectory.
  The `CMakeBuildSystem.extract` method interprets the `makefile`'s parsed contents, and builds up the
  per-subdirectory `SimCollection` simulator groups.

- `SimCollection` (`sim_collection.py`): A group of simulators, e.g., all of the VAX or PDP-11 simulators,
  or the PDP-8 simulator. It also maps simulator source macro names to source lists that become *CMake* 
  variables to make the emitted `CMakeLists.txt` files more readable. The `SimCollection.write_simulators`
  method emits the simulator subdirectory `CMakeLists.txt` file.

- `SIMHBasicSimulator` (`basic_simulator.py`): An individual SIMH simulator stored inside a `SimCollection`.
  This class tracks of the simulator's sources, simulator-specific defines and include paths, as well as 
  detects when the simulator requires 64-bit address and 64-bit data, when the simulator requires video 
  support. The `SIMHBasicSimulator.write_section` method emits the individual simulator's `add_simulator`
  function call to the `CMakeLists.txt` file stream passed by the parent `SimCollection`.

  `SIMHBasicSimulator` has several subclasses that specialize the `write_section` method. For example, the
  *BESM6* simulator requires a Cyrillic font, which requires additional *CMake* code to search for an
  available font from a list of known Cyrillic-supporting fonts. The `VAXSimulator` class injects *CMake*
  code to symlink, hard link or copy the `vax` executable to `microvax3900`. The
  `SimCollection.special_simulators` dictionary maps simulator names to specialized `SIMHBasicSimulator`
  subclasses. If the simulator's name is not present in the `special_simulators` dictionary,
  `SIMHBasicSimulator` is used.

#### Using `generate.py` outside of *open-simh*

`generate.py` can be used outside of the *open-simh* project for separately maintained simulator
repositories. If you do use `generate.py` in your own simulator repository, you **must** customize
the `simgen/packaging.py` script so that the expected simulators matches the collected simulators.

An example `packaging.py` script that can be copied, pasted and customized (leave the boilerplate
in place, edit and customize after the "Your customizations here...")

```python
### packaging.py boilerplate starts:

import os
import functools

## Initialize package_info to an empty dictionary here so
## that it's visible to write_packaging().
package_info = {}


class SIMHPackaging:
    def __init__(self, family, install_flag = True) -> None:
        self.family = family
        self.processed = False
        self.install_flag = install_flag

    def was_processed(self) -> bool:
        return self.processed == True
    
    def encountered(self) -> None:
        self.processed = True

class PkgFamily:
    def __init__(self, component_name, display_name, description) -> None:
        self.component_name = component_name
        self.display_name   = display_name
        self.description    = description

    def write_component_info(self, stream, indent) -> None:
        pkg_description = self.description
        if pkg_description[-1] != '.':
            pkg_description += '.'
        sims = []
        for sim, pkg in package_info.items():
            if pkg.family is self and pkg.was_processed():
                sims.append(sim)
        sims.sort()

        if len(sims) > 0:
            sims.sort()
            pkg_description += " Simulators: " + ', '.join(sims)
            indent0 = ' ' * indent
            indent4 = ' ' * (indent + 4)
            stream.write(indent0 + "cpack_add_component(" + self.component_name + "\n")
            stream.write(indent4 + "DISPLAY_NAME \"" + self.display_name + "\"\n")
            stream.write(indent4 + "DESCRIPTION \"" + pkg_description + "\"\n")
            stream.write(indent0 + ")\n")

    def __lt__(self, obj):
        return self.component_name < obj.component_name
    def __eq__(self, obj):
        return self.component_name == obj.component_name
    def __gt__(self, obj):
        return self.component_name > obj.component_name
    def __hash__(self):
        return hash(self.component_name)

def write_packaging(toplevel_dir) -> None:
    families = set([sim.family for sim in package_info.values()])
    pkging_file = os.path.join(toplevel_dir, 'cmake', 'simh-packaging.cmake')
    print("==== writing {0}".format(pkging_file))
    with open(pkging_file, "w") as stream:
        ## Runtime support family:
        stream.write("""## The default runtime support component/family:
cpack_add_component(runtime_support
    DISPLAY_NAME "Runtime support"
    DESCRIPTION "Required SIMH runtime support (documentation, shared libraries)"
    REQUIRED
)

## Basic documentation for SIMH
install(FILES doc/simh.doc TYPE DOC COMPONENT runtime_support)

""")

        ## Simulators:
        for family in sorted(families):
            family.write_component_info(stream, 0)

## The default packaging family for simulators not associated with
## any particular family. Also used for runtime and documentation:
default_family = PkgFamily("default_family", "Default SIMH simulator family.",
    """The SIMH simulator collection of historical processors and computing systems that do not belong to
any other simulated system family"""
)

### packaging.py boilerplate ends...

### Your customizations here:

### Instantiate a simulator package family:
foosim_family = PkgFamily("foosim_family", "A collection of simulators",
    """Description of your simulators, may span multiple lines within the three quote
marks."""
)

### Add simulators to the package famil(y|ies)
package_info["sim1"] = SIMHPackaging(foosim_family)
package_info["sim2"] = SIMHPackaging(foosim_family)
```


### CPack configuration

[*CPack*][cpack] is the *CMake* utility for packaging SIMH's pre-built binaries. Like *CMake*, *CPack* has
its own generators that support different packaging systems. Some packaging systems only support a single,
all-in-one package, like Debian `.deb` packages. Other packaging systems support a finer grained approach
with individually selectable package components, like the NullSoft Installation System (NSIS) and macOS'
"productbuild" packagers.

`cpack-setup.cmake` imports the *CPack* module and customizes *CPack* variables and generator
settings. Principally, `cpack-setup.cmake` sets the output name for the package, which is
`simh-major.minor-suffix`. The suffix defaults to the system's name, `${CMAKE_SYSTEM_NAME}` for
non-Windows operating systems, and `(win64|win32)-(Debug|Release)-(vs2022|vs2019|vs2017|vs2015)[-xp]` for
Windows systems. The `-xp` suffix is only appended when building Windows XP-compatible binaries. Setting
the `SIMH_PACKAGE_SUFFIX` environment variable overrides the default suffix.

Platform and generator-specific settings in `cpack-setup.cmake` include:

- Runtime installation exclusions: A list of files that should be excluded from the resulting installation
  package. For Windows, *CPack* should not install well known DLLs or anything from the `system32`
  directory. Linux and macOS do not currently have a runtime exclusion list. If that changes, edit the
  `pre_runtime_exclusions` and `post_runtime_exclusions` variables' regular expressions.

- NullSoft Installation System (NSIS): Arrange to use the `installer-customizations/NSIS.template.in`
  script timeplate (see below.) NSIS also uses "SIMH-major.minor" (e.g., "SIMH-4.0") as the installation
  directory's name and defaults to installing SIMH in the user's `$LocalAppData\Programs".

- WIX Windows Installer: Sets the WIX GUID for SIMH. This GUID was entirely fabricated by a GUID
  generator.

- Debian: Adds Debian package dependencies for *libsd2*, *libsd2-ttf*, *libpcap*, *libvdeplug2* and
  *libedit2*. **This dependency list needs to be periodically revisited when dependencies change.**

`simh-packaging.cmake` defines package components and basic runtime support installation common to all
simulators. Package components are groups of SIMH simulator "families" -- there is a strong correlation
between a simulator family and the SIMH simulator subdirectories. For example, all of the VAX simulators
belong to the `vax_family` component. `simh-packaging.cmake` creates the `vax_family` *CPack* component
and `add_simulator` adds a simulator to its package family via its `PKG_FAMILY <family>` option.

The `experimental_family` is the exception to the simulator family-subdirectory rule, serving as the
package family for the currently experimental simulators.

The `runtime_support` family is intended for executables and documentation common to all
simulators. Currently, only the `simh.doc` SIMH documentation is part of this family. Future **FIXME**:
populate this family with additional, non-simulator-specific documentation.

The `cmake/installer-customizations` subdirectory is where *CPack*-specific generator customizations
should be kept. The NSIS installer template was altered so that the resulting SIMH installer executable
only required user privileges to installer; the default escalates to "admin" privileges, which are
unnecessary for SIMH.

<!-- Reference links -->
[cmake]: https://cmake.org
[cmake_modules]: https://gitlab.kitware.com/cmake/cmake/-/tree/master/Modules
[cmake_readme]: ../README-CMake.md
[cpack]: https://cmake.org/cmake/help/latest/module/CPack.html
[cpack_generators]: https://cmake.org/cmake/help/latest/manual/cpack-generators.7.html#manual:cpack-generators(7)
[external_project]: https://cmake.org/cmake/help/latest/module/ExternalProject.html
[find_package]: https://cmake.org/cmake/help/latest/command/find_package.html
[gnuinstalldirs]: https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html
[libsdl2]: https://github.com/libsdl-org/SDL
[mingw-w64]: https://www.mingw-w64.org/
[nullsoft]: https://sourceforge.net/projects/nsis/
[pacman]: https://archlinux.org/pacman/
[unity_framework]: http://www.throwtheswitch.org/unity
[vcpkg]: https://vcpkg.io
[vde_network]: https://wiki.virtualsquare.org/#!index.md

<!--
Local Variables:
mode: gfm
eval: (markdown-toc-mode 1)
eval: (auto-fill-mode 1)
markdown-enable-math: t
fill-column: 106
End:
-->
