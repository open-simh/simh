##=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
## dep-locate.cmake
##
## Consolidated list of runtime dependencies for simh, probed/found via
## CMake's find_package() and pkg_check_modules() when 'pkgconfig' is
## available.
##=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

##-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~
## Find packages:
##-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~

if (WITH_REGEX)
    if (PREFER_PCRE)
        if (USING_VCPKG)
            find_package(unofficial-pcre CONFIG)
        else ()
            ## LEGACY strategy:
            find_package(PCRE)
        endif ()
    else ()
        ## There isn't a difference between the vcpkg and LEGACY strategy.
        ## SIMH provides its own FindPCRE2.cmake module...
        find_package(PCRE2)
    endif ()
endif ()

if (WITH_REGEX OR WITH_VIDEO)
    set(ZLIB_USE_STATIC_LIBS ON)
    find_package(ZLIB)
endif ()

if (WITH_VIDEO)
    if (NOT USING_VCPKG)
        ## LEGACY strategy:
        find_package(PNG)
        find_package(Freetype)
        find_package(SDL2 NAMES sdl2 SDL2)
        find_package(SDL2_ttf NAMES sdl2_ttf SDL2_ttf)
    else ()
        ## vcpkg strategy:
        find_package(PNG REQUIRED)
        find_package(SDL2 CONFIG)
        find_package(SDL2_ttf CONFIG)
    endif ()
endif ()

if (WITH_NETWORK)
    if (WITH_VDE)
        find_package(VDE)
    endif ()

    ## pcap is special: Headers only and dynamically loaded.
    if (WITH_PCAP)
        find_package(PCAP)
    endif (WITH_PCAP)
endif (WITH_NETWORK)

if (NOT WIN32 OR MINGW)
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
        if (WITH_REGEX)
            if (PREFER_PCRE AND NOT PCRE_FOUND)
                pkg_check_modules(PCRE IMPORTED_TARGET libpcre)
            elseif (NOT PREFER_PCRE AND NOT PCRE2_FOUND)
                pkg_check_modules(PCRE IMPORTED_TARGET libpcre2-8)
            endif ()
        endif (WITH_REGEX)

        if (WITH_REGEX OR WITH_VIDEO)
            if (NOT ZLIB_FOUND)
                pkg_check_modules(ZLIB IMPORTED_TARGET zlib)
            endif ()
        endif ()

        if (WITH_VIDEO)
            if (NOT PNG_FOUND)
                pkg_check_modules(PNG IMPORTED_TARGET libpng16)
            endif ()
            if (NOT SDL2_FOUND)
                pkg_check_modules(SDL2 IMPORTED_TARGET sdl2)
                if (NOT SDL2_FOUND)
                    pkg_check_modules(SDL2 IMPORTED_TARGET SDL2)
                endif ()
            endif ()

            if (NOT SDL2_ttf_FOUND)
                pkg_check_modules(SDL2_ttf IMPORTED_TARGET SDL2_ttf)
                if (NOT SDL2_ttf_FOUND)
                    pkg_check_modules(SDL2_ttf IMPORTED_TARGET sdl2_ttf)
                endif ()
            endif ()
        endif (WITH_VIDEO)

        if (WITH_NETWORK)
            if (WITH_VDE AND NOT VDE_FOUND)
                pkg_check_modules(VDE IMPORTED_TARGET vdeplug)
            endif ()
        endif (WITH_NETWORK)
    endif ()
endif ()

##-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~
## Add rules for the superbuild if dependencies need to be built:
##-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~

if (NO_DEP_BUILD)
    ## Not going to build dependencies...
    return ()
endif ()

include (ExternalProject)

# Source URLs (to make it easy to update versions):
set(ZLIB_SOURCE_URL     "https://github.com/madler/zlib/archive/v1.2.13.zip")
set(PCRE2_SOURCE_URL    "https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.42/pcre2-10.42.zip")
## PCRE needs multiple URLs to chase a working SF mirror:
list(APPEND PCRE_SOURCE_URL
    "https://sourceforge.net/projects/pcre/files/pcre/8.45/pcre-8.45.zip/download?use_mirror=cytranet"
    "https://sourceforge.net/projects/pcre/files/pcre/8.45/pcre-8.45.zip/download?use_mirror=phoenixnap"
    "https://sourceforge.net/projects/pcre/files/pcre/8.45/pcre-8.45.zip/download?use_mirror=versaweb"
    "https://sourceforge.net/projects/pcre/files/pcre/8.45/pcre-8.45.zip/download?use_mirror=netactuate"
    "https://sourceforge.net/projects/pcre/files/pcre/8.45/pcre-8.45.zip/download?use_mirror=cfhcable"
    "https://sourceforge.net/projects/pcre/files/pcre/8.45/pcre-8.45.zip/download?use_mirror=freefr"
    "https://sourceforge.net/projects/pcre/files/pcre/8.45/pcre-8.45.zip/download?use_mirror=master"
)
set(PNG_SOURCE_URL      "https://github.com/glennrp/libpng/archive/refs/tags/v1.6.40.tar.gz")
## Freetype also needs multiple URLs to chase a working mirror:
list(APPEND FREETYPE_SOURCE_URL
    "https://github.com/freetype/freetype/archive/refs/tags/VER-2-13-0.zip"
    "https://sourceforge.net/projects/freetype/files/freetype2/2.13.1/ft2131.zip/download?use_mirror=cytranet"
    "https://sourceforge.net/projects/freetype/files/freetype2/2.13.1/ft2131.zip/download?use_mirror=phoenixnap"
    "https://sourceforge.net/projects/freetype/files/freetype2/2.13.1/ft2131.zip/download?use_mirror=versaweb"
    "https://sourceforge.net/projects/freetype/files/freetype2/2.13.1/ft2131.zip/download?use_mirror=netactuate"
    "https://sourceforge.net/projects/freetype/files/freetype2/2.13.1/ft2131.zip/download?use_mirror=cfhcable"
    "https://sourceforge.net/projects/freetype/files/freetype2/2.13.1/ft2131.zip/download?use_mirror=freefr"
    "https://sourceforge.net/projects/freetype/files/freetype2/2.13.1/ft2131.zip/download?use_mirror=master"
    "https://download.savannah.gnu.org/releases/freetype/freetype-2.13.1.tar.xz"
    "https://gitlab.freedesktop.org/freetype/freetype/-/archive/VER-2-13-0/freetype-VER-2-13-0.zip"
)
set(SDL2_SOURCE_URL     "https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.28.1.zip")
set(SDL2_TTF_SOURCE_URL "https://github.com/libsdl-org/SDL_ttf/archive/refs/tags/release-2.20.2.zip")

## Need to build ZLIB for both PCRE and libpng16:
if ((WITH_REGEX OR WITH_VIDEO) AND NOT ZLIB_FOUND)
    ExternalProject_Add(zlib-dep
        URL ${ZLIB_SOURCE_URL}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        ## These patches come from vcpkg so that only the static libraries are built and
        ## installed. If the patches don't apply cleanly (and there's a build error), that
        ## means a version number got bumped and need to see what patches, if any, are
        ## still applicable.
        PATCH_COMMAND
            git -c core.longpaths=true -c core.autocrlf=false --work-tree=. --git-dir=.git
            apply
                "${SIMH_DEP_PATCHES}/zlib/0001-Prevent-invalid-inclusions-when-HAVE_-is-set-to-0.patch"
                "${SIMH_DEP_PATCHES}/zlib/0002-skip-building-examples.patch"
                "${SIMH_DEP_PATCHES}/zlib/0003-build-static-or-shared-not-both.patch"
                "${SIMH_DEP_PATCHES}/zlib/0004-android-and-mingw-fixes.patch"
                --ignore-whitespace --whitespace=nowarn --verbose
    )

    BuildDepMatrix(zlib-dep zlib CMAKE_ARGS -DBUILD_SHARED_LIBS:Bool=${BUILD_SHARED_DEPS})

    list(APPEND SIMH_BUILD_DEPS zlib)
    list(APPEND SIMH_DEP_TARGETS zlib-dep)
    message(STATUS "Building ZLIB from ${ZLIB_SOURCE_URL}.")
    set(ZLIB_PKG_STATUS "ZLIB source build")
endif ()

IF (WITH_REGEX AND NOT (PCRE_FOUND OR PCRE2_FOUND OR TARGET unofficial::pcre::pcre))
    set(PCRE_DEPS)
    IF (TARGET zlib-dep)
        list(APPEND PCRE_DEPS zlib-dep)
    ENDIF (TARGET zlib-dep)

    set(PCRE_CMAKE_ARGS -DBUILD_SHARED_LIBS:Bool=${BUILD_SHARED_DEPS})
    if (NOT PREFER_PCRE)
        set(PCRE_URL ${PCRE2_SOURCE_URL})
        list(APPEND PCRE_CMAKE_ARGS 
            -DPCRE2_BUILD_PCRE2_8:Bool=On
            -DPCRE2_BUILD_PCRE2GREP:Bool=Off
            -DPCRE2_STATIC_RUNTIME:Bool=On
            -DPCRE2_SUPPORT_LIBEDIT:Bool=Off
            -DPCRE2_SUPPORT_LIBREADLINE:Bool=Off
        )
        if (WIN32)
            list(APPEND PCRE_CMAKE_ARGS
                -DBUILD_STATIC_LIBS:Bool=On
            )
        endif ()

        # IF(MSVC)
        #   list(APPEND PCRE_CMAKE_ARGS -DINSTALL_MSVC_PDB=On)
        # ENDIF(MSVC)

        message(STATUS "Building PCRE2 from ${PCRE_URL}")
        set(PCRE_PKG_STATUS "pcre2 source build")
    ELSE ()
        set(PCRE_URL ${PCRE_SOURCE_URL})
        list(APPEND PCRE_CMAKE_ARGS
            -DPCRE_BUILD_PCREGREP:Bool=Off
            -DPCRE_SUPPORT_LIBEDIT:Bool=Off
            -DPCRE_SUPPORT_LIBREADLINE:Bool=Off
        )
        if (WIN32)
            list(APPEND PCRE_CMAKE_ARGS
                -DBUILD_SHARED_LIBS:Bool=Off
                -DPCRE_STATIC_RUNTIME:Bool=On
            )
        endif ()

        message(STATUS "Building PCRE from ${PCRE_URL}")
        set(PCRE_PKG_STATUS "pcre source build")
    ENDIF ()

    ExternalProject_Add(pcre-ext
        URL
            ${PCRE_URL}
        DEPENDS
            ${PCRE_DEPS}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
    )

    BuildDepMatrix(pcre-ext pcre CMAKE_ARGS ${PCRE_CMAKE_ARGS})

    list(APPEND SIMH_BUILD_DEPS pcre)
    list(APPEND SIMH_DEP_TARGETS pcre-ext)
ELSE ()
  set(PCRE_PKG_STATUS "regular expressions disabled")
ENDIF ()

set(BUILD_WITH_VIDEO FALSE)
IF (WITH_VIDEO)
    IF (NOT PNG_FOUND)
        set(PNG_DEPS)
        if (NOT ZLIB_FOUND)
            list(APPEND PNG_DEPS zlib-dep)
        endif (NOT ZLIB_FOUND)

        ExternalProject_Add(png-dep
            URL
                ${PNG_SOURCE_URL}
            DEPENDS
                ${PNG_DEPS}
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
        )

        ## Work around the GCC 8.1.0 SEH index regression.
        set(PNG_CMAKE_BUILD_TYPE_RELEASE "Release")
        if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND
            CMAKE_C_COMPILER_VERSION VERSION_EQUAL "8.1" AND
            NOT CMAKE_BUILD_VERSION)
            message(STATUS "PNG: Build using MinSizeRel CMAKE_BUILD_TYPE with GCC 8.1")
            set(PNG_CMAKE_BUILD_TYPE_RELEASE "MinSizeRel")
        endif()

        BuildDepMatrix(png-dep libpng
            CMAKE_ARGS
                -DPNG_SHARED:Bool=${BUILD_SHARED_DEPS}
                -DPNG_STATUS:Bool=On
                -DPNG_EXECUTABLES:Bool=Off
                -DPNG_TESTS:Bool=Off
            RELEASE_BUILD ${PNG_CMAKE_BUILD_TYPE_RELEASE}
        )

        list(APPEND SIMH_BUILD_DEPS "png")
        list(APPEND SIMH_DEP_TARGETS "png-dep")
        message(STATUS "Building PNG from ${PNG_SOURCE_URL}")
        list(APPEND VIDEO_PKG_STATUS "PNG source build")
    ENDIF (NOT PNG_FOUND)

    IF (NOT SDL2_FOUND)
        ExternalProject_Add(sdl2-dep
            URL ${SDL2_SOURCE_URL}
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
        )

        BuildDepMatrix(sdl2-dep SDL2 CMAKE_ARGS "-DBUILD_SHARED_LIBS:Bool=${BUILD_SHARED_DEPS}")

        list(APPEND SIMH_BUILD_DEPS "SDL2")
        list(APPEND SIMH_DEP_TARGETS "sdl2-dep")
        message(STATUS "Building SDL2 from ${SDL2_SOURCE_URL}.")
        list(APPEND VIDEO_PKG_STATUS "SDL2 source build")
    ENDIF (NOT SDL2_FOUND)

    IF (NOT FREETYPE_FOUND)
        set(FREETYPE_DEPS)
        if (TARGET zlib-dep)
            list(APPEND FREETYPE_DEPS zlib-dep)
        endif ()
        if (TARGET png-dep)
            list(APPEND FREETYPE_DEPS png-dep)
        endif ()

        ExternalProject_Add(freetype-dep
            URL
                ${FREETYPE_SOURCE_URL}
            DEPENDS
                ${FREETYPE_DEPS}
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
        )

        BuildDepMatrix(freetype-dep Freetype 
            CMAKE_ARGS
                "-DBUILD_SHARED_LIBS:Bool=${BUILD_SHARED_DEPS}"
                "-DFT_DISABLE_BZIP2:Bool=TRUE"
                "-DFT_DISABLE_HARFBUZZ:Bool=TRUE"
                "-DFT_DISABLE_BROTLI:Bool=TRUE"
        )

        list(APPEND SIMH_BUILD_DEPS "Freetype")
        list(APPEND SIMH_DEP_TARGETS freetype-dep)
        message(STATUS "Building Freetype from ${FREETYPE_SOURCE_URL}.")
    ENDIF ()

    IF (NOT SDL2_ttf_FOUND)
        set(SDL2_ttf_DEPS)
        if (TARGET sdl2-dep)
            list(APPEND SDL2_ttf_DEPS sdl2-dep)
        endif (TARGET sdl2-dep)
        if (TARGET freetype-dep)
            list(APPEND SDL2_ttf_DEPS freetype-dep)
        endif ()

        ExternalProject_Add(sdl2-ttf-dep
            URL
                ${SDL2_TTF_SOURCE_URL}
            DEPENDS
                ${SDL2_ttf_DEPS}
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
            PATCH_COMMAND
                git -c core.longpaths=true -c core.autocrlf=false --work-tree=. --git-dir=.git
                apply
                    "${SIMH_DEP_PATCHES}/SDL_ttf/fix-pkgconfig.patch"
                    --ignore-whitespace --whitespace=nowarn --verbose
        )

        set(sdl2_ttf_cmake_args)
        list(APPEND sdl2_ttf_cmake_args
                "-DBUILD_SHARED_LIBS:Bool=${BUILD_SHARED_DEPS}"
                "-DSDL2TTF_SAMPLES:Bool=Off"
                "-DSDL2TTF_VENDORED:Bool=Off"
                "-DSDL2TTF_HARFBUZZ:Bool=Off"
        )

        BuildDepMatrix(sdl2-ttf-dep SDL2_ttf CMAKE_ARGS ${sdl2_ttf_cmake_args})

        list(APPEND SIMH_BUILD_DEPS "SDL2_ttf")
        list(APPEND SIMH_DEP_TARGETS "sdl2-ttf-dep")
        message(STATUS "Building SDL2_ttf from https://www.libsdl.org/release/SDL2_ttf-2.0.15.zip.")
        list(APPEND VIDEO_PKG_STATUS "SDL2_ttf source build")
    ENDIF (NOT SDL2_ttf_FOUND)

    set(BUILD_WITH_VIDEO TRUE)
ELSE ()
    set(VIDEO_PKG_STATUS "video support disabled")
ENDIF(WITH_VIDEO)
