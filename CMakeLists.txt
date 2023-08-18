## Note: pthreads4w requires 3.14 or better.
cmake_minimum_required(VERSION 3.14)

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR
         ""
        "!!! DO NOT BUILD CMake artifacts in the SIMH source directory !!!\n"
        ""
        "Create a subdirectory and build in that subdirectory, e.g.:"
        "\n"
        "  $ mkdir cmake-build\n"
        "  $ cd cmake-build\n"
        "  $ cmake -G \"your generator here\" ..\n"
        ""
        "Preventing in-tree source build.")
endif ()

if (CMAKE_VERSION VERSION_LESS "3.21" AND NOT DEFINED SIMH_INSTALLER_WARNING)
    message(WARNING "!!! DO NOT CREATE INSTALLERS WITH THIS VERSION OF CMAKE (${CMAKE_VERSION}) !!!"
        "\n"
        "Do not create a release or installers with this version of CMake. It does not have "
        "the required install(RUNTIME_DEPENDENCY_SET) or install(IMPORTED_RUNTIME_ARTIFACTS) "
        "functionality to collect runtime executable dependencies as part of the installation"
        " packages, such as Mac OS bundles or Windows DLLs."
        "\n"
        "Minimum version for creating installers with CPack is 3.21."
    )
    set(SIMH_INSTALLER_WARNING TRUE CACHE BOOL "Installer/CPack warning issued when CMAKE_VERSION < 3.21" FORCE)
endif ()

## SIMH Version variables:
set(SIMH_VERSION_MAJOR 4)
set(SIMH_VERSION_MINOR 1)
set(SIMH_VERSION_PATCH 0)
set(SIMH_VERSION "${SIMH_VERSION_MAJOR}.${SIMH_VERSION_MINOR}.${SIMH_VERSION_PATCH}"
    CACHE PATH "Open-Simh version string.")

# Places to look for CMake modules/includes
set(SIMH_INCLUDE_PATH_LIST
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/cmake/installer-customizations)
list(APPEND CMAKE_MODULE_PATH ${SIMH_INCLUDE_PATH_LIST})
message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")

## vcpkg sanity checking: Cannot use vcpkg and XP toolkit together. If this is
## a XP build, disable vcpkg:
set(USING_VCPKG FALSE)
if (DEFINED ENV{VCPKG_ROOT})
    if (CMAKE_GENERATOR_TOOLSET MATCHES "v[0-9][0-9][0-9]_xp")
        message(FATAL_ERROR
            "Configuration conflict: Cannot build XP binaries with vcpkg. Either "
            "unset VCPKG_ROOT in your environment or remove the '-T' toolkit option."
            "\n"
            "Also remove CMakeCache.txt and recursively remove the CMakeFiles "
            "subdirectory in your build folder before reconfiguring.")
    endif ()

    set(USING_VCPKG TRUE)

    ## Defer loading the CMAKE_TOOLCHAIN_FILE:
    set(SIMH_CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    if(DEFINED CMAKE_TOOLCHAIN_FILE)
        ## Use the user's provided toolchain file, but load it later.
        message(STATUS "Deferring CMAKE_TOOLCHAIN_FILE load.")
        set(SIMH_CMAKE_TOOLCHAIN_FILE "${CMAKE_TOOLCHAIN_FILE}")
        unset(CMAKE_TOOLCHAIN_FILE)
        unset(CMAKE_TOOLCHAIN_FILE CACHE)
    endif()

    file(TO_CMAKE_PATH "${SIMH_CMAKE_TOOLCHAIN_FILE}" SIMH_CMAKE_TOOLCHAIN_FILE)
    message(STATUS "SIMH_CMAKE_TOOLCHAIN_FILE is ${SIMH_CMAKE_TOOLCHAIN_FILE}")
endif ()

## Respect MSVC_RUNTIME_LIBRARY's setting. the policy has to be set before
## project(), otherwise it'd have been put into platform-quirks.
##
## Note: See cmake\build_dep_matrix.cmake to see how this is propagated down
## into the dependency libraries.
if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.15)
    cmake_policy(SET CMP0091 NEW)
endif()

project(simh VERSION "${SIMH_VERSION}" LANGUAGES C CXX)

include(vcpkg-setup)
include(GNUInstallDirs)

## Provide a default CMAKE_BUILD_TYPE if CMAKE_CONFIGURATION_TYPES is empty or not defined.
if (NOT CMAKE_CONFIGURATION_TYPES)
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
        message(STATUS "CMAKE_BUILD_TYPE defaulted to ${CMAKE_BUILD_TYPE}")
    else (NOT CMAKE_BUILD_TYPE)
        message(STATUS "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
    endif (NOT CMAKE_BUILD_TYPE)
endif ()

# SIMH_SYSTEM_ID: Roughly analogous to the autoconf system triple. Used (almost exclusively)
# as part of the dependencies' top-level directory name.
set(SIMH_SYSTEM_ID ${CMAKE_SYSTEM_NAME})
string(REPLACE "." ";" version_list ${CMAKE_SYSTEM_VERSION})
list(GET version_list 0 version_major)
string(APPEND SIMH_SYSTEM_ID "-" ${version_major})

if (CMAKE_C_LIBRARY_ARCHITECTURE)
    string(APPEND SIMH_SYSTEM_ID -${CMAKE_C_LIBRARY_ARCHITECTURE})
endif (CMAKE_C_LIBRARY_ARCHITECTURE)
string(APPEND SIMH_SYSTEM_ID -${CMAKE_C_COMPILER_ID})
string(REPLACE "." ";" version_list ${CMAKE_C_COMPILER_VERSION})
list(GET version_list 0 version_major)
list(GET version_list 1 version_minor)
if (NOT version_minor)
  set(version_minor 0)
endif ()
string(APPEND SIMH_SYSTEM_ID "-${version_major}.${version_minor}")
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    string(APPEND SIMH_SYSTEM_ID "-64")
else ()
    string(APPEND SIMH_SYSTEM_ID "-32")
endif ()
if (CMAKE_GENERATOR MATCHES "Visual Studio .*" AND CMAKE_GENERATOR_TOOLSET MATCHES "v[0-9][0-9][0-9]_xp")
    string(APPEND SIMH_SYSTEM_ID "-XP")
endif ()

# SIMH_DEP_TOPDIR: This is the top-level directory where dependencies live. It's cached so that
# scripts (e.g., cmake-builder.ps1) can update PATH to find shared objects and DLLs.
#
# Only add to search paths if NO_DEP_BUILD is False
set(SIMH_DEP_TOPDIR
    ${CMAKE_SOURCE_DIR}/cmake/dependencies/${SIMH_SYSTEM_ID}
    CACHE PATH "Top-level directory for SIMH library dependencies")

# SIMH_DEP_PATCHES: This is where various external package patches live. Also cached just in case
# it's needed by a script.
set(SIMH_DEP_PATCHES ${CMAKE_SOURCE_DIR}/cmake/patches
        CACHE PATH "Top-level directory for external dependency patches")

##-- Options --##
set(NO_DEP_BUILD_OPTVAL FALSE)
if (NOT DEFINED NO_DEP_BUILD)
    ## For Windows, always build dependencies
    if (WIN32 AND NOT DEFINED ENV{MSYSTEM} AND NOT USING_VCPKG)
        message(STATUS "Setting NO_DEP_BUILD to FALSE, will BUILD missing dependencies")
        set(NO_DEP_BUILD_OPTVAL FALSE)
    else ()
        message(STATUS "Setting NO_DEP_BUILD to TRUE, will NOT BUILD missing dependencies")
        set(NO_DEP_BUILD_OPTVAL TRUE)
    endif ()
else ()
    set(NO_DEP_BUILD_OPTVAL ${NO_DEP_BUILD})
    if (NO_DEP_BUILD_OPTVAL)
        message(STATUS "Missing dependencies WILL NOT BE BUILT.")
    else ()
        message(STATUS "Missing dependencies WILL BE BUILT.")
    endif ()
endif ()

set(MAC_UNIVERSAL_OPTVAL FALSE)
if (NOT DEFINED MAC_UNIVERSAL)
    if (APPLE)
        message("macOS universal binaries WILL NOT BE BUILT")
    endif ()
else ()
    set(MAC_UNIVERSAL_OPTVAL ${MAC_UNIVERSAL})
    if (MAC_UNIVERSAL_OPTVAL)
        message(STATUS "macOS universal binaries WILL BE BUILT.")
    else ()
        message(STATUS "macOS universal binaries NOT WILL BE BUILT.")
    endif ()
endif ()

option(NO_DEP_BUILD
       "Enable (=1)/disable (=0) Dependency library builds (def: enabled)"
       ${NO_DEP_BUILD_OPTVAL})
option(BUILD_SHARED_DEPS
       "Enable (=1)/disable (=0) building dependencies as shared libraries/DLLs (def: disabled)"
       FALSE)
option(WITH_ASYNC
       "Enable (=1)/disable (=0) Asynchronous I/O and threading."
       TRUE)
option(WITH_REGEX
       "Enable (=1)/disable (=0) PCRE/PCRE2 regular expression support (def: enabled)"
       TRUE)
option(PREFER_PCRE
       "Prefer (=1)/ignore (=0) Prefer PCRE over PCRE2 (def: ignore)"
       FALSE)
option(WITH_NETWORK
       "Enable (=1)/disable (=0) simulator networking support. (def: enabled)"
       TRUE)
option(WITH_PCAP
       "Enable (=1)/disable (=0) libpcap (packet capture) support. (def: enabled)"
       TRUE)
option(WITH_SLIRP
       "Enable (=1)/disable (=0) SLIRP network support. (def: enabled)"
       TRUE)
option(WITH_VDE
       "Enable (=1)/disable (=0) VDE2/VDE4 network support. (def: enabled)"
       TRUE)
option(WITH_TAP
       "Enable (=1)/disable (=0) TAP/TUN device network support. (def: enabled)"
       TRUE)
option(WITH_VMNET
       "Enable (=1)/disable (=0) macOS vmnet.framework network support. (def: enabled)"
       TRUE)
option(WITH_VIDEO
       "Enable (=1)/disable (=0) simulator display and graphics support (def: enabled)"
       TRUE)
option(DONT_USE_ROMS
       "Enable (=1)/disable (=0) building support ROMs. (def: disabled)"
       FALSE)
option(ENABLE_CPPCHECK
       "Enable (=1)/disable (=0) 'cppcheck' static code analysis. (def: disabled.)"
       FALSE)
option(WINAPI_DEPRECATION
       "Show (=1)/mute (=0) Various WinAPI deprecation warning (def: mute.)"
       FALSE)
option(WARNINGS_FATAL
       "Compile-time warnings are errors (e.g., \"-Werror\" on GCC)"
       FALSE)
option(RELEASE_LTO
       "Use Link Time Optimization (LTO) in Release builds."
       FALSE)
option(DEBUG_WALL
       "Turn on maximal compiler warnings in Debug builds (e.g., \"-Wall\" or \"/W4\")"
       FALSE)
option(SIMH_PACKAGE_SUFFIX
       "The packaging suffix for CPack artifacts, e.g. simh-SIMH_VERSION-SIMH_PACKAGE_SUFFIX."
       "")
option(MAC_UNIVERSAL
       "macOS universal binary flag: TRUE -> build universal binaries, FALSE -> don't."
       ${MAC_UNIVERSAL_OPTVAL})

# Places where CMake should look for dependent package configuration fragments and artifacts:
set(SIMH_PREFIX_PATH_LIST)
if (NOT (USING_VCPKG OR NO_DEP_BUILD))
    list(APPEND CMAKE_INCLUDE_PATH ${SIMH_DEP_TOPDIR})
    list(APPEND SIMH_PREFIX_PATH_LIST ${SIMH_DEP_TOPDIR})
    list(APPEND CMAKE_PREFIX_PATH ${SIMH_PREFIX_PATH_LIST})
endif ()

## libpcap, if not found, will put its headers in $CMAKE_BINARY_DIR/include.
## Make sure we can find them.
list(APPEND CMAKE_INCLUDE_PATH "${CMAKE_BINARY_DIR}/include")

## Additional command line arguments for dependencies. Need this because Powershell will report
## an error if anything is sent to stderr and $ErrorDefaultAction is set to "Stop".
set(DEP_CMAKE_ARGS "-Wno-dev" "--no-warn-unused-cli")

## build-stage directory hierarchy for dependency installs:
if (NOT (USING_VCPKG OR NO_DEP_BUILD) AND NOT EXISTS ${SIMH_DEP_TOPDIR})
    message(STATUS "Creating dependency library directory hierarchy")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SIMH_DEP_TOPDIR} ${SIMH_DEP_TOPDIR}/include ${SIMH_DEP_TOPDIR}/lib
                    ${SIMH_DEP_TOPDIR}/bin
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif ()

## Default install location is ${CMAKE_SOURCE_DIR}/SIMH-install if not otherwise set
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/SIMH-install" CACHE PATH "${CMAKE_SOURCE_DIR}/SIMH-install default install prefix" FORCE)
endif()

## The default runtime output directory is the BIN subdirectory in the source tree's top
set(SIMH_LEGACY_INSTALL "${CMAKE_SOURCE_DIR}/BIN")
if (WIN32)
    string(APPEND SIMH_LEGACY_INSTALL "/Win32")
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SIMH_LEGACY_INSTALL})

## Source directory is always on the global include path.
include_directories(${CMAKE_SOURCE_DIR})

if (NOT USING_VCPKG AND NOT NO_DEP_BUILD)
    # Make sure we can include and link from the dependency directory
    list(APPEND CMAKE_LIBRARY_PATH ${SIMH_DEP_TOPDIR}/lib)
    link_directories(${SIMH_DEP_TOPDIR}/lib)
    list(APPEND CMAKE_INCLUDE_PATH ${SIMH_DEP_TOPDIR}/include)
    include_directories(${SIMH_DEP_TOPDIR}/include)
endif ()

## CMake policy tweaks

if (CMAKE_VERSION GREATER_EQUAL 3.24)
    ## Until I figure out a way to add DOWNLOAD_EXTRACT_TIMESTAMP to the call to
    ## ExternalProject_Add cleanly..
    cmake_policy(SET CMP0135 NEW)
endif()

## "Standard" includes
include(CheckCSourceCompiles)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckTypeSize)
include(CheckCCompilerFlag)
include(FindPackageHandleStandardArgs)

## Deal with platform quirkiness
include(platform-quirks)

# Find packages, arrange for dependency download/compile/install:
#
# SIMH_BUILD_DEPS is the list of the dependencies' names, for pretty-printing.
# SIMH_DEP_TARGETS is the list of dependency targets' names that we'll actually build.

set(SIMH_BUILD_DEPS)
set(SIMH_DEP_TARGETS)

set(THREADING_PKG_STATUS "unknown")
set(ZLIB_PKG_STATUS "unknown")
set(PCRE_PKG_STATUS "unknown")
set(VIDEO_PKG_STATUS)
set(NETWORK_PKG_STATUS)

# if (USING_VCPKG)
#     ## Sanity checking output: Ensure that vcpkg picked up the correct triplet
#     message(STATUS "VCPKG sanity check:\n"
#         "    .. VCPKG target triplet is ${VCPKG_TARGET_TRIPLET}\n"
#         "    .. VCPKG_CRT_LINKAGE is ${VCPKG_CRT_LINKAGE}"
#         ## "    .. CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}\n"
#         ## "    .. CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
#     )
# endif ()

# SIMH local packages:
include(build_dep_matrix)
include(os-features)
include(dep-locate)
include(dep-link)

if (VIDEO_PKG_STATUS)
    string(REPLACE ";" ", " VIDEO_PKG_STATUS "${VIDEO_PKG_STATUS}")
else (VIDEO_PKG_STATUS)
    set(VIDEO_PKG_STATUS "unknown")
endif (VIDEO_PKG_STATUS)

if (NETWORK_PKG_STATUS)
    string(REPLACE ";" ", " NETWORK_PKG_STATUS "${NETWORK_PKG_STATUS}")
else (NETWORK_PKG_STATUS)
    set(NETWORK_PKG_STATUS "unknown")
endif (NETWORK_PKG_STATUS)

set(CPPCHECK_STATUS "disabled.")

if (ENABLE_CPPCHECK)
    find_program(cppcheck_cmd NAMES cppcheck)
    if (cppcheck_cmd)
        list(APPEND cppcheck_cmd
             "--language=c"
             "--enable=warning,style,performance,portability,information,missingInclude"
             "--suppress=missingIncludeSystem"
             "--inline-suppr"
             "--std=c99"
             "--force")
        set(CPPCHECK_STATUS "found and enabled.")
        if (WIN32)
            if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                list(APPEND cppcheck_cmd
                     "--platform=win64")
                set(CPPCHECK_STATUS "found, Win64 platform.")
            else ()
                list(APPEND cppcheck_cmd
                     "--platform=win32A")
                set(CPPCHECK_STATUS "found, Win32 ASCII platform.")
            endif ()
        endif ()
    else (cppcheck_cmd)
        set(CPPCHECK_STATUS "'cppcheck' not installed.")
    endif ()
endif ()

set(_feature_text "Libraries and features:\n")
string(APPEND _feature_text "\n    * Build with video/graphics support. ${BUILD_WITH_VIDEO}")
string(APPEND _feature_text "\n    * Build with networking support .... ${BUILD_WITH_NETWORK}")
string(APPEND _feature_text "\n    * Build internal ROMS .............. ")
if (DONT_SET_ROMS)
    string(APPEND _feature_text "No")
else ()
    string(APPEND _feature_text "Yes")
endif ()
string(APPEND _feature_text "\n    * Thread support ................... ${THREADING_PKG_STATUS}")
string(APPEND _feature_text "\n    * zlib ............................. ${ZLIB_PKG_STATUS}")
string(APPEND _feature_text "\n    * Perl-Compatible RegExps........... ${PCRE_PKG_STATUS}")
string(APPEND _feature_text "\n    * PNG, Freetype, SDL2, SDL2_ttf .... ${VIDEO_PKG_STATUS}")
string(APPEND _feature_text "\n    * Network support .................. ${NETWORK_PKG_STATUS}")

get_target_property(OS_FEATURE_DEFS os_features INTERFACE_COMPILE_DEFINITIONS)
list(LENGTH OS_FEATURE_DEFS len_os_features)
string(APPEND _feature_text "\n    * OS Feature definitions")
if (OS_FEATURE_DEFS)
    string(APPEND _feature_text ":")
    foreach (feature ${OS_FEATURE_DEFS})
        string(APPEND _feature_text "\n      .. ${feature}")
    endforeach()
else ()
    string(APPEND _feature_text " ........... None defined.")
endif ()

get_target_property(OS_FEATURE_LIBS os_features INTERFACE_LINK_LIBRARIES)
list(LENGTH OS_FEATURE_LIBS len_os_features)
string(APPEND _feature_text "\n    * OS Feature libraries")
if (OS_FEATURE_LIBS)
    string(APPEND _feature_text ":")
    foreach (feature ${OS_FEATURE_LIBS})
        string(APPEND _feature_text "\n      .. ${feature}")
    endforeach ()
else ()
    string(APPEND _feature_text " ............. None required.")
endif ()

string(APPEND _feature_text "\n    * 'cppcheck' ....................... ${CPPCHECK_STATUS}")
string(APPEND _feature_text "\n")

message(STATUS ${_feature_text})
unset(_feature_text)

unset(ROM_STATUS)

if (NO_DEP_BUILD AND SIMH_BUILD_DEPS)
    ## Don't build dependencies. Just wail about them.
    message("")
    message("Missing SIMH dependencies:")
    foreach (dep ${SIMH_BUILD_DEPS})
        message(STATUS "    ${dep}")
    endforeach()
    message("")
    message("Please install the above dependencies via your platform package management or")
    message("software installation system and reconfigure.")
    message("")
    message("Also see the .travis/deps.h file for Brew and apt packages installed during")
    message("github.com workflow actions.")
    message(FATAL_ERROR "Missing dependencies, cannot continue.")

    ## TODO: Assume that these dependencies are optional?
endif ()

if (NOT DEFINED DO_DEPENDENCY_BUILD OR SIMH_BUILD_DEPS)
    if (DEFINED DO_DEPENDENCY_BUILD AND NOT DO_DEPENDENCY_BUILD AND SIMH_BUILD_DEPS)
        message(FATAL_ERROR "Dependency libraries did not build successfully!!??")
    endif()

    if (SIMH_BUILD_DEPS)
        message(STATUS "Building dependency libraries as a superbuild")
        set(DO_DEPENDENCY_BUILD ON CACHE BOOL "Superbuild flag" FORCE)
    else ()
        set(DO_DEPENDENCY_BUILD OFF CACHE BOOL "Superbuild flag" FORCE)
    endif ()
else ()
    set(DO_DEPENDENCY_BUILD ${DO_DEPENDENCY_BUILD} CACHE BOOL "Superbuild flag" FORCE)
endif ()

if (NOT DO_DEPENDENCY_BUILD)
    include (add_simulator)
    if (WITH_SLIRP)
      add_subdirectory(slirp)
    endif (WITH_SLIRP)

## Don't delete yet ##    set(Python_ADDITIONAL_VERSIONS 3)
## Don't delete yet ##    include(FindPythonInterp)
## Don't delete yet ##    if (PYTHONINTERP_FOUND AND PYTHON_VERSION_MAJOR GREATER_EQUAL 3)
## Don't delete yet ##        add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/cmake/simh-simulators.cmake
## Don't delete yet ##          COMMAND ${PYTHON_EXECUTABLE} "-m" generate "--srcdir" "${CMAKE_SOURCE_DIR}"
## Don't delete yet ##            MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/makefile
## Don't delete yet ##            COMMENT "Updating cmake/simh-simulators.cmake"
## Don't delete yet ##            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmake
## Don't delete yet ##            COMMAND_EXPAND_LISTS)
## Don't delete yet ##        message(STATUS "Added makefile update rule.")
## Don't delete yet ##    endif (PYTHONINTERP_FOUND AND PYTHON_VERSION_MAJOR GREATER_EQUAL 3)
## Don't delete yet ##
## Don't delete yet ##    if (${CMAKE_SOURCE_DIR}/makefile IS_NEWER_THAN ${CMAKE_SOURCE_DIR}/cmake/simh-simulators.cmake)
## Don't delete yet ##        if (NOT PYTHONINTERP_FOUND OR PYTHON_VERSION_MAJOR LESS 3)
## Don't delete yet ##            if (NOT PYTHONINTERP_FOUND)
## Don't delete yet ##                message("!! Python not found")
## Don't delete yet ##            elseif (PYTHON_VERSION_MAJOR LESS 3)
## Don't delete yet ##                message("!! Python is not python3")
## Don't delete yet ##            endif (NOT PYTHONINTERP_FOUND)
## Don't delete yet ##
## Don't delete yet ##            message("!!")
## Don't delete yet ##            message("!! Cannot update cmake/simh-simulators.cmake, using older version.")
## Don't delete yet ##            message("!! Proceed with caution: This build might not succeed.")
## Don't delete yet ##            message("!!")
## Don't delete yet ##            message(STATUS "Keeping existing cmake/simh-simulators.cmake")
## Don't delete yet ##        else (NOT PYTHONINTERP_FOUND OR PYTHON_VERSION_MAJOR LESS 3)
## Don't delete yet ##            message(STATUS "Updating cmake/simh-simulators.cmake")
## Don't delete yet ##            execute_process(COMMAND ${PYTHON_EXECUTABLE} "-m" generate "--srcdir" "${CMAKE_SOURCE_DIR}"
## Don't delete yet ##                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmake)
## Don't delete yet ##        endif (NOT PYTHONINTERP_FOUND OR PYTHON_VERSION_MAJOR LESS 3)
## Don't delete yet ##    endif ()

    message(STATUS "Adding simulators")
    include(simh-simulators)

    include(cpack-setup)
    include(simh-packaging)
else ()
    message(STATUS "")
    message(STATUS "Building the following dependency libraries:")
    message(STATUS "")
    foreach(dep ${SIMH_BUILD_DEPS})
        message(STATUS "    ${dep}")
    endforeach ()
    message(STATUS "")

    ExternalProject_Add (simh_superbuild
        DEPENDS
            ${SIMH_DEP_TARGETS}
        SOURCE_DIR
            ${CMAKE_SOURCE_DIR}
        CMAKE_ARGS
            -DDO_DEPENDENCY_BUILD:BOOL=OFF
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        INSTALL_COMMAND ""
        BINARY_DIR
            ${CMAKE_BINARY_DIR}
        USES_TERMINAL_CONFIGURE TRUE
        USES_TERMINAL_BUILD     TRUE
    )

    ## Ensure that the next build rechecks the dependency
    ## libraries that were just built and finds them.
    unset(Freetype_FOUND CACHE)
    unset(PCAP_FOUND CACHE)
    unset(PCRE_FOUND CACHE)
    unset(PCRE2_FOUND CACHE)
    unset(PNG_FOUND CACHE)
    unset(PTW_FOUND CACHE)
    unset(SDL2_FOUND CACHE)
    unset(SDL2_ttf_FOUND CACHE)
    unset(ZLIB_FOUND CACHE)
endif ()
