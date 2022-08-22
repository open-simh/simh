## platform_quirks.cmake
##
## This is the place where the CMake build handles various platform quirks,
## such as architecture-specific prefixes (Linux, Windows) and MacOS
## HomeBrew
##
## "scooter me fecit"

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
    elseif (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
        foreach (arch "x86_64-linux-gnu" "aarch64-linux-gnu")
            if (EXISTS "/usr/lib/${arch}")
                message(STATUS "CMAKE_LIBRARY_ARCHITECTURE set to ${arch}")
                set(CMAKE_C_LIBRARY_ARCHITECTURE "${arch}")
                set(CMAKE_LIBRARY_ARCHITECTURE "${arch}") 
            endif()
        endforeach()
    endif ()
endif()

## Smells like HomeBrew?
if (CMAKE_HOST_APPLE)
    if (EXISTS "/usr/local/Cellar" OR EXISTS "/opt/homebrew/Cellar")
        message(STATUS "Adding HomeBrew paths to library and include search")
        set(hb_topdir "/usr/local/Cellar")
        if (EXISTS "/opt/homebrew/Cellar")
            set(hb_topdir "/opt/homebrew/Cellar")
        endif()

        file(GLOB hb_lib_candidates LIST_DIRECTORIES TRUE "${hb_topdir}/*/*/lib")
        file(GLOB hb_include_candidates LIST_DIRECTORIES TRUE "${hb_topdir}/*/*/include/*")
        # file(GLOB hb_include_cand2 LIST_DIRECTORIES TRUE "${hb_topdir}/*/*/include/*")
        # list(APPEND hb_include_candidates ${hb_include_cand2})

        # message("@@ lib candidates ${hb_lib_candidates}")
        # message("@@ inc candidates ${hb_include_candidates}")

        set(hb_libs "")
        foreach (hb_path ${hb_lib_candidates})
            if (IS_DIRECTORY "${hb_path}")
                # message("@@ consider ${hb_path}")
                list(APPEND hb_libs "${hb_path}")
            endif()
        endforeach()

        set(hb_includes "")
        foreach (hb_path ${hb_include_candidates})
            if (IS_DIRECTORY "${hb_path}")
                # message("@@ consider ${hb_path}")
                list(APPEND hb_includes "${hb_path}")
            endif()
        endforeach()

        # message("hb_libs ${hb_libs}")
        # message("hb_includes ${hb_includes}")

        list(PREPEND CMAKE_LIBRARY_PATH ${hb_libs})
        list(PREPEND CMAKE_INCLUDE_PATH ${hb_includes})

        unset(hb_lib_candidates)
        unset(hb_include_candidates)
        unset(hb_includes)
        unset(hb_libs)
        unset(hb_path)
    endif()

#    if (POLICY CMP0042)
#        ## Embed runtime path
#        cmake_policy(SET CMP0042 NEW)
#    endif ()
#
#    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()
