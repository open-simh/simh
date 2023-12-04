##+=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~
## vcpkg setup for MSVC. MinGW builds should use 'pacman' to install
## required dependency libraries.
##-=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~

if (NOT USING_VCPKG)
    return ()
endif ()

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

            if (MSVC OR CMAKE_C_COMPILER_ID MATCHES ".*Clang")
                set(SIMH_VCPKG_PLATFORM "windows")
                set(SIMH_VCPKG_RUNTIME "")
                if (NOT BUILD_SHARED_DEPS)
                    set(SIMH_VCPKG_RUNTIME "static")
                endif ()
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
set(VCPKG_CRT_LINKAGE "dynamic")
if (VCPKG_TARGET_TRIPLET MATCHES ".*-static")
    set(VCPKG_CRT_LINKAGE "static")
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
