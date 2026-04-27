##+=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
## vcpkg setup for MSVC. MinGW builds should use 'pacman' to install
## required dependency libraries.
##-=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

## If vcpkg isn't a submodule or the user didn't specify $VCPKG_ROOT, then
## we're not setting up vcpkg.
if (NOT "vcpkg" IN_LIST SIMH_GIT_SUBMODS AND NOT DEFINED ENV{VCPKG_ROOT})
    set(USING_VCPKG FALSE)
    return ()
endif ()

##-=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
## vcpkg sanity checking: Cannot use vcpkg and XP toolkit together. If this is
## a XP build, disable vcpkg:
##-=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
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
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(SIMH_CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
else ()
    ## Use the user's provided toolchain file, but load it later.
    message(STATUS "Deferring CMAKE_TOOLCHAIN_FILE load.")
    set(SIMH_CMAKE_TOOLCHAIN_FILE "${CMAKE_TOOLCHAIN_FILE}")
    unset(CMAKE_TOOLCHAIN_FILE)
    unset(CMAKE_TOOLCHAIN_FILE CACHE)
endif()

file(TO_CMAKE_PATH "${SIMH_CMAKE_TOOLCHAIN_FILE}" SIMH_CMAKE_TOOLCHAIN_FILE)
message(STATUS "SIMH_CMAKE_TOOLCHAIN_FILE is ${SIMH_CMAKE_TOOLCHAIN_FILE}")

## Target triplet: arch-platform{-runtime}, e.g., x86-windows-static-md or x64-windows
if (NOT DEFINED VCPKG_TARGET_TRIPLET)
    if (DEFINED ENV{VCPKG_DEFAULT_TRIPLET})
        ## User has a target triplet in mind, so use it.
        set(VCPKG_TARGET_TRIPLET ENV{VCPKG_DEFAULT_TRIPLET})
    else ()
        ## Set the target triplet:
        if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
            ## Default to x64, unless otherwise directed:
            set(SIMH_VCPKG_ARCH "x64")
            if(CMAKE_GENERATOR_PLATFORM MATCHES "[Ww][Ii][Nn]32")
                set(SIMH_VCPKG_ARCH "x86")
            elseif(CMAKE_GENERATOR_PLATFORM MATCHES "[Aa][Rr][Mm]64")
                set(SIMH_VCPKG_ARCH "arm64")
            elseif(CMAKE_GENERATOR_PLATFORM MATCHES "[Aa][Rr][Mm]")
                set(SIMH_VCPKG_ARCH "arm")
            endif()

            if (WIN32 AND (CMAKE_C_COMPILER_ID STREQUAL "MSVC" OR CMAKE_C_COMPILER_ID STREQUAL "Clang"))
                set(SIMH_VCPKG_PLATFORM "windows")
                ## "static-md": The triplet where the MS CRT is dynamically linked. "static" is the
                ## community triplet's runtime where the MS CRT is static. Preferentially, we use
                ## the dynamic MS CRT.
                set(SIMH_VCPKG_RUNTIME "static-md")
            elseif (MINGW OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
                set(SIMH_VCPKG_PLATFORM "mingw")
                set(SIMH_VCPKG_RUNTIME "dynamic")
            endif ()
        elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
            if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "aarch64")
                set(SIMH_VCPKG_ARCH "arm64")
            else ()
                set(SIMH_VCPKG_ARCH "x64")
            endif ()

            set (SIMH_VCPKG_PLATFORM "linux")
        else ()
            message(FATAL_ERROR "Could not determine VCPKG platform and system triplet."
                "\n"
                "(a) Are you sure that VCPKG is usable on this system? Check VCPKG_ROOT and ensure that"
                "you have properly boostrapped VCPKG."
                "\n"
                "(b) If VCPKG is not usable on this system, unset the VCPKG_ROOT environment variable.")
        endif ()

        ## Set the default triplet in the environment; older vcpkg installs on
        ## appveyor don't necessarily support the "--triplet" command line argument.
        set(use_triplet "${SIMH_VCPKG_ARCH}-${SIMH_VCPKG_PLATFORM}")
        if (SIMH_VCPKG_RUNTIME)
            string(APPEND use_triplet "-${SIMH_VCPKG_RUNTIME}")
        endif ()

        set(VCPKG_TARGET_TRIPLET "${use_triplet}" CACHE STRING "Vcpkg target triplet (ex. x86-windows)" FORCE)
        unset(use_triplet)

        set(ENV{VCPKG_DEFAULT_TRIPLET} ${VCPKG_TARGET_TRIPLET})
    endif ()
endif ()

## Set VCPKG_CRT_LINKAGE to pass down so that SIMH matches the triplet's link
## environment. Otherwise, the build will get a lot of "/NODEFAULTLIB" warnings.
if (VCPKG_TARGET_TRIPLET MATCHES ".*-static$")
    set(VCPKG_CRT_LINKAGE "static")
else ()
    set(VCPKG_CRT_LINKAGE "dynamic")
endif ()

message(STATUS "Executing deferred vcpkg toolchain initialization.\n"
    "    .. VCPKG target triplet is ${VCPKG_TARGET_TRIPLET}\n"
    "    .. VCPKG_CRT_LINKAGE is ${VCPKG_CRT_LINKAGE}")

## Initialize vcpkg after CMake detects the compiler and we've to set the platform triplet.
## VCPKG_INSTALL_OPTIONS are additional args to 'vcpkg install'. Don't need to see the
## usage instructions each time...
if (NOT ("--no-print-usage" IN_LIST VCPKG_INSTALL_OPTIONS))
    list(APPEND VCPKG_INSTALL_OPTIONS
        "--no-print-usage"
    )
endif ()

include(${SIMH_CMAKE_TOOLCHAIN_FILE})
