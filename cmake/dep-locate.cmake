##=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
## dep-locate.cmake
##
## Consolidated list of runtime dependencies for simh, probed/found via
## CMake's find_package() and pkg_check_modules() when 'pkgconfig' is
## available.
##=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

find_package(PkgConfig)

if (NOT PKG_CONFIG_FOUND OR (WIN32))
    ## Note: PCRE/PCRE2 depend on finding zlib, although new versions of the PNG
    ## FindPackage explicitly look for zlib on their own. This creates a problem
    ## if we look for zlib via pkgconfig, because PNG won't pick it up (because
    ## PNG doesn't fall back to pkgconfig like we do.) Consequently, ZLIB won't
    ## get built as one of our dependencies.
    ##
    ## Solution is to look for ZLIB here via FindPackage, but don't fall back to
    ## pkgconfig.
    find_package(Curses)

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
        find_package(VDE)
    endif (WITH_NETWORK)
else ()
    pkg_check_modules(CURSES IMPORTED_TARGET ncurses)
    pkg_check_modules(ZLIB IMPORTED_TARGET zlib)

    if (WITH_REGEX)
        if (PREFER_PCRE)
            pkg_check_modules(PCRE IMPORTED_TARGET libpcre)
        else ()
            pkg_check_modules(PCRE IMPORTED_TARGET libpcre2-8)
        endif ()
    endif (WITH_REGEX)

    if (WITH_VIDEO)
        pkg_check_modules(PNG IMPORTED_TARGET libpng16)
        pkg_check_modules(HARFBUZZ IMPORTED_TARGET harfbuzz)
        pkg_check_modules(Freetype IMPORTED_TARGET freetype2)
        pkg_check_modules(SDL2 IMPORTED_TARGET sdl2)
        if (NOT SDL2_FOUND)
            pkg_check_modules(SDL2 IMPORTED_TARGET SDL2)
        endif (NOT SDL2_FOUND)

        pkg_check_modules(SDL2_ttf IMPORTED_TARGET SDL2_ttf)
        if (NOT SDL2_ttf_FOUND)
            pkg_check_modules(SDL2_ttf IMPORTED_TARGET sdl2_ttf)
        endif (NOT SDL2_ttf_FOUND)
    endif (WITH_VIDEO)

    if (WITH_NETWORK)
        pkg_check_modules(VDE IMPORTED_TARGET vdeplug)
    endif (WITH_NETWORK)
endif ()

## pcap is special: Headers only and dynamically loaded.
if (WITH_NETWORK)
    if (WITH_PCAP)
        find_package(PCAP)
    endif (WITH_PCAP)
endif (WITH_NETWORK)
