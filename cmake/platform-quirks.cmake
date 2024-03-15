# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Except as contained in this notice, the names of The Authors shall not be
# used in advertising or otherwise to promote the sale, use or other dealings
# in this Software without prior written authorization from the Authors.

## platform_quirks.cmake
##
## This is the place where the CMake build handles various platform quirks,
## such as architecture-specific prefixes (Linux, Windows) and MacOS
## HomeBrew
##
## Author: B. Scott Michel
# "scooter me fecit"


set(EXTRA_TARGET_CFLAGS)
set(EXTRA_TARGET_CFLAGS)

# For 64-bit builds (and this is especially true for MSVC), set the library
# architecture.
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    ## Strongly encourage (i.e., force) CMake to look in the x64 architecture
    ## directories:
    if (MSVC OR MINGW)
        # set(CMAKE_C_LIBRARY_ARCHITECTURE "x64")
        # set(CMAKE_LIBRARY_ARCHITECTURE "x64")
    elseif (${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
        ## Linux has architecture-specific subdirectories where CMake needs to
        ## search for headers. Currently, we know about x64 and ARM architecture
        ## variants.
        foreach (arch "x86_64-linux-gnu" "aarch64-linux-gnu" "arm-linux-gnueabihf")
            if (EXISTS "/usr/lib/${arch}")
                message(STATUS "CMAKE_LIBRARY_ARCHITECTURE set to ${arch}")
                set(CMAKE_C_LIBRARY_ARCHITECTURE "${arch}")
                set(CMAKE_LIBRARY_ARCHITECTURE "${arch}") 
            endif()
        endforeach()
    endif ()
endif()

if (WIN32)
    ## At some point, bring this back in to deal with MS ISO C99 deprecation.
    ## Right now, it's not in the code base and the warnings are squelched.
    ##
    ## (keep): if (MSVC_VERSION GREATER_EQUAL 1920)
    ## (keep):     add_compile_definitions(USE_ISO_C99_NAMES)
    ## (keep): endif ()

    if (MSVC)
        ## Flags enabled in the SIMH VS solution (diff redution):
        ##
        ## /EHsc: Standard C++ exception handling, extern "C" functions never
        ##        throw exceptions.
        ## /FC: Output full path name of source in diagnostics
        ## /GF: String pooling
        ## /GL: Whole program optimization
        ## /Gy: Enable function-level linking
        ## /Oi: Emit intrinsic functions
        ## /Ot: Favor fast code
        ## /Oy: Suppress generating a stack frame (??? why?)
        add_compile_options("$<$<CONFIG:Release>:/EHsc;/GF;/Gy;/Oi;/Ot;/Oy;/Zi>")
        add_compile_options("$<$<CONFIG:Debug>:/EHsc;/FC>")

        if (RELEASE_LTO)
            ## /LTCG: Link-Time Code Generation. Pair with /GL at compile time.
            add_compile_options("$<$<CONFIG:Release>:/GL>")
            add_link_options("$<$<CONFIG:Release>:/LTCG>")
            message(STATUS "Adding LTO to Release compiler and linker flags")
        endif ()

        ## Set the MSVC runtime. Note CMP0091 policy is set to new early on in
        ## the top-level CMakeLists.txt
        if (BUILD_SHARED_DEPS)
            set(use_rtdll "$<$<BOOL:${BUILD_SHARED_DEPS}:DLL>")
        else ()
            set(use_rtdll "")
        endif ()

        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>${use_rtll}")

        ## Disable automagic add for _MBCS:
        add_definitions(-D_SBCS)

        if (CMAKE_VERSION VERSION_LESS "3.23")
            ## -5 Evil hack to ensure that find_package() can match against an empty
            ## prefix and not trigger the "CMAKE_FIND_LIBRARY_PREFIXES not set" bug.
            list(APPEND CMAKE_FIND_LIBRARY_PREFIXES "lib" "|")
        endif ()

        list(APPEND EXTRA_TARGET_CFLAGS
             "$<$<CONFIG:Debug>:$<$<BOOL:${DEBUG_WALL}>:/W4>>"
             "$<$<CONFIG:Release>:/W3>"
             # 4100: Unused arg warning.
             "/wd4100"
        )

        ## Uncomment this line if you end up with /NODEFAULTLIB warninigs. You will also
        ## need to build with the '--verbose' flag and check the values of "/M*" flags
        ## (typically you should see /MT or /MTd for the static runtime libraries.)
        ##
        # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /verbose:lib")

        if (WARNINGS_FATAL)
            message(STATUS "WARNINGS_FATAL: Compiler warnings are errors!! (/WX)")
            list(APPEND EXTRA_TARGET_CFLAGS "/WX")
        endif ()
    endif ()
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # The MSVC solution builds as 32-bit, but none of the *nix platforms do.
    #
    # If 32-bit compiles have to be added back, uncomment the following 2 lines:
    #
    # add_compile_options("-m32")
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
endif ()


if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID MATCHES ".*Clang")
    # include(fpintrin)

    # Turn on warnings about strict overflow/potential overflows.
    ## LIST(APPEND EXTRA_TARGET_CFLAGS "-Wall" "-fno-inline" "-fstrict-overflow" "-Wstrict-overflow=3")
    LIST(APPEND EXTRA_TARGET_CFLAGS
        "-U__STRICT_ANSI__"
        "$<$<CONFIG:Debug>:$<$<BOOL:${DEBUG_WALL}>:-Wall>>"
        ## Only add if WARNINGS_FATAL set; has undesirable consequences with LTO.
        "$<$<CONFIG:Release>:-Wall>"
    )

    # 07 NOV 2022: Apparently, -O3 is kosher now.
    #
    # 'O3' optimization and strict overflow cause all kinds of simulator issues, especially inside
    # the VAX simulators. Reduce optimization and ensure strict overflow is turned off.

    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(update_o2 TRUE)
        if (NOT MINGW)
            if (RELEASE_LTO AND (NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release"))
                check_c_compiler_flag("-flto" GCC_LTO_FLAG)
                if (GCC_LTO_FLAG)
                    message(STATUS "Adding LTO to Release compiler and linker flags")
                    set(lto_flag "$<$<CONFIG:Release>:-flto>")
                    list(APPEND EXTRA_TARGET_CFLAGS "${lto_flag}")
                    list(APPEND EXTRA_TARGET_LFLAGS "${lto_flag}")
                    set(update_o2 FALSE)
                else ()
                    message(STATUS "Compiler does not support Link Time Optimization.")
                endif ()
            else ()
                message(STATUS "Link Time Optimization NOT ENABLED.")
            endif ()
        elseif (MINGW)
            message(STATUS "MinGW: Link Time Optimization BROKEN, not added to Release flags")
        endif ()

        if (update_o2)
            message(STATUS "Replacing '-O3' with '-O2'")
            string(REGEX REPLACE "-O3" "-O2" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
            string(REGEX REPLACE "-O3" "-O2" CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
        endif ()

        if (WARNINGS_FATAL` OR RELEASE_LTO)
            check_c_compiler_flag("-Werror" GCC_W_ERROR_FLAG)
            if (GCC_W_ERROR_FLAG)
                if (WARNINGS_FATAL)
                    message(STATUS "WARNINGS_FATAL: Compiler warnings are errors!! (-Werror)")
                    list(APPEND EXTRA_TARGET_CFLAGS "-Werror")
                endif ()
                if (RELEASE_LTO)
                message(STATUS "WARNINGS_FATAL: Link-time optimization warnings are errors!! (-Werror)")
                list(APPEND EXTRA_TARGET_LFLAGS "-Werror")
                endif ()
            endif ()
        endif ()

        message(STATUS "Adding GNU-specific optimizations to CMAKE_C_FLAGS_RELEASE")
        list(APPEND opt_flags "-finline-functions" "-fgcse-after-reload" "-fpredictive-commoning"
                            "-fipa-cp-clone" "-fno-unsafe-loop-optimizations" "-fno-strict-overflow")
    elseif (CMAKE_C_COMPILER_ID MATCHES ".*Clang")
        message(STATUS "Adding Clang-specific optimizations to CMAKE_C_FLAGS_RELEASE")
        list(APPEND opt_flags "-fno-strict-overflow")
    endif()

    foreach (opt_flag ${opt_flags})
        message(STATUS "    ${opt_flag}")
        string(REGEX REPLACE "${opt_flag}[ \t\r\n]*" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
        string(APPEND CMAKE_C_FLAGS_RELEASE " ${opt_flag}")
        string(REGEX REPLACE "${opt_flag}[ \t\r\n]*" "" CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
        string(APPEND CMAKE_C_FLAGS_MINSIZEREL " ${opt_flag}")
    endforeach ()
else ()
    message(STATUS "Not changing CMAKE_C_FLAGS_RELEASE on ${CMAKE_C_COMPILER_ID}")
endif ()


if (CMAKE_HOST_APPLE)
    ## Look for app bundles and frameworks after looking for Unix-style packages:
    set(CMAKE_FIND_FRAMEWORK "LAST")
    set(CMAKE_FIND_APPBUNDLE "LAST")

    if (EXISTS "/usr/local/Cellar" OR EXISTS "/opt/homebrew/Cellar")
        ## Smells like HomeBrew. Bulk add the includes and library subdirectories
        message(STATUS "Adding HomeBrew paths to library and include search")
        set(hb_topdir "/usr/local/Cellar")
        if (EXISTS "/opt/homebrew/Cellar")
            set(hb_topdir "/opt/homebrew/Cellar")
        endif()

        file(GLOB hb_lib_candidates LIST_DIRECTORIES TRUE "${hb_topdir}/*/*/lib")
        file(GLOB hb_include_candidates LIST_DIRECTORIES TRUE "${hb_topdir}/*/*/include")

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
    elseif(EXISTS /opt/local/bin/port)
        # MacPorts
        list(PREPEND CMAKE_LIBRARY_PATH /opt/local/lib)
        list(PREPEND CMAKE_INCLUDE_PATH /opt/local/include)
    endif()

    ## Universal binaries?
    if (MAC_UNIVERSAL)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
    endif ()
endif()
