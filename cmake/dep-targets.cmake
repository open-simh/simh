include (ExternalProject)

add_library(zlib_lib INTERFACE)
add_library(regexp_lib INTERFACE)
add_library(simh_video INTERFACE)
add_library(simh_network INTERFACE)
add_library(simh_ncurses INTERFACE)

# Source URLs (to make it easy to update versions):
set(ZLIB_SOURCE_URL     "https://github.com/madler/zlib/archive/v1.2.11.zip")
set(PCRE2_SOURCE_URL    "https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.40/pcre2-10.40.tar.bz2")
set(PCRE_SOURCE_URL     "https://sourceforge.net/projects/pcre/files/pcre/8.45/pcre-8.45.tar.bz2/download")
set(PNG_SOURCE_URL      "https://sourceforge.net/projects/libpng/files/libpng16/1.6.37/libpng-1.6.37.tar.xz/download")
set(FREETYPE_SOURCE_URL "https://download.savannah.nongnu.org/releases/freetype/freetype-2.11.1.tar.xz")
set(SDL2_SOURCE_URL     "https://www.libsdl.org/release/SDL2-2.0.20.zip")
set(SDL2_TTF_SOURCE_URL "https://github.com/libsdl-org/SDL_ttf/archive/refs/tags/release-2.0.18.tar.gz")

## LIBPCAP is a special case
set(LIBPCAP_PROJECT "libpcap")
set(LIBPCAP_ARCHIVE_NAME "libpcap")
set(LIBPCAP_RELEASE "1.10.1")
set(LIBPCAP_ARCHIVE_TYPE "tar.gz")
set(LIBPCAP_TAR_ARCHIVE "${LIBPCAP_ARCHIVE_NAME}-${LIBPCAP_RELEASE}.${LIBPCAP_ARCHIVE_TYPE}")
set(LIBPCAP_SOURCE_URL  "https://github.com/the-tcpdump-group/libpcap/archive/refs/tags/${LIBPCAP_TAR_ARCHIVE}")
# set(LIBPCAP_SOURCE_URL  "https://github.com/the-tcpdump-group/libpcap.git")

## ncurses library:
if (TARGET PkgConfig::CURSES)
    target_link_libraries(simh_ncurses INTERFACE PkgConfig::CURSES)
    set(NCURSES_PKG_STATUS "pkg-config ncurses")
elseif (CURSES_HAVE_NCURSES_H)
    target_compile_definitions(simh_ncurses INTERFACE HAVE_NCURSES)
    target_include_directories(simh_ncurses INTERFACE "${CURSES_INCLUDE_DIRS}")
    target_link_libraries(simh_ncurses INTERFACE "${CURSES_LIBRARIES}")
    set(NCURSES_PKG_STATUS "detected ncurses")
elseif (WIN32)
    set(NCURSES_PKG_STATUS "not supported on Win32")
else ()
    set(NCURSES_PKG_STATUS "not found")
endif ()

if (ZLIB_FOUND)
    if (TARGET ZLIB::ZLIB)
        target_compile_definitions(zlib_lib INTERFACE $<TARGET_PROPERTY:ZLIB::ZLIB,INTERFACE_COMPILE_DEFINITIONS>)
        target_include_directories(zlib_lib INTERFACE $<TARGET_PROPERTY:ZLIB::ZLIB,INTERFACE_INCLUDE_DIRECTORIES>)
        target_link_libraries(zlib_lib INTERFACE ZLIB::ZLIB)
        set(ZLIB_PKG_STATUS "detected ZLIB")
    elseif (TARGET PkgConfig::ZLIB)
        target_compile_definitions(zlib_lib INTERFACE $<TARGET_PROPERTY:PkgConfig::ZLIB,INTERFACE_COMPILE_DEFINITIONS>)
        target_include_directories(zlib_lib INTERFACE $<TARGET_PROPERTY:PkgConfig::ZLIB,INTERFACE_INCLUDE_DIRECTORIES>)
        target_link_libraries(zlib_lib INTERFACE PkgConfig::ZLIB)
        set(ZLIB_PKG_STATUS "pkg-config ZLIB")
    else (TARGET ZLIB::ZLIB)
        target_include_directories(zlib_lib INTERFACE ${ZLIB_INCLUDE_DIRS})
        target_link_libraries(zlib_lib INTERFACE ${ZLIB_LIBRARIES})
        set(ZLIB_PKG_STATUS "detected ZLIB")
    endif (TARGET ZLIB::ZLIB)
else (ZLIB_FOUND)
    ExternalProject_Add(zlib-dep
        URL ${ZLIB_SOURCE_URL}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
    )

    BuildDepMatrix(zlib-dep zlib)

    list(APPEND SIMH_BUILD_DEPS zlib)
    list(APPEND SIMH_DEP_TARGETS zlib-dep)
    message(STATUS "Building ZLIB from ${ZLIB_SOURCE_URL}.")
    set(ZLIB_PKG_STATUS "ZLIB source build")
endif (ZLIB_FOUND)

## Freetype will sometimes find BZip2 in AppVeyor's image, which means that we
## need to bring it along as a dependency for AppVeyor builds. Ordinarily, though,
## it's not a dependency for SIMH.

if (BZIP2_FOUND)
    if (TARGET BZip2::BZip2)
        target_compile_definitions(zlib_lib INTERFACE $<TARGET_PROPERTY:BZip2::BZip2,INTERFACE_COMPILE_DEFINITIONS>)
        target_include_directories(zlib_lib INTERFACE $<TARGET_PROPERTY:BZip2::BZip2,INTERFACE_INCLUDE_DIRECTORIES>)
        target_link_libraries(zlib_lib INTERFACE BZip2::BZip2)
    elseif (TARGET PkgConfig::BZip2)
        target_compile_definitions(zlib_lib INTERFACE $<TARGET_PROPERTY:PkgConfig::BZip2,INTERFACE_COMPILE_DEFINITIONS>)
        target_include_directories(zlib_lib INTERFACE $<TARGET_PROPERTY:PkgConfig::BZip2,INTERFACE_INCLUDE_DIRECTORIES>)
        target_link_libraries(zlib_lib INTERFACE BZip2::BZip2)
    else (TARGET BZip2::BZip2)
        target_include_directories(zlib_lib INTERFACE ${BZIP2_INCLUDE_DIR})
        target_link_libraries(zlib_lib INTERFACE ${BZIP2_LIBRARIES})
    endif (TARGET BZip2::BZip2)
endif (BZIP2_FOUND)

IF (WITH_REGEX)
    ## TEMP: Use PCRE until patches for PCRE2 are avaiable.
    ## (Prefer PCRE2 over PCRE (unless PREFER_PCRE is set)
    IF (TARGET PkgConfig::PCRE)
        target_compile_definitions(regexp_lib INTERFACE $<TARGET_PROPERTY:PkgConfig::PCRE,INTERFACE_COMPILE_DEFINITIONS>)
        target_include_directories(regexp_lib INTERFACE $<TARGET_PROPERTY:PkgConfig::PCRE,INTERFACE_COMPILE_DEFINITIONS>)
        target_link_libraries(regexp_lib INTERFACE PkgConfig::PCRE)
        if (PREFER_PCRE)
            target_compile_definitions(regexp_lib INTERFACE HAVE_PCRE_H PCRE_STATIC)
            set(PCRE_PKG_STATUS "pkg-config pcre")
        else ()
            target_compile_definitions(regexp_lib INTERFACE HAVE_PCRE2_H PCRE2_STATIC)
            set(PCRE_PKG_STATUS "pkg-config pcre2")
        endif ()
    ELSEIF (NOT PREFER_PCRE AND PCRE2_FOUND)
        target_compile_definitions(regexp_lib INTERFACE HAVE_PCRE2_H PCRE2_STATIC)
        target_include_directories(regexp_lib INTERFACE ${PCRE2_INCLUDE_DIRS})
        target_link_libraries(regexp_lib INTERFACE ${PCRE2_LIBRARY})

        set(PCRE_PKG_STATUS "detected pcre2")
    ELSEIF (PCRE_FOUND)
        target_compile_definitions(regexp_lib INTERFACE HAVE_PCRE_H PCRE_STATIC)
        target_include_directories(regexp_lib INTERFACE ${PCRE_INCLUDE_DIRS})
        target_link_libraries(regexp_lib INTERFACE ${PCRE_LIBRARY})

        set(PCRE_PKG_STATUS "detected pcre")
    ELSE ()
        set(PCRE_DEPS)
        IF (TARGET zlib-dep)
          list(APPEND PCRE_DEPS zlib-dep)
        ENDIF (TARGET zlib-dep)

        if (NOT PREFER_PCRE)
            set(PCRE_URL ${PCRE2_SOURCE_URL})
            set(PCRE_CMAKE_ARGS 
                  -DBUILD_SHARED_LIBS:Bool=On
                  -DPCRE2_BUILD_PCREGREP:Bool=Off)
            # IF(MSVC)
            #   list(APPEND PCRE_CMAKE_ARGS -DINSTALL_MSVC_PDB=On)
            # ENDIF(MSVC)

            message(STATUS "Building PCRE2 from ${PCRE_URL}")
            set(PCRE_PKG_STATUS "pcre2 source build")
        ELSE ()
            set(PCRE_URL ${PCRE_SOURCE_URL})
            set(PCRE_CMAKE_ARGS 
                  -DBUILD_SHARED_LIBS:Bool=Off
                  -DPCRE_BUILD_PCREGREP:Bool=Off)

            message(STATUS "Building PCRE from ${PCRE_URL}")
            set(PCRE_PKG_STATUS "pcre source build")
        ENDIF ()

        ExternalProject_Add(pcre-ext
          URL ${PCRE_URL}
          DEPENDS
              ${PCRE_DEPS}
          CONFIGURE_COMMAND ""
          BUILD_COMMAND ""
          INSTALL_COMMAND ""
          )

        BuildDepMatrix(pcre-ext pcre CMAKE_ARGS ${PCRE_CMAKE_ARGS})

        list(APPEND SIMH_BUILD_DEPS pcre)
        list(APPEND SIMH_DEP_TARGETS pcre-ext)
    ENDIF ()
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
            URL ${PNG_SOURCE_URL}
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

        BuildDepMatrix(png-dep libpng RELEASE_BUILD ${PNG_CMAKE_BUILD_TYPE_RELEASE})

        list(APPEND SIMH_BUILD_DEPS "png")
        list(APPEND SIMH_DEP_TARGETS "png-dep")
        message(STATUS "Building PNG from ${PNG_SOURCE_URL}")
        list(APPEND VIDEO_PKG_STATUS "PNG source build")
    ENDIF (NOT PNG_FOUND)

    IF (NOT Freetype_FOUND)
        set(FREETYPE_DEPS)
        if (TARGET png-dep)
            list(APPEND FREETYPE_DEPS "png-dep")
        endif (TARGET png-dep)

        ExternalProject_Add(freetype-dep
            URL ${FREETYPE_SOURCE_URL}
            DEPENDS
                ${FREETYPE_DEPS}
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
        )

        BuildDepMatrix(freetype-dep FreeType)

        list(APPEND SIMH_BUILD_DEPS "freetype")
        list(APPEND SIMH_DEP_TARGETS "freetype-dep")
        message(STATUS "Building Freetype from ${FREETYPE_SOURCE_URL}.")
        list(APPEND VIDEO_PKG_STATUS "Freetype source build")
    endif (NOT Freetype_FOUND)

    IF (NOT SDL2_FOUND)
        ExternalProject_Add(sdl2-dep
            URL ${SDL2_SOURCE_URL}
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
        )

        BuildDepMatrix(sdl2-dep SDL2)

        list(APPEND SIMH_BUILD_DEPS "SDL2")
        list(APPEND SIMH_DEP_TARGETS "sdl2-dep")
        message(STATUS "Building SDL2 from ${SDL2_SOURCE_URL}.")
        list(APPEND VIDEO_PKG_STATUS "SDL2 source build")
    ENDIF (NOT SDL2_FOUND)

    IF (NOT SDL2_ttf_FOUND)
        set(SDL2_ttf_depdir ${CMAKE_BINARY_DIR}/sdl2-ttf-dep-prefix/src/sdl2-ttf-dep/)
        set(SDL2_ttf_DEPS)

        if (TARGET sdl2-dep)
            list(APPEND SDL2_ttf_DEPS sdl2-dep)
        endif (TARGET sdl2-dep)
        if (TARGET freetype-dep)
            list(APPEND SDL2_ttf_DEPS freetype-dep)
        endif (TARGET freetype-dep)

        ExternalProject_Add(sdl2-ttf-dep
            URL ${SDL2_TTF_SOURCE_URL}
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
            DEPENDS
                ${SDL2_ttf_DEPS}
        )

        BuildDepMatrix(sdl2-ttf-dep SDL2_ttf)

        list(APPEND SIMH_BUILD_DEPS "SDL2_ttf")
        list(APPEND SIMH_DEP_TARGETS "sdl2-ttf-dep")
        message(STATUS "Building SDL2_ttf from https://www.libsdl.org/release/SDL2_ttf-2.0.15.zip.")
        list(APPEND VIDEO_PKG_STATUS "SDL2_ttf source build")
    ENDIF (NOT SDL2_ttf_FOUND)

    IF (SDL2_ttf_FOUND)
        IF (TARGET SDL2_ttf::SDL2_ttf)
            target_compile_definitions(simh_video INTERFACE $<TARGET_PROPERTY:SDL2_ttf::SDL2_ttf,INTERFACE_COMPILE_DEFINITIONS>)
            target_include_directories(simh_video INTERFACE $<TARGET_PROPERTY:SDL2_ttf::SDL2_ttf,INTERFACE_INCLUDE_DIRECTORIES>)
            target_link_libraries(simh_video INTERFACE SDL2_ttf::SDL2_ttf)
            list(APPEND VIDEO_PKG_STATUS "detected SDL2_ttf")
        ELSEIF (TARGET PkgConfig::SDL2_ttf)
            target_compile_definitions(simh_video INTERFACE $<TARGET_PROPERTY:PkgConfig::SDL2_ttf,INTERFACE_COMPILE_DEFINITIONS>)
            target_include_directories(simh_video INTERFACE $<TARGET_PROPERTY:PkgConfig::SDL2_ttf,INTERFACE_INCLUDE_DIRECTORIES>)
            target_link_libraries(simh_video INTERFACE PkgConfig::SDL2_ttf)
            list(APPEND VIDEO_PKG_STATUS "pkg-config SDL2_ttf")
        ELSEIF (DEFINED SDL_ttf_LIBRARIES AND DEFINED SDL_ttf_INCLUDE_DIRS)
            target_link_libraries(simh_video INTERFACE ${SDL_ttf_LIBRARIES})
            target_include_directories(simh_video INTERFACE ${SDL_ttf_INCLUDE_DIRS})
            list(APPEND VIDEO_PKG_STATUS "detected SDL2_ttf")
        ELSE ()
            message(FATAL_ERROR "SDL2_ttf_FOUND set but no SDL2_ttf::SDL2_ttf import library or SDL_ttf_LIBRARIES/SDL_ttf_INCLUDE_DIRS? ")
        ENDIF ()
    ENDIF (SDL2_ttf_FOUND)

    IF (SDL2_FOUND)
        target_compile_definitions(simh_video INTERFACE USE_SIM_VIDEO HAVE_LIBSDL)

        IF (TARGET SDL2::SDL2)
            target_compile_definitions(simh_video INTERFACE $<TARGET_PROPERTY:SDL2::SDL2,INTERFACE_COMPILE_DEFINITIONS>)
            target_include_directories(simh_video INTERFACE $<TARGET_PROPERTY:SDL2::SDL2,INTERFACE_INCLUDE_DIRECTORIES>)
            target_link_libraries(simh_video INTERFACE SDL2::SDL2)
            list(APPEND VIDEO_PKG_STATUS "detected SDL2")
        ELSEIF (TARGET PkgConfig::SDL2)
            target_compile_definitions(simh_video INTERFACE $<TARGET_PROPERTY:PkgConfig::SDL2,INTERFACE_COMPILE_DEFINITIONS>)
            target_include_directories(simh_video INTERFACE $<TARGET_PROPERTY:PkgConfig::SDL2,INTERFACE_INCLUDE_DIRECTORIES>)
            target_link_libraries(simh_video INTERFACE PkgConfig::SDL2)
            list(APPEND VIDEO_PKG_STATUS "pkg-config SDL2")
        ELSEIF (DEFINED SDL2_LIBRARIES AND DEFINED SDL2_INCLUDE_DIRS)
            target_link_libraries(simh_video INTERFACE ${SDL2_LIBRARIES})
            target_include_directories(simh_video INTERFACE ${SDL2_INCLUDE_DIRS})
            list(APPEND VIDEO_PKG_STATUS "detected SDL2")
        ELSE ()
            message(FATAL_ERROR "SDL2_FOUND set but no SDL2::SDL2 import library or SDL2_LIBRARIES/SDL2_INCLUDE_DIRS?")
        ENDIF ()
    ENDIF (SDL2_FOUND)

    IF (Freetype_FOUND)
        IF (TARGET Freetype::Freetype)
            target_compile_definitions(simh_video INTERFACE $<TARGET_PROPERTY:Freetype::Freetype,INTERFACE_COMPILE_DEFINITIONS>)
            target_include_directories(simh_video INTERFACE $<TARGET_PROPERTY:Freetype::Freetype,INTERFACE_INCLUDE_DIRECTORIES>)
            target_link_libraries(simh_video INTERFACE Freetype::Freetype)
            list(APPEND VIDEO_PKG_STATUS "detected Freetype")
        ELSEIF (TARGET PkgConfig::Freetype)
            target_compile_definitions(simh_video INTERFACE $<TARGET_PROPERTY:PkgConfig::Freetype,INTERFACE_COMPILE_DEFINITIONS>)
            target_include_directories(simh_video INTERFACE $<TARGET_PROPERTY:PkgConfig::Freetype,INTERFACE_INCLUDE_DIRECTORIES>)
            target_link_libraries(simh_video INTERFACE PkgConfig::Freetype)
            list(APPEND VIDEO_PKG_STATUS "pkg-config Freetype")
        ENDIF (TARGET Freetype::Freetype)

        if (HARFBUZZ_FOUND)
            if (TARGET Harfbuzz::Harfbuzz)
                target_compile_definitions(simh_video INTERFACE $<TARGET_PROPERTY:Harfbuzz::Harfbuzz,INTERFACE_COMPILE_DEFINITIONS>)
                target_include_directories(simh_video INTERFACE $<TARGET_PROPERTY:Harfbuzz::Harfbuzz,INTERFACE_INCLUDE_DIRECTORIES>)
                target_link_libraries(simh_video INTERFACE Harfbuzz::Harfbuzz)
            elseif (TARGET PkgConfig::Harfbuzz)
                target_compile_definitions(simh_video INTERFACE $<TARGET_PROPERTY:Harfbuzz::Harfbuzz,INTERFACE_COMPILE_DEFINITIONS>)
                target_include_directories(simh_video INTERFACE $<TARGET_PROPERTY:Harfbuzz::Harfbuzz,INTERFACE_INCLUDE_DIRECTORIES>)
                target_link_libraries(simh_video INTERFACE Harfbuzz::Harfbuzz)
            endif ()
        endif (HARFBUZZ_FOUND)

        if (BROTLIDEC_FOUND)
            target_include_directories(simh_video INTERFACE ${BROTLIDEC_INCLUDE_DIRS})
            target_link_libraries(simh_video INTERFACE ${BROTLIDEC_LIBRARIES})
        endif (BROTLIDEC_FOUND)
    ENDIF (Freetype_FOUND)

    IF (PNG_FOUND)
        if (TARGET PNG::PNG)
            target_compile_definitions(simh_video INTERFACE $<TARGET_PROPERTY:PNG::PNG,INTERFACE_COMPILE_DEFINITIONS>)
            target_include_directories(simh_video INTERFACE $<TARGET_PROPERTY:PNG::PNG,INTERFACE_INCLUDE_DIRECTORIES>)
            target_link_libraries(simh_video INTERFACE PNG::PNG)
            list(APPEND VIDEO_PKG_STATUS "detected PNG")
        elseif (TARGET PkgConfig::PNG)
            target_compile_definitions(simh_video INTERFACE $<TARGET_PROPERTY:PkgConfig::PNG,INTERFACE_COMPILE_DEFINITIONS>)
            target_include_directories(simh_video INTERFACE $<TARGET_PROPERTY:PkgConfig::PNG,INTERFACE_INCLUDE_DIRECTORIES>)
            target_link_libraries(simh_video INTERFACE PkgConfig::PNG)
            list(APPEND VIDEO_PKG_STATUS "pkg-config PNG")
        else ()
            target_include_directories(simh_video INTERFACE ${PNG_INCLUDE_DIRS})
            target_link_libraries(simh_video INTERFACE ${PNG_LIBRARIES})
            list(APPEND VIDEO_PKG_STATUS "detected PNG")
        endif ()

        target_compile_definitions(simh_video INTERFACE ${PNG_DEFINITIONS} HAVE_LIBPNG)
    ENDIF (PNG_FOUND)

    set(BUILD_WITH_VIDEO TRUE)
ELSE ()
    set(VIDEO_PKG_STATUS "video support disabled")
ENDIF(WITH_VIDEO)

if (WITH_NETWORK)
    ## By default, use the static network runtime (bad choice of manifest constant,
    ## but what can one do?)
    set(network_runtime)

    if (PCAP_FOUND)
        set(network_runtime USE_SHARED)
        foreach(hdr "${PCAP_INCLUDE_DIRS}")
          file(STRINGS ${hdr}/pcap/pcap.h hdrcontent REGEX "pcap_compile *\\(.*const")
          # message("hdrcontent: ${hdrcontent}")
          list(LENGTH hdrcontent have_bpf_const)
          if (${have_bpf_const} GREATER 0)
            message(STATUS "pcap_compile requires BPF_CONST_STRING")
            list(APPEND network_runtime BPF_CONST_STRING)
          endif()
        endforeach()

        target_include_directories(simh_network INTERFACE "${PCAP_INCLUDE_DIRS}")
        target_compile_definitions(simh_network INTERFACE HAVE_PCAP_NETWORK)

        list (APPEND NETWORK_PKG_STATUS "PCAP dynamic")
    else (PCAP_FOUND)
        list(APPEND NETWORK_PKG_STATUS "PCAP dynamic (unpacked)")

        message(STATUS "Downloading ${LIBPCAP_SOURCE_URL}")
        message(STATUS "Destination ${CMAKE_BINARY_DIR}/libpcap")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/libpcap"
            RESULT_VARIABLE LIBPCAP_MKDIR
        )
        if (NOT (${LIBPCAP_MKDIR} EQUAL 0))
            message(FATAL_ERROR "Could not create ${CMAKE_CMAKE_BINARY_DIR}/libpcap")
        endif (NOT (${LIBPCAP_MKDIR} EQUAL 0))

        file(DOWNLOAD "${LIBPCAP_SOURCE_URL}" "${CMAKE_BINARY_DIR}/libpcap/libpcap.${LIBPCAP_ARCHIVE_TYPE}"
                STATUS LIBPCAP_DOWNLOAD
            )
        list(GET LIBPCAP_DOWNLOAD 0 LIBPCAP_DL_STATUS)
        if (NOT (${LIBPCAP_DL_STATUS} EQUAL 0))
            list(GET LIBPCAP_DOWNLOAD 1 LIBPCAP_DL_ERROR)
            message(FATAL_ERROR "Download failed: ${LIBPCAP_DL_ERROR}")
        endif (NOT (${LIBPCAP_DL_STATUS} EQUAL 0))

        message(STATUS "Extracting headers ${LIBPCAP_SOURCE_URL}")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xvf "${CMAKE_BINARY_DIR}/libpcap/libpcap.${LIBPCAP_ARCHIVE_TYPE}"
                "${LIBPCAP_PROJECT}-${LIBPCAP_ARCHIVE_NAME}-${LIBPCAP_RELEASE}/pcap.h"
                "${LIBPCAP_PROJECT}-${LIBPCAP_ARCHIVE_NAME}-${LIBPCAP_RELEASE}/pcap/*.h"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/libpcap"
            RESULT_VARIABLE LIBPCAP_EXTRACT
        )
        if (NOT (${LIBPCAP_EXTRACT} EQUAL 0))
            message(FATAL_ERROR "Extract failed.")
        endif (NOT (${LIBPCAP_EXTRACT} EQUAL 0))

        message(STATUS "Copying headers from ${CMAKE_BINARY_DIR}/libpcap/${LIBPCAP_PROJECT}-${LIBPCAP_ARCHIVE_NAME}-${LIBPCAP_RELEASE}/pcap")
        message(STATUS "Destination ${SIMH_DEP_TOPDIR}/include/pcap")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E copy_directory "${LIBPCAP_PROJECT}-${LIBPCAP_ARCHIVE_NAME}-${LIBPCAP_RELEASE}/"
                "${SIMH_DEP_TOPDIR}/include/"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/libpcap"
            RESULT_VARIABLE LIBPCAP_COPYDIR
        )
        if (NOT (${LIBPCAP_COPYDIR} EQUAL 0))
            message(FATAL_ERROR "Copy failed.")
        endif (NOT (${LIBPCAP_COPYDIR} EQUAL 0))
    endif (PCAP_FOUND)

    ## TAP/TUN devices
    if (WITH_TAP)
        target_compile_definitions(simh_network INTERFACE ${NETWORK_TUN_DEFS})
    endif (WITH_TAP)

    if (VDE_FOUND)
        if (TARGET PkgConfig::VDE)
            target_compile_definitions(simh_network INTERFACE $<TARGET_PROPERTY:PkgConfig::VDE,INTERFACE_COMPILE_DEFINITIONS>)
            target_include_directories(simh_network INTERFACE $<TARGET_PROPERTY:PkgConfig::VDE,INTERFACE_INCLUDE_DIRECTORIES>)
            target_link_libraries(simh_network INTERFACE PkgConfig::VDE)
            list(APPEND NETWORK_PKG_STATUS "pkg-config VDE")
        else ()
            target_include_directories(simh_network INTERFACE "${VDEPLUG_INCLUDE_DIRS}")
            target_link_libraries(simh_network INTERFACE "${VDEPLUG_LIBRARY}")
            list(APPEND NETWORK_PKG_STATUS "detected VDE")
        endif ()

        target_compile_definitions(simh_network INTERFACE HAVE_VDE_NETWORK)
    endif (VDE_FOUND)

    if (WITH_TAP)
        if (HAVE_TAP_NETWORK)
            target_compile_definitions(simh_network INTERFACE HAVE_TAP_NETWORK)

            if (HAVE_BSDTUNTAP)
                target_compile_definitions(simh_network INTERFACE HAVE_BSDTUNTAP)
                list(APPEND NETWORK_PKG_STATUS "BSD TUN/TAP")
            else (HAVE_BSDTUNTAP)
                list(APPEND NETWORK_PKG_STATUS "TAP")
            endif (HAVE_BSDTUNTAP)
   
        endif (HAVE_TAP_NETWORK)
    endif (WITH_TAP)

    if (WITH_SLIRP)
        target_link_libraries(simh_network INTERFACE slirp)
        list(APPEND NETWORK_PKG_STATUS "NAT(SLiRP)")
    endif (WITH_SLIRP)

    ## Finally, set the network runtime
    if (NOT network_runtime)
        ## Default to USE_SHARED... USE_NETWORK is deprecated.
        set(network_runtime USE_SHARED)
    endif (NOT network_runtime)

    target_compile_definitions(simh_network INTERFACE ${network_runtime})
    set(BUILD_WITH_NETWORK TRUE)
else (WITH_NETWORK)
    set(NETWORK_STATUS "networking disabled")
    set(NETWORK_PKG_STATUS "network disabled")
    set(BUILD_WITH_NETWORK FALSE)
endif (WITH_NETWORK)

## Add needed platform runtime/libraries.
##
## Windows: socket libs (even with networking disabled), winmm (for ms timer functions)
## Linux: large file support
if (WIN32)
    target_link_libraries(simh_network INTERFACE ws2_32 wsock32)
endif ()