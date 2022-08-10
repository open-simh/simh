##=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
## dep-locate.cmake
##
## Consolidated list of runtime dependencies for simh, probed/found via
## CMake's find_package() and pkg_check_modules() when 'pkgconfig' is
## available.
##=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

## Note: PCRE/PCRE2 depend on finding zlib, although new versions of the PNG
## FindPackage explicitly look for zlib on their own. This creates a problem
## if we look for zlib via pkgconfig, because PNG won't pick it up (because
## PNG doesn't fall back to pkgconfig like we do.) Consequently, ZLIB won't
## get built as one of our dependencies.
##
## Solution is to look for ZLIB here via FindPackage, but don't fall back to
## pkgconfig.
find_package(ZLIB)
find_package(BZip2)

if (WITH_REGEX)
  find_package(PCRE)
  find_package(PCRE2)
endif ()

if (WITH_VIDEO)
    find_package(PNG)
    find_package(BrotliDec)
    find_package(Harfbuzz)
    find_package(Freetype)

    ## There is no FindSDL2.cmake. It ships with a CMake config file, which
    ## means that we should only find_package() in config mode.
    find_package(SDL2 CONFIG)
    ## Same thing for SDL2_ttf.
    find_package(SDL2_ttf CONFIG NAMES SDL2_ttf SDL_ttf)
endif (WITH_VIDEO)

if (WITH_NETWORK)
    if (WITH_PCAP)
        find_package(PCAP)

        if (NOT WIN32)
            ## See if pcap_compile() is present:
            set(saved_cmake_required_incs ${CMAKE_REQUIRED_INCLUDES})
            set(saved_cmake_required_libs ${CMAKE_REQUIRED_LIBRARIES})

            set(CMAKE_REQUIRED_INCLUDES ${PCAP_INCLUDE_DIRS})
            set(CMAKE_REQUIRED_LIBRARIES ${PCAP_LIBRARIES})

            check_symbol_exists(pcap_compile pcap.h HAVE_PCAP_COMPILE)

            set(CMAKE_REQUIRED_INCLUDES ${saved_cmake_required_incs})
            set(CMAKE_REQUIRED_LIBRARIES ${saved_cmake_required_libs})
        endif (NOT WIN32)
    endif (WITH_PCAP)

    find_package(VDE)
endif (WITH_NETWORK)

##=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
## pkg-config is our fallback when find_package() can't find a package.
##=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

find_package(PkgConfig)

## pkg_check_modules() should only be used with runtime dependencies that don't
## have a corresponding 'Find<package>.cmake' script.
##
## Don't use pkg-config searches with MS Visual C -- the library names are
## incompatible.
##
## [Leave the "(MSVC)" alone. Intent is to add "OR <cond>" if there are more
##  exceptions to pkg-config than just Windows.]

if (PKG_CONFIG_FOUND AND NOT (MSVC))
    if (NOT ZLIB_FOUND)
        pkg_check_modules(ZLIB IMPORTED_TARGET zlib)
    endif (NOT ZLIB_FOUND)

    if (WITH_REGEX)
        if (PREFER_PCRE AND NOT PCRE_FOUND)
            pkg_check_modules(PCRE IMPORTED_TARGET libpcre)
        elseif (NOT PREFER_PCRE AND NOT PCRE2_FOUND)
            pkg_check_modules(PCRE IMPORTED_TARGET libpcre2-8)
        endif (PREFER_PCRE AND NOT PCRE_FOUND)
    endif (WITH_REGEX)

    if (WITH_VIDEO)
        if (NOT PNG_FOUND)
            pkg_check_modules(PNG IMPORTED_TARGET libpng16)
        endif (NOT PNG_FOUND)

        if (NOT HARFBUZZ_FOUND)
            pkg_check_modules(HARFBUZZ IMPORTED_TARGET harfbuzz)
        endif (NOT HARFBUZZ_FOUND)

        if (NOT Freetype_FOUND)
            pkg_check_modules(Freetype IMPORTED_TARGET freetype2)
        endif (NOT Freetype_FOUND)

        if (NOT SDL2_FOUND)
            pkg_check_modules(SDL2 IMPORTED_TARGET sdl2)
            if (NOT SDL2_FOUND)
                pkg_check_modules(SDL2 IMPORTED_TARGET SDL2)
            endif (NOT SDL2_FOUND)
        endif (NOT SDL2_FOUND)

        IF (NOT SDL2_ttf_FOUND)
            pkg_check_modules(SDL2_ttf IMPORTED_TARGET sdl2_ttf)
            if (NOT SDL2_ttf_FOUND)
                pkg_check_modules(SDL2_ttf IMPORTED_TARGET SDL2_ttf)
            endif (NOT SDL2_ttf_FOUND)
        ENDIF (NOT SDL2_ttf_FOUND)
    endif (WITH_VIDEO)
endif (PKG_CONFIG_FOUND AND NOT (MSVC))
