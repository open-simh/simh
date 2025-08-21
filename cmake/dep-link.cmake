##+
## dep-link.cmake: Create the dependency interface libraries
##-
             
add_library(simh_regexp INTERFACE)
add_library(simh_video INTERFACE)
add_library(simh_network INTERFACE)
        
## LIBPCAP is a special case
set(LIBPCAP_PROJECT "libpcap")
set(LIBPCAP_ARCHIVE_NAME "libpcap")
set(LIBPCAP_RELEASE "1.10.1")
set(LIBPCAP_ARCHIVE_TYPE "tar.gz")
set(LIBPCAP_TAR_ARCHIVE "${LIBPCAP_ARCHIVE_NAME}-${LIBPCAP_RELEASE}.${LIBPCAP_ARCHIVE_TYPE}")
set(LIBPCAP_SOURCE_URL  "https://github.com/the-tcpdump-group/libpcap/archive/refs/tags/${LIBPCAP_TAR_ARCHIVE}")
                                                             
function(fix_interface_libs _targ)
get_target_property(_aliased ${_targ} ALIASED_TARGET)
    if(NOT _aliased)
        set(fixed_libs)
        get_property(orig_libs TARGET ${_targ} PROPERTY INTERFACE_LINK_LIBRARIES)
        foreach(each_lib IN LISTS ${_lib})
            get_filename_component(stripped_lib "${each_lib}" DIRECTORY)
            if (stripped_lib)
                string(STRIP ${each_lib} stripped_lib)
                file(TO_CMAKE_PATH "${stripped_lib}" stripped_lib)
                if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
                  file(REAL_PATH "${stripped_lib}" stripped_lib)
                endif ()
            endif ()
            list(APPEND fixed_libs ${stripped_lib})
            message("** \"${each_lib}\" -> \"${stripped_lib}\"")
        endforeach ()
        set_property(TARGET ${_targ} PROPERTY INTERFACE_LINK_LIBRARIES ${fixed_libs})
    endif ()
endfunction ()

## Ubuntu 16.04 -- when we find the SDL2 library, there are trailing spaces. Strip
## spaces from SDL2_LIBRARIES (and potentially others as we find them).
function (fix_libraries _lib)
    set(fixed_libs)
    foreach(each_lib IN LISTS ${_lib})
        get_filename_component(stripped_lib "${each_lib}" DIRECTORY)
        if (stripped_lib)
            string(STRIP ${stripped_lib} stripped_lib)
            file(TO_CMAKE_PATH "${stripped_lib}" stripped_lib)
            if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
              file(REAL_PATH "${stripped_lib}" stripped_lib)
            endif ()
        endif ()
        list(APPEND fixed_libs ${stripped_lib})
    endforeach ()
    set(${_lib} ${fixed_libs} PARENT_SCOPE)
endfunction ()

set(BUILD_WITH_VIDEO FALSE)
IF (WITH_VIDEO)
    ## +10 chaotic neutral hack: The SDL2_ttf CMake configuration include "-lfreetype" and
    ## "-lharfbuzz", but, if you're on MacOS, you need to tell the linker where these libraries
    ## are located...
    set(ldirs)
    foreach (lname ${FREETYPE_LIBRARIES} ${FREETYPE_LIBRARY} ${HARFBUZZ_LIBRARIES} ${HARFBUZZ_LIBRARY})
        get_filename_component(dirname "${lname}" DIRECTORY)
        if (dirname)
            string(STRIP ${dirname} dirname)
            file(TO_CMAKE_PATH "${dirname}" dirname)
            if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
              file(REAL_PATH "${dirname}" dirname)
            endif ()
            list(APPEND ldirs ${dirname})
        endif()
    endforeach ()
    get_property(ilink_dirs TARGET simh_video PROPERTY INTERFACE_LINK_DIRECTORIES)
    list(APPEND ilink_dirs ${ldirs})
    set_property(TARGET simh_video PROPERTY INTERFACE_LINK_DIRECTORIES ${ilink_dirs})
    unset(ilink_dirs)
    unset(ldirs)

    IF (SDL2_ttf_FOUND)
        IF (WIN32 AND TARGET SDL2_ttf::SDL2_ttf-static)
            target_link_libraries(simh_video INTERFACE SDL2_ttf::SDL2_ttf-static)
            list(APPEND VIDEO_PKG_STATUS "SDL2_ttf static")
        ELSEIF (TARGET SDL2_ttf::SDL2_ttf)
            target_link_libraries(simh_video INTERFACE SDL2_ttf::SDL2_ttf)
            list(APPEND VIDEO_PKG_STATUS "SDL2_ttf dynamic")
        ELSEIF (TARGET PkgConfig::SDL2_ttf)
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
        ##
        ## Hopefully this hack can go away. Had to move the target_compile_definitions
        ## over to add_simulator.cmake to accomodate the BESM6 SDL irregularity.
        ##
        ## (keep)  if (CMAKE_HOST_APPLE)
        ## (keep)      ## NOTE: This shouldn't be just an Apple platform quirk; SDL_main should
        ## (keep)      ## be used by all platforms. <sigh!>
        ## (keep)      target_compile_definitions(simh_video INTERFACE SDL_MAIN_AVAILABLE)
        ## (keep)  endif ()

        ## Link to SDL2main if defined for this platform.
        target_link_libraries(simh_video INTERFACE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>)

        IF (WIN32 AND TARGET SDL2::SDL2-static AND TARGET SDL2_ttf::SDL2_ttf-static)
            ## Prefer the static version on Windows, but only if SDL2_ttf is also static.
            target_link_libraries(simh_video INTERFACE SDL2::SDL2-static)
            list(APPEND VIDEO_PKG_STATUS "SDL2 static")
        ELSEIF (TARGET SDL2::SDL2)
            fix_interface_libs(SDL2::SDL2)
            target_link_libraries(simh_video INTERFACE SDL2::SDL2)
            list(APPEND VIDEO_PKG_STATUS "SDL2 dynamic")
        ELSEIF (TARGET PkgConfig::SDL2)
            fix_interface_libs(PkgConfig::SDL2)
            target_link_libraries(simh_video INTERFACE PkgConfig::SDL2)
            list(APPEND VIDEO_PKG_STATUS "pkg-config SDL2")
        ELSEIF (DEFINED SDL2_LIBRARIES AND DEFINED SDL2_INCLUDE_DIRS)
            fix_libraries(SDL2_LIBRARIES)
            target_link_libraries(simh_video INTERFACE ${SDL2_LIBRARIES})
            target_include_directories(simh_video INTERFACE ${SDL2_INCLUDE_DIRS})
            list(APPEND VIDEO_PKG_STATUS "detected SDL2")
        ELSE ()
            message(FATAL_ERROR "SDL2_FOUND set but no SDL2::SDL2 import library or SDL2_LIBRARIES/SDL2_INCLUDE_DIRS?")
        ENDIF ()
    ENDIF (SDL2_FOUND)

    IF (NOT USING_VCPKG AND FREETYPE_FOUND)
        if (TARGET Freetype::Freetype)
            target_link_libraries(simh_video INTERFACE freetype)
            list(APPEND VIDEO_PKG_STATUS "Freetype::Freetype")
        ELSEIF (TARGET PkgConfig::Freetype)
            target_link_libraries(simh_video INTERFACE PkgConfig::Freetype)
            list(APPEND VIDEO_PKG_STATUS "pkg-config Freetype")
        ELSE ()
            target_link_libraries(simh_video INTERFACE ${FREETYPE_LIBRARIES})
            target_include_directories(simh_video INTERFACE ${FREETYPE_INCLUDE_DIRS})
            list(APPEND VIDEO_PKG_STATUS "detected Freetype")
        ENDIF ()
    ENDIF ()

    IF (PNG_FOUND)
        target_compile_definitions(simh_video INTERFACE HAVE_LIBPNG)

        if (TARGET PNG::PNG)
            target_link_libraries(simh_video INTERFACE PNG::PNG)
            list(APPEND VIDEO_PKG_STATUS "interface PNG")
        elseif (TARGET PkgConfig::PNG)
            target_link_libraries(simh_video INTERFACE PkgConfig::PNG)
            list(APPEND VIDEO_PKG_STATUS "pkg-config PNG")
        else ()
            target_include_directories(simh_video INTERFACE ${PNG_INCLUDE_DIRS})
            target_link_libraries(simh_video INTERFACE ${PNG_LIBRARIES})
            list(APPEND VIDEO_PKG_STATUS "detected PNG")
        endif ()
    ENDIF (PNG_FOUND)

    set(BUILD_WITH_VIDEO TRUE)
ELSE ()
    set(VIDEO_PKG_STATUS "video support disabled")
ENDIF()

if (WITH_REGEX)
    ## TEMP: Use PCRE until patches for PCRE2 are avaiable.
    ##
    ## 1. Prefer PCRE2 over PCRE (unless PREFER_PCRE is set)
    ## 2. Prefer interface libraries before using detected find_package
    ##    variables.
    IF (TARGET PkgConfig::PCRE)
        target_link_libraries(simh_regexp INTERFACE PkgConfig::PCRE)
        if (PREFER_PCRE)
            target_compile_definitions(simh_regexp INTERFACE HAVE_PCRE_H)
            set(PCRE_PKG_STATUS "pkg-config pcre")
        else ()
            target_compile_definitions(simh_regexp INTERFACE HAVE_PCRE2_H)
            if (WIN32)
                ## Use static linkage (vice DLL) on Windows:
                target_compile_definitions(simh_regexp INTERFACE PCRE2_STATIC)
            endif ()
            set(PCRE_PKG_STATUS "pkg-config pcre2")
        endif ()
    ELSEIF (TARGET unofficial::pcre::pcre)
        ## vcpkg:
        target_link_libraries(simh_regexp INTERFACE unofficial::pcre::pcre)
        target_compile_definitions(simh_regexp INTERFACE HAVE_PCRE_H)
        target_compile_definitions(simh_regexp INTERFACE PCRE_STATIC)
        set(PCRE_PKG_STATUS "vcpkg pcre")
    ELSEIF (NOT PREFER_PCRE AND PCRE2_FOUND)
        target_compile_definitions(simh_regexp INTERFACE HAVE_PCRE2_H)
        target_include_directories(simh_regexp INTERFACE ${PCRE2_INCLUDE_DIRS})
            target_link_libraries(simh_regexp INTERFACE ${PCRE2_LIBRARY})
        if (WIN32)
            ## Use static linkage (vice DLL) on Windows:
            target_compile_definitions(simh_regexp INTERFACE PCRE2_STATIC)
        endif ()

        set(PCRE_PKG_STATUS "detected pcre2")
    ELSEIF (PCRE_FOUND)
        target_compile_definitions(simh_regexp INTERFACE HAVE_PCRE_H)
        target_include_directories(simh_regexp INTERFACE ${PCRE_INCLUDE_DIRS})
        target_link_libraries(simh_regexp INTERFACE ${PCRE_LIBRARY})
        if (WIN32)
            target_compile_definitions(simh_regexp INTERFACE PCRE_STATIC)
        endif ()
        set(PCRE_PKG_STATUS "detected pcre")
    endif ()
endif ()

if ((WITH_REGEX OR WITH_VIDEO) AND ZLIB_FOUND)
    target_compile_definitions(simh_regexp INTERFACE HAVE_ZLIB)
    target_compile_definitions(simh_video INTERFACE HAVE_ZLIB)
    if (TARGET ZLIB::ZLIB)
        target_link_libraries(simh_regexp INTERFACE ZLIB::ZLIB)
        target_link_libraries(simh_video INTERFACE ZLIB::ZLIB)
        set(ZLIB_PKG_STATUS "interface ZLIB")
    elseif (TARGET PkgConfig::ZLIB)
        target_link_libraries(simh_regexp INTERFACE PkgConfig::ZLIB)
        target_link_libraries(simh_video INTERFACE PkgConfig::ZLIB)
        set(ZLIB_PKG_STATUS "pkg-config ZLIB")
    else ()
        target_include_directories(simh_regexp INTERFACE ${ZLIB_INCLUDE_DIRS})
        target_link_libraries(simh_regexp INTERFACE ${ZLIB_LIBRARIES})
        target_include_directories(simh_video INTERFACE ${ZLIB_INCLUDE_DIRS})
        target_link_libraries(simh_video INTERFACE ${ZLIB_LIBRARIES})
        set(ZLIB_PKG_STATUS "detected ZLIB")
    endif ()
endif ()


if (WITH_NETWORK)
    set(network_runtime USE_SHARED)
    ## pcap is special: Headers only and dynamically loaded.
    if (WITH_PCAP)
        find_package(PCAP)

        if (NOT PCAP_FOUND)
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
            message(STATUS "Destination ${CMAKE_BINARY_DIR}/include/pcap")
            execute_process(
                COMMAND "${CMAKE_COMMAND}" -E copy_directory
                    "${LIBPCAP_PROJECT}-${LIBPCAP_ARCHIVE_NAME}-${LIBPCAP_RELEASE}/"
                    "${CMAKE_BINARY_DIR}/include/"
                WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/libpcap"
                RESULT_VARIABLE LIBPCAP_COPYDIR
            )
            if (NOT (${LIBPCAP_COPYDIR} EQUAL 0))
                message(FATAL_ERROR "Copy failed.")
            endif (NOT (${LIBPCAP_COPYDIR} EQUAL 0))

            ## And try finding it again...
            find_package(PCAP)
        else ()
            list (APPEND NETWORK_PKG_STATUS "PCAP dynamic")
        endif ()


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
        endif ()
    endif ()

    ## TAP/TUN devices
    if (WITH_TAP)
        target_compile_definitions(simh_network INTERFACE ${NETWORK_TUN_DEFS})
    endif (WITH_TAP)

    if (WITH_VDE AND VDE_FOUND)
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
    endif ()

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

    if (WITH_VMNET AND APPLE)
        # CMAKE_OSX_DEPLOYMENT_TARGET is attractive, but not set by default.
        # See what we're using, either by default or what the user has set.
        check_c_source_compiles(
            "
            #include <Availability.h>
            #if TARGET_OS_OSX && __MAC_OS_X_VERSION_MIN_REQUIRED < 101000
            #error macOS too old
            #endif
            int main(int argc, char **argv) { return 0; }
            " TARGETING_MACOS_10_10)
        if (NOT TARGETING_MACOS_10_10)
        message(FATAL_ERROR "vmnet.framework requires targeting macOS 10.10 or newer")
        endif()

        # vmnet requires blocks for its callback parameter, even in vanilla C.
        # This is only supported in clang, not by GCC. It's default in clang,
        # but we should be clear about it. Do a feature instead of compiler
        # check anyways though, in case GCC does add it eventually.
        check_c_compiler_flag(-fblocks HAVE_C_BLOCKS)
        if (NOT HAVE_C_BLOCKS)
            message(FATAL_ERROR "vmnet.framework requires blocks support in the C compiler")
        endif()
        target_compile_options(simh_network INTERFACE -fblocks)

        target_link_libraries(simh_network INTERFACE "-framework vmnet")
        target_compile_definitions(simh_network INTERFACE HAVE_VMNET_NETWORK)
        list(APPEND NETWORK_PKG_STATUS "macOS vmnet.framework")
    endif(WITH_VMNET AND APPLE)

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
