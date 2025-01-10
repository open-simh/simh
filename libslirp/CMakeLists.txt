if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message("")
  message("*** Do NOT build or generate CMake artifacts in the source directory! ***")
  message("")
  message("Create a subdirectory and build in that subdirectory, e.g.:")
  message("")
  message("  $ mkdir cmake-build")
  message("  $ cd cmake-build")
  message("  $ cmake -G \"your generator here\" ..")
  message("")
  message(FATAL_ERROR "Preventing in-tree source build.")
endif ()

cmake_minimum_required(VERSION 3.10)

set(SLIRP_MAJOR_VERSION 4)
set(SLIRP_MINOR_VERSION 8)
set(SLIRP_MICRO_VERSION 0)
set(SLIRP_VERSION "${SLIRP_MAJOR_VERSION}.${SLIRP_MINOR_VERSION}.${SLIRP_MICRO_VERSION}")
set(SLIRP_VERSION_STRING "\"${SLIRP_VERSION}\"")

project(libslirp VERSION "${SLIRP_VERSION}" LANGUAGES C)

include(CheckSymbolExists)

## Use Release as the default build configuration when not otherwise
## specified
if (NOT CMAKE_CONFIGURATION_TYPES)
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
        message(STATUS "CMAKE_BUILD_TYPE defaulted to ${CMAKE_BUILD_TYPE}")
    endif ()
endif ()

option(NOGLIB "Build libslirp with minimal GLib replacement." TRUE)
option(WALL "Turn on full compiler warnings." TRUE)
option(TARGET_WINVER "Specify a specific Windows version, e.g., 0x0601 for Windows 7" "")

set(LIBSLIRP_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/arp_table.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bootp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cksum.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dhcpv6.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dnssearch.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/if.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ip6_icmp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ip6_input.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ip6_output.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ip_icmp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ip_input.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ip_output.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mbuf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/misc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ncsi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ndp_table.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sbuf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/slirp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/socket.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/state.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/stream.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tcp_input.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tcp_output.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tcp_subr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tcp_timer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tftp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/udp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/udp6.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vmstate.c)

if (NOGLIB)
    list(APPEND LIBSLIRP_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/minimal/glib-stubs.c)
endif ()

set(EXTRA_TARGET_CFLAGS "")

if (WALL)
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        LIST(APPEND EXTRA_TARGET_CFLAGS "$<$<BOOL:${WALL}>:-Wall>" "$<$<BOOL:${WALL}>:-Wformat>")
    elseif (CMAKE_C_COMPILER_ID MATCHES ".*Clang")
        if (NOT WIN32)
            LIST(APPEND EXTRA_TARGET_CFLAGS "$<$<BOOL:${WALL}>:-Wall>" "$<$<BOOL:${WALL}>:-Wformat>")
        else ()
            LIST(APPEND EXTRA_TARGET_CFLAGS "$<$<BOOL:${WALL}>:-Wall>" "$<$<BOOL:${WALL}>:-Wformat>")
        endif ()
    elseif(MSVC)
        list(APPEND EXTRA_TARGET_CFLAGS "$<$<BOOL:${WALL}>:/W3>")
    endif ()
endif ()

add_library(slirp SHARED ${LIBSLIRP_SRCS})
add_library(slirp_static STATIC ${LIBSLIRP_SRCS})

set(EXTRA_C_DEFS "")

## If we ever have to restore CMAKE_REQUIRED_DEFINITIONS and CMAKE_REQUIRED_LIBRARIES
set(saved_cmake_required_definitions ${CMAKE_REQUIRED_DEFINITIONS})
set(saved_cmake_required_libraries ${CMAKE_REQUIRED_LIBRARIES})

if (WIN32)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS "-DWIN32_LEAN_AND_MEAN")
    list(APPEND CMAKE_REQUIRED_LIBRARIES ws2_32 iphlpapi)
endif ()

if (NOT NOGLIB)
    ## Find GLib and use it
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
    if (NOT GLIB_FOUND)
        pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib)
    endif ()

    if (GLIB_FOUND)
        target_compile_options(slirp INTERFACE ${GLIB_CFLAGS})
        target_include_directories(slirp PUBLIC ${GLIB_INCLUDE_DIRS})
        target_include_directories(slirp INTERFACE ${GLIB_INCLUDE_DIRS})
        target_link_libraries(slirp PUBLIC PkgConfig::GLIB)
        target_link_libraries(slirp INTERFACE PkgConfig::GLIB)
        target_link_directories(slirp_static INTERFACE ${GLIB_STATIC_LIBRARY_DIRS})

        target_compile_options(slirp_static INTERFACE ${GLIB_STATIC_CFLAGS})
        target_include_directories(slirp_static PUBLIC ${GLIB_STATIC_INCLUDE_DIRS})
        target_include_directories(slirp_static INTERFACE ${GLIB_STATIC_INCLUDE_DIRS})
        target_link_libraries(slirp_static PUBLIC ${GLIB_STATIC_LIBRARIES})
        target_link_libraries(slirp_static INTERFACE ${GLIB_STATIC_LIBRARIES})
        target_link_directories(slirp_static INTERFACE ${GLIB_STATIC_LIBRARY_DIRS})
    else ()
        message(FATAL_ERROR "NOGLIB is False, but pkg-config cannot find glib-2.0 or glib.")
    endif ()
else ()
    # Prefer clock_gettime over gettimeofday:
    check_symbol_exists(clock_gettime time.h HAVE_CLOCK_GETTIME)

    if (HAVE_CLOCK_GETTIME)
        list(APPEND EXTRA_C_DEFS HAVE_TIME_H HAVE_CLOCK_GETTIME)
    else ()
        check_symbol_exists(gettimeofday sys/time.h SYS_TIME_H_GETTIMEOFDAY)
        if (SYS_TIME_H_GETTIMEOFDAY)
            list(APPEND EXTRA_C_DEFS HAVE_SYS_TIME_H HAVE_GETTIMEOFDAY)
        endif ()
    endif ()

    check_symbol_exists(vasprintf stdio.h HAVE_VASPRINTF)
    if (HAVE_VASPRINTF)
        list(APPEND EXTRA_C_DEFS HAVE_VASPRINTF)
    else ()
        if (NOT HAVE_VASPRINTF)
            set(save_cmake_reqd_defs ${CMAKE_REQUIRED_DEFINITIONS})
            list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
            check_symbol_exists(vasprintf stdio.h HAVE_VASPRINTF_GNU_SOURCE)
            set(CMAKE_REQUIRED_DEFINITIONS ${save_cmake_reqd_defs})

            if (HAVE_VASPRINTF_GNU_SOURCE)
                list(APPEND EXTRA_C_DEFS HAVE_VASPRINTF _GNU_SOURCE)
            endif ()
        endif ()
    endif ()

    # Create the endianness defines:
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.20")
        set(G_BYTE_ORDER "G_LITTLE_ENDIAN")
        if (CMAKE_C_BYTE_ORDER EQUAL "BIG_ENDIAN")
            set(G_BYTE_ORDER "G_BIG_ENDIAN")
        endif ()
    else ()
        include(TestBigEndian)
        test_big_endian(BYTE_ORDER)
        set(G_BYTE_ORDER "G_LITTLE_ENDIAN")
        if (BYTE_ORDER EQUAL 1)
            set(G_BYTE_ORDER "G_BIG_ENDIAN")
        endif ()
     endif ()

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/minimal/glib-endian.cmake"
                   "${CMAKE_CURRENT_BINARY_DIR}/build-include/glib-endian.h")

    ## Include directories to build:
    target_include_directories(slirp BEFORE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/minimal")
    target_include_directories(slirp_static BEFORE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/minimal")

    ## Include directories for users:
    target_include_directories(slirp BEFORE INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/minimal")
    target_include_directories(slirp_static BEFORE INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/minimal")
endif ()

# Create the version file in the build directory's build-include subdir:
file(READ src/libslirp-version.h.in SLIRP_VERSION_FILE)
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/build-include/libslirp-version.h" CONTENT "${SLIRP_VERSION_FILE}" @ONLY)
set(SLIRP_VERSION_FILE)

if (WIN32)
    check_symbol_exists(inet_pton "winsock2.h;windows.h;ws2tcpip.h" HAVE_INET_PTON)

    target_link_libraries(slirp PUBLIC ws2_32 iphlpapi)
    target_link_libraries(slirp_static PUBLIC ws2_32 iphlpapi)
else ()
    check_symbol_exists(inet_pton "arpa/inet.h" HAVE_INET_PTON)
endif ()

if (HAVE_INET_PTON)
    list(APPEND EXTRA_C_DEFS HAVE_INET_PTON)
endif ()

## macOS needs -lresolv:
if (APPLE)
    target_link_libraries(slirp PUBLIC resolv)
    target_link_libraries(slirp_static PUBLIC resolv)
endif ()

if (NOGLIB)
    ## ISO C safety functions... nice to use when available (and reduces the "deprecation"
    ## warnings on Windows. Will need updates on other platforms )
    set(saved_iso_c_required_defs ${CMAKE_REQUIRED_DEFINITIONS})
    list(APPEND CMAKE_REQUIRED_DEFINITIONS "-D__STDC_WANT_LIB_EXT1__=1" "-D__STDC_WANT_LIB_EXT2__=1")

    check_symbol_exists(_sopen_s io.h HAVE__SOPEN_S)
    if (HAVE__SOPEN_S)
        list(APPEND EXTRA_C_DEFS HAVE__SOPEN_S)
    endif ()

    check_symbol_exists(_lseek io.h HAVE__LSEEK)
    if (HAVE__LSEEK)
        list(APPEND EXTRA_C_DEFS HAVE__LSEEK)
    endif ()

    check_symbol_exists(_read io.h HAVE__READ)
    if (HAVE__READ)
        list(APPEND EXTRA_C_DEFS HAVE__READ)
    endif ()

    check_symbol_exists(_close io.h HAVE__CLOSE)
    if (HAVE__CLOSE)
        list(APPEND EXTRA_C_DEFS HAVE__CLOSE)
    endif ()

    check_symbol_exists(_strdup string.h HAVE__STRDUP_S)
    if (HAVE__STRDUP_S)
        list(APPEND EXTRA_C_DEFS HAVE__STRDUP_S)
    endif ()

    check_symbol_exists(_dupenv_s stdlib.h HAVE_DUPENV_S)
    if (HAVE_DUPENV_S)
        list(APPEND EXTRA_C_DEFS HAVE_DUPENV_S)
    endif ()

    check_symbol_exists(strcpy_s string.h HAVE_STRCPY_S)
    if (HAVE_STRCPY_S)
        list(APPEND EXTRA_C_DEFS HAVE_STRCPY_S)
    endif ()

    check_symbol_exists(strerror_s string.h HAVE_STRERROR_S)
    if (HAVE_STRERROR_S)
        list(APPEND EXTRA_C_DEFS HAVE_STRERROR_S)
    endif ()

    check_symbol_exists(sscanf_s stdio.h HAVE_SSCANF_S)
    if (HAVE_SSCANF_S)
        list(APPEND EXTRA_C_DEFS HAVE_SSCANF_S)
    endif ()

    set(CMAKE_REQUIRED_DEFINITIONS ${saved_iso_c_required_defs})
endif ()

target_compile_definitions(slirp PRIVATE G_LOG_DOMAIN="Slirp" BUILDING_LIBSLIRP
                           ${EXTRA_C_DEFS})
target_compile_definitions(slirp_static PRIVATE G_LOG_DOMAIN="Slirp" BUILDING_LIBSLIRP
                           slirp_static ${EXTRA_C_DEFS})
target_compile_definitions(slirp_static PUBLIC LIBSLIRP_STATIC)

if (TARGET_WINVER)
    target_compile_definitions(slirp PRIVATE "WINVER=${TARGET_WINVER}" "_WIN32_WINNT=${TARGET_WINVER}")
    target_compile_definitions(slirp_static PRIVATE "WINVER=${TARGET_WINVER}" "_WIN32_WINNT=${TARGET_WINVER}")
endif ()

## Include directories required to build:
target_include_directories(
    slirp
    AFTER
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_BINARY_DIR}/build-include"
)
target_include_directories(
    slirp_static
    AFTER
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_BINARY_DIR}/build-include")

## Include directories for the user:
target_include_directories(
    slirp
    AFTER
    INTERFACE
        "${CMAKE_CURRENT_BINARY_DIR}/build-include"
)
target_include_directories(
    slirp_static
    AFTER
    INTERFACE
    "${CMAKE_CURRENT_BINARY_DIR}/build-include"
)

target_compile_options(slirp PRIVATE ${EXTRA_TARGET_CFLAGS})
target_compile_options(slirp_static PRIVATE ${EXTRA_TARGET_CFLAGS})

### Helper function to build test executables:

function(fab_test_executable _targ)
    cmake_parse_arguments(SLIRP "USE_SHARED;NOLIB" "" "SOURCES" ${ARGN})

    add_executable(${_targ} ${SLIRP_SOURCES})
    target_compile_definitions(${_targ} PRIVATE ${EXTRA_C_DEFS})
    target_include_directories(${_targ} BEFORE PRIVATE "src" "${CMAKE_CURRENT_BINARY_DIR}/build-include")
    if (NOGLIB)
        target_include_directories(slirp AFTER PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
        target_include_directories(${_targ} BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/minimal")
        target_include_directories(slirp_static AFTER PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/build-include")
    endif ()

    target_compile_options(${_targ} PRIVATE ${EXTRA_TARGET_CFLAGS})

    if (NOT SLIRP_NOLIB)
        if (SLIRP_USE_SHARED)
            target_link_libraries(${_targ} PUBLIC slirp)
        else ()
            target_link_libraries(${_targ} PUBLIC slirp_static)
        endif ()
    endif ()

    if (WIN32)
        target_link_libraries(${_targ} PUBLIC ws2_32 iphlpapi)
    endif ()
endfunction()


### Tests:

include(CTest)

## pingtest:
fab_test_executable(pingtest SOURCES "test/pingtest.c")
add_test(NAME "pingtest" COMMAND pingtest)

## ncsitest:
fab_test_executable(ncsitest SOURCES "test/ncsitest.c")
add_test(NAME "ncsitest" COMMAND ncsitest)

if (NOGLIB)
    ## String operations
    fab_test_executable(stringops_test USE_SHARED NOLIB SOURCES "minimal/tests/stringops_test.c" "minimal/glib-stubs.c")
    add_test(NAME "minimal:stringops" COMMAND stringops_test)

    ## strstr_test:
    fab_test_executable(strstr_test USE_SHARED NOLIB SOURCES "minimal/tests/strstr_len_test.c" "minimal/glib-stubs.c")
    add_test(NAME "minimal:strstr" COMMAND strstr_test)

    ## Debug parser
    fab_test_executable(parsedebug_test NOLIB SOURCES "minimal/tests/parsedebug.c" "minimal/glib-stubs.c")
    add_test(NAME "minimal:parse_debug" COMMAND parsedebug_test)

    # GRand random numbers.
    fab_test_executable(grand_test NOLIB SOURCES "minimal/tests/grand_test.c" "minimal/glib-stubs.c")
    add_test(NAME "minimal:grand_test" COMMAND grand_test)
endif ()

