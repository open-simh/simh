if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message("")
  message("*** Do NOT build or generate CMake artifacts in the SIMH source directory! ***")
  message("")
  message("Create a subdirectory and build in that subdirectory, e.g.:")
  message("")
  message("  $ mkdir cmake-build")
  message("  $ cd cmake-build")
  message("  $ cmake -G \"your generator here\" ..")
  message("")
  message(FATAL_ERROR "Preventing in-tree source build.")
endif ()

## Note: pthreads4w requires 3.14 or better.
cmake_minimum_required(VERSION 3.10)

set(SIMH_VERSION "4.0.0")
project(simh VERSION "${SIMH_VERSION}" LANGUAGES C CXX)

include(GNUInstallDirs)
include(CheckCSourceCompiles)
include(CheckCCompilerFlag)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(ExternalProject)

find_package(PkgConfig)
find_package(PackageHandleStandardArgs)

## Provide a default CMAKE_BUILD_TYPE if CMAKE_CONFIGURATION_TYPES is empty or not defined.
if (NOT CMAKE_CONFIGURATION_TYPES)
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
        message(STATUS "CMAKE_BUILD_TYPE defaulted to ${CMAKE_BUILD_TYPE}")
    else (NOT CMAKE_BUILD_TYPE)
        message(STATUS "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
    endif (NOT CMAKE_BUILD_TYPE)
endif ()

# For 64-bit builds (and this is especially true for MSVC), set the library
# architecture.
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # For the WinPcap and Npcap SDKs, the Lib subdirectory of the top-level
    # directory contains 32-bit libraries; the 64-bit libraries are in the
    # Lib/x64 directory.
    #
    # The only way to *FORCE* CMake to look in the Lib/x64 directory
    # without searching in the Lib directory first appears to be to set
    # CMAKE_LIBRARY_ARCHITECTURE to "x64".
    #
    if (MSVC OR MINGW)
        set(CMAKE_C_LIBRARY_ARCHITECTURE "x64")
        set(CMAKE_LIBRARY_ARCHITECTURE "x64")
    else (EXISTS /usr/lib/x86_64-linux-gnu)
        # Linux-ism
        set(CMAKE_C_LIBRARY_ARCHITECTURE "x86_64-linux-gnu")
        set(CMAKE_LIBRARY_ARCHITECTURE "x86_64-linux-gnu")
    endif ()
endif()

# SIMH_SYSTEM_ID: Roughly analogous to the autoconf system triple. Used (almost exclusively)
# as part of the depedencies' top-level directory name. 
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
endif (NOT version_minor)
string(APPEND SIMH_SYSTEM_ID "-${version_major}.${version_minor}")

# SIMH_DEP_TOPDIR: This is the top-level directory where dependencies live. It's cached so that
# scripts (e.g., cmake-builder.ps1) can update PATH to find shared objects and DLLs.
set(SIMH_DEP_TOPDIR
    ${CMAKE_SOURCE_DIR}/cmake/dependencies/${SIMH_SYSTEM_ID}
    CACHE PATH "Top-level directory for SIMH library dependencies")

# SIMH_DEP_PATCHES: This is where various external package patches live. Also cached just in case
# it's needed by a script.
set(SIMH_DEP_PATCHES ${CMAKE_SOURCE_DIR}/cmake/patches
        CACHE PATH "Top-level directory for external dependency patches")

# Places to look for CMake modules/includes
set(SIMH_INCLUDE_PATH_LIST ${CMAKE_SOURCE_DIR}/cmake)
list(APPEND CMAKE_INCLUDE_PATH ${SIMH_INCLUDE_PATH_LIST})
list(APPEND CMAKE_MODULE_PATH ${SIMH_INCLUDE_PATH_LIST})

# Places where CMake should look for dependent package configuration fragments and artifacts:
set(SIMH_PREFIX_PATH_LIST)
list(APPEND SIMH_PREFIX_PATH_LIST ${SIMH_DEP_TOPDIR})
list(APPEND CMAKE_PREFIX_PATH ${SIMH_PREFIX_PATH_LIST})

##-- Options --##
option(ALL_IN_ONE
       "Use the 'all-in-one' project definitions (vs. individual CMakeLists.txt) (def: disabled)"
       FALSE)
option(WITH_ASYNC
       "Enable (=1)/disable (=0) Asynchronous I/O and threading."
       TRUE)
option(WITH_REGEX
       "Enable (=1)/disable (=0) PCRE/PCRE2 regular expression support (def: enabled)"
       TRUE)
option(PREFER_PCRE
       "Prefer (=1)/ignore (=0) Prefer PCRE over PCRE2 (def: ignore)"
       TRUE)
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
option(WITH_VIDEO
       "Enable (=1)/disable (=0) simulator display and graphics support (def: enabled)"
       TRUE)
option(PANDA_LIGHTS
       "Enable (=1)/disable (=0) KA-10/KI-11 simulator's Panda display. (def: disabled)"
       FALSE)
option(DONT_USE_ROMS
       "Enable (=1)/disable (=0) building support ROMs. (def: disabled)"
       FALSE)
option(ENABLE_CPPCHECK
       "Enable (=1)/disable (=0) 'cppcheck' static code analysis. (def: disabled.)"
       FALSE)

## Additional command line arguments for dependencies. Need this because Powershell will report
## an error if anything is sent to stderr and $ErrorDefaultAction is set to "Stop".
set(DEP_CMAKE_ARGS "-Wno-dev" "--no-warn-unused-cli")

## build-stage directory hierarchy for dependency installs:
if (NOT EXISTS ${SIMH_DEP_TOPDIR})
    message(STATUS "Creating dependency library directory hierarchy")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SIMH_DEP_TOPDIR} ${SIMH_DEP_TOPDIR}/include ${SIMH_DEP_TOPDIR}/lib 
                    ${SIMH_DEP_TOPDIR}/bin
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif ()

## Default install location is ${CMAKE_SOURCE_DIR}/BIN if not otherwise set
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/" CACHE PATH "${CMAKE_SOURCE_DIR} default install prefix" FORCE)
  set(CMAKE_INSTALL_BINDIR BIN CACHE PATH "BIN default binary install directory" FORCE)

  if (CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_INSTALL_BINDIR "BIN/Win32/$<CONFIG>" CACHE PATH "BIN/Win32/<config> default binary install directory" FORCE)
  endif ()
endif()

# Make sure we can include and link from the top level source...
include_directories(${CMAKE_SOURCE_DIR} ${SIMH_DEP_TOPDIR}/include)
link_directories(${SIMH_DEP_TOPDIR}/lib)

set(EXTRA_CFLAGS "")

# A bunch of Windows customizations
if (WIN32)
    list(APPEND CMAKE_LIBRARY_PATH ${SIMH_DEP_TOPDIR}/lib)
    list(APPEND CMAKE_INCLUDE_PATH ${SIMH_DEP_TOPDIR}/include)

    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

    if (MSVC_VERSION GREATER_EQUAL 1920)
        add_compile_definitions(USE_ISO_C99_NAMES)
    endif ()
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # For some reason, the MSVC solution builds as 32-bit, but none of the
    # *nix platforms do.
    #
    # If 32-bit compiles have to be added back, uncomment the following 2 lines:
    #
    # add_compile_options("-m32")
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
endif (WIN32)

# Tweak compiler flags
if (MSVC)
    list(APPEND EXTRA_CFLAGS "$<$<CONFIG:Debug>:/W3>" "$<$<CONFIG:Debug>:/w34744>" "$<$<CONFIG:Debug>:/w34777>")
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /verbose:lib")
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # include(fpintrin)

    # Turn on warnings about strict overflow/potential overflows.
    ## LIST(APPEND EXTRA_CFLAGS "-Wall" "-fno-inline" "-fstrict-overflow" "-Wstrict-overflow=3")
    LIST(APPEND EXTRA_CFLAGS "-Wformat" "-U__STRICT_ANSI__")

    # 'O3' optimization and strict overflow cause all kinds of simulator issues, especially inside
    # the VAX simulators. Reduce optimization and ensure strict overflow is turned off.
    string(REGEX REPLACE "-O3" "-O2" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    string(REGEX REPLACE "-O3" "-O2" CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")

    foreach (opt_flag "-finline-functions" "-fgcse-after-reload" "-fpredictive-commoning"
                      "-fipa-cp-clone" "-fno-unsafe-loop-optimizations" "-fno-strict-overflow")
        string(REGEX REPLACE "${opt_flag}[ \t\r\n]*" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
        string(APPEND CMAKE_C_FLAGS_RELEASE " ${opt_flag}")
        string(REGEX REPLACE "${opt_flag}[ \t\r\n]*" "" CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
        string(APPEND CMAKE_C_FLAGS_MINSIZEREL " ${opt_flag}")
    endforeach ()
endif (MSVC)

## CMake policy tweaks
if (POLICY CMP0135)
    ## Until I figure out a way to add DOWNLOAD_EXTRACT_TIMESTAMP to the call to
    ## ExternalProject_Add cleanly..
    cmake_policy(SET CMP0135 NEW)
endif()

# Find packages, arrange for dependency download/compile/install:
#
# SIMH_BUILD_DEPS is the list of the dependecies' names, for pretty-printing.
# SIMH_DEP_TARGETS is the list of dependency targets' names that we'll actually build.

set(SIMH_BUILD_DEPS)
set(SIMH_DEP_TARGETS)

set(THREADING_PKG_STATUS "unknown")
set(ZLIB_PKG_STATUS "unknown")
set(PCRE_PKG_STATUS "unknown")
set(VIDEO_PKG_STATUS)
set(NETWORK_PKG_STATUS)
set(NCURSES_PKG_STATUS "unknown")

include(git-commit-id)
include(build_dep_matrix)
include(os-features)
include(dep-locate)
include(dep-targets)

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

get_target_property(OS_FEATURE_DEFS os_features INTERFACE_COMPILE_DEFINITIONS)
string(REPLACE ";" ", " OS_FEATURE_DEFS "${OS_FEATURE_DEFS}")
if (NOT OS_FEATURE_DEFS)
  set(OS_FEATURE_DEFS "no operating system-specific features")
endif (NOT OS_FEATURE_DEFS)
get_target_property(OS_FEATURE_LIBS os_features INTERFACE_LINK_LIBRARIES)
string(REPLACE ";" ", " OS_FEATURE_LIBS "${OS_FEATURE_LIBS}")
if (NOT OS_FEATURE_LIBS)
  set(OS_FEATURE_LIBS "no operating system-specific libraries")
endif (NOT OS_FEATURE_LIBS)

set(CPPCHECK_STATUS "disabled.")

find_program(cppcheck_cmd NAMES cppcheck)
if (cppcheck_cmd)
    list(APPEND cppcheck_cmd
         "--language=c"
         "--enable=warning,style,performance,portability,information,missingInclude"
         "--suppress=missingIncludeSystem"
         "--inline-suppr"
         "--std=c99"
         "--force")
    set(CPPCHECK_STATUS "enabled.")
    if (WIN32)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            list(APPEND cppcheck_cmd
                 "--platform=win64")
            set(CPPCHECK_STATUS "enabled, Win64 platform.")
        else ()
            list(APPEND cppcheck_cmd
                 "--platform=win32A")
            set(CPPCHECK_STATUS "enabled, Win32 ASCII platform.")
        endif ()
    endif ()
else (cppcheck_cmd)
    set(CPPCHECK_STATUS "'cppcheck' not installed.")
endif ()

message(STATUS "")
message(STATUS "")
message(STATUS "Libraries and features:")
message(STATUS "  Thread support ................... ${THREADING_PKG_STATUS}")
message(STATUS "  zlib ............................. ${ZLIB_PKG_STATUS}")
message(STATUS "  Perl-Compatible RegExps........... ${PCRE_PKG_STATUS}")
message(STATUS "  PNG, Freetype, SDL2, SDL2_ttf .... ${VIDEO_PKG_STATUS}")
message(STATUS "  Packet Capture (PCAP) ............ ${NETWORK_PKG_STATUS}")
message(STATUS "  ncurses .......................... ${NCURSES_PKG_STATUS}")
message(STATUS "  OS Feature definitions ........... ${OS_FEATURE_DEFS}")
message(STATUS "  OS Feature libraries ............. ${OS_FEATURE_LIBS}")
message(STATUS "")
message(STATUS "Build options:")
message(STATUS "  Build with video/graphics support. ${BUILD_WITH_VIDEO}")
message(STATUS "  Build with networking support .... ${BUILD_WITH_NETWORK}")
if (DONT_SET_ROMS)
    set(ROM_STATUS "No")
else ()
    set(ROM_STATUS "Yes")
endif ()
message(STATUS "  Build internal ROMS .............. ${ROM_STATUS}")
message(STATUS "  'cppcheck' ....................... ${CPPCHECK_STATUS}")
message(STATUS "")

unset(ROM_STATUS)

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

    set(Python_ADDITIONAL_VERSIONS 3)
    include(FindPythonInterp)
    if (PYTHONINTERP_FOUND AND PYTHON_VERSION_MAJOR GREATER_EQUAL 3)
        add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/cmake/simh-simulators.cmake
                                  ${CMAKE_SOURCE_DIR}/cmake/simh_makefile.cmake
            COMMAND ${PYTHON_EXECUTABLE} "-m" generate
            MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/makefile
            COMMENT "Updating cmake/simh-simulators.cmake and cmake/simh_makefile.cmake"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmake
            COMMAND_EXPAND_LISTS)
        message(STATUS "Added makefile update rule.")
    endif (PYTHONINTERP_FOUND AND PYTHON_VERSION_MAJOR GREATER_EQUAL 3)

    if (${CMAKE_SOURCE_DIR}/makefile IS_NEWER_THAN ${CMAKE_SOURCE_DIR}/cmake/simh-simulators.cmake OR
        ${CMAKE_SOURCE_DIR}/makefile IS_NEWER_THAN ${CMAKE_SOURCE_DIR}/cmake/simh_makefile.cmake)
        if (NOT PYTHONINTERP_FOUND OR PYTHON_VERSION_MAJOR LESS 3)
            if (NOT PYTHONINTERP_FOUND)
                message("!! Python not found")
            elseif (PYTHON_VERSION_MAJOR LESS 3)
                message("!! Python is not python3")
            endif (NOT PYTHONINTERP_FOUND)

            message("!!")
            message("!! Cannot update cmake/simh-simulators.cmake, using older version.")
            message("!! Proceed with caution: This build might not succeed.")
            message("!!")
            message(STATUS "Updating cmake/simh-simulators.cmake and cmake/simh_makefile.cmake")
        else (NOT PYTHONINTERP_FOUND OR PYTHON_VERSION_MAJOR LESS 3)
            message(STATUS "Updating cmake/simh-simulators.cmake and cmake/simh_makefile.cmake")
            execute_process(COMMAND ${PYTHON_EXECUTABLE} "-m" generate
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cmake)
        endif (NOT PYTHONINTERP_FOUND OR PYTHON_VERSION_MAJOR LESS 3)
    endif (${CMAKE_SOURCE_DIR}/makefile IS_NEWER_THAN ${CMAKE_SOURCE_DIR}/cmake/simh-simulators.cmake OR
           ${CMAKE_SOURCE_DIR}/makefile IS_NEWER_THAN ${CMAKE_SOURCE_DIR}/cmake/simh_makefile.cmake)

    message(STATUS "Adding simulators")

    if (NOT ALL_IN_ONE)
        include(simh-simulators)
    else (NOT ALL_IN_ONE)
        include(simh_makefile)
    endif (NOT ALL_IN_ONE)

    # Install the DLLs... including Packet.DLL
    if (WIN32 AND EXISTS ${SIMH_DEP_TOPDIR}/bin)
        file(GLOB SIMH_DLLS ${SIMH_DEP_TOPDIR}/bin/*.dll)
        install(FILES ${SIMH_DLLS}
            DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}
            COMPONENT RUNTIME
        )
    endif (WIN32 AND EXISTS ${SIMH_DEP_TOPDIR}/bin)
else (NOT DO_DEPENDENCY_BUILD)
    message(STATUS "")
    message(STATUS "Building the following dependency libraries:")
    message(STATUS "")
    message(STATUS "  ${SIMH_BUILD_DEPS}")
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
            ${CMAKE_CURRENT_BINARY_DIR}
        USES_TERMINAL_CONFIGURE TRUE
        USES_TERMINAL_BUILD     TRUE
        ${USE_DOWNLOAD_EXTRACT_TIMESTAMP}
    )

    ## Ensure that the next build rechecks the dependency
    ## libraries that were just built and finds them.
    unset(FREETYPE_FOUND CACHE)
    unset(PCAP_FOUND CACHE)
    unset(PCRE_FOUND CACHE)
    unset(PCRE2_FOUND CACHE)
    unset(PNG_FOUND CACHE)
    unset(PTW_FOUND CACHE)
    unset(SDL2_FOUND CACHE)
    unset(SDL2_ttf_FOUND CACHE)
    unset(ZLIB_FOUND CACHE)
endif (NOT DO_DEPENDENCY_BUILD)
