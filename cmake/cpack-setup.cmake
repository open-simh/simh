## CPack setup -- sets the CPACK_* variables for the sundry installers
##
## Author: B. Scott Michel (scooter.phd@gmail.com)
## "scooter me fecit"


## pre_runtime_exclusions: These are the names of dependency libraries, esp. on Windows
## that should not get installed as runtime or library dependencies.
##
## post_runtime_exclusions: These are regex expressions for the dependency paths to filter out,
## notably Windows system32 DLLs.
set(pre_runtime_exclusions)
list(APPEND pre_runtime_exclusions
    ## Windows:
    "(ext|api)-ms-.*"
    "hvsifiletrust.dll"
    "pdmutilities.dll"
)
set(post_runtime_exclusions)
list(APPEND post_runtime_exclusions
    ".*system32/.*\\.dll"
)

## Make runtime_support the default component (vice "Unspecified")
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "runtime_support")

# After we know where everything will install, let CPack figure out
# how to assemble it into a package file.
set(CPACK_PACKAGE_VENDOR "The Open-SIMH project")

if (SIMH_PACKAGE_SUFFIX)
    set(buildSuffix "${SIMH_PACKAGE_SUFFIX}")
else ()
    set(buildSuffix "")
    if (WIN32)
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            list(APPEND buildSuffix "win64")
        else ()
            list(APPEND buildSuffix "win32")
        endif ()

        list(APPEND buildSuffix "\${CPACK_BUILD_CONFIG}")
        ## If using Visual Studio, append the compiler and toolkit:
        if (CMAKE_GENERATOR MATCHES "Visual Studio 17 .*")
            list(APPEND buildSuffix "vs2022")
        elseif (CMAKE_GENERATOR MATCHES "Visual Studio 16 .*")
            list(APPEND buildSuffix "vs2019")
        elseif (CMAKE_GENERATOR MATCHES "Visual Studio 15 .*")
            list(APPEND buildSuffix "vs2017")
        elseif (CMAKE_GENERATOR MATCHES "Visual Studio 14 .*")
            list(APPEND buildSuffix "vs2015")
        endif ()
        if (CMAKE_GENERATOR_TOOLSET MATCHES "v[0-9][0-9][0-9]_xp")
            string(APPEND buildSuffix "xp")
        endif ()
    else ()
        list(APPEND buildSuffix ${CMAKE_SYSTEM_NAME})
    endif ()

    list(JOIN buildSuffix "-" buildSuffix)

    message(STATUS "No SIMH_PACKAGE_SUFFIX supplied, default is ${buildSuffix}.")
endif ()

string(JOIN "-" CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}"
    "${CMAKE_PROJECT_VERSION}"
    "${buildSuffix}"
)

message(STATUS "CPack output file name: ${CPACK_PACKAGE_FILE_NAME}")
unset(buildSuffix)

## When applicable (e.g., NSIS Windows), install under the SIMH-x.y directory:
set(CPACK_PACKAGE_INSTALL_DIRECTORY "SIMH-${SIMH_VERSION_MAJOR}.${SIMH_VERSION_MINOR}")
## License file:
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE.txt)

set(CPACK_PACKAGE_CONTACT    "open-simh@nowhere.org")
set(CPACK_PACKAGE_MAINTAINER "open-simh@nowhere.org")

## Runtime dependencies:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
    ## Don't install runtime dependencies on Linux platforms. The platform's
    ## package management system will take care of this for us.
    if (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        install(RUNTIME_DEPENDENCY_SET simhRuntime
            COMPONENT runtime_support
            PRE_EXCLUDE_REGEXES ${pre_runtime_exclusions}
            POST_EXCLUDE_REGEXES  ${post_runtime_exclusions}
        )
    endif ()
endif ()


## Extra properties and variables:
set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_BINARY_DIR}/CPackSimhCustom.cmake)
configure_file(${CMAKE_SOURCE_DIR}/cmake/installer-customizations/CPackSimhCustom.cmake.in
    ${CMAKE_BINARY_DIR}/CPackSimhCustom.cmake
    @ONLY)

## CPack generator-specific configs:

##+
## NullSoft Installation System (NSIS) Windows installer. Creates an installer EXE.
##-
set(CPACK_NSIS_PACKAGE_NAME ${CPACK_PACKAGE_INSTALL_DIRECTORY})
set(CPACK_NSIS_INSTALL_ROOT "$LocalAppData\\\\Programs")

## CPack does this configure_file on its own to genreate the project.nsi file.
## Keeping these lines for history.
# configure_file(${CMAKE_SOURCE_DIR}/cmake/installer-customizations/NSIS.template.in
#     ${CMAKE_BINARY_DIR}/NSIS.template
#     @ONLY)

###+
### WIX MSI Windows installer.
###
###
### Upgrade GUID shouldn't really change.
###-
set(CPACK_WIX_UPGRADE_GUID "ed5dba4c-7c9e-4af8-ac36-37e14c637696")

##+
## Debian:
##-

list(APPEND debian_depends
    libsdl2-2.0-0
    libsdl2-ttf-2.0-0
    libpcap0.8
    libvdeplug2
    libedit2
)

string(JOIN ", " CPACK_DEBIAN_PACKAGE_DEPENDS ${debian_depends})


include(CPack)
