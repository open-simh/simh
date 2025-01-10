## Various and sundry operating system features.
##
## Author: B. Scott Michel
## "scooter me fecit"

include(CheckSymbolExists)
include(CMakePushCheckState)

include(pthreads-dep)

set(NEED_LIBRT FALSE)

add_library(os_features INTERFACE)

## Editline support?
find_package(EDITLINE)
if (TARGET Editline::Editline)
    target_link_libraries(os_features INTERFACE Editline::Editline)
endif ()

if (WITH_ASYNC)
    ## semaphores and sem_timedwait support (OS feature):
    check_include_file(semaphore.h semaphore_h_found)
    if (semaphore_h_found)
        cmake_push_check_state()

        get_property(zz_thread_defs  TARGET thread_lib PROPERTY INTERFACE_COMPILE_DEFINITIONS)
        get_property(zz_thread_incs  TARGET thread_lib PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
        get_property(zz_thread_lopts TARGET thread_lib PROPERTY INTERFACE_LINK_OPTIONS)
        get_property(zz_thread_libs  TARGET thread_lib PROPERTY INTERFACE_LINK_LIBRARIES)

        list(APPEND CMAKE_REQUIRE_DEFINITIONS ${zz_thread_defs})
        list(APPEND CMAKE_REQUIRED_INCLUDES ${zz_thread_incs})
        list(APPEND CMAKE_REQUIRED_LINK_OPTIONS ${zz_thread_lopts})
        list(APPEND CMAKE_REQUIRED_LIBRARIES ${zz_thread_libs})

        check_symbol_exists(sem_timedwait semaphore.h have_sem_timedwait)

        if (NOT have_sem_timedwait)
            ## Maybe it's in librt, like shm_open (and more likely, it's not.)
            list(APPEND CMAKE_REQUIRED_LIBRARIES rt)
            check_symbol_exists(sem_timedwait semaphore.h have_sem_timedwait_rt)
            if (have_sem_timedwait_rt)
                set(NEED_LIBRT TRUE)
            endif (have_sem_timedwait_rt)
        endif (NOT have_sem_timedwait)

        cmake_pop_check_state()

        if (have_sem_timedwait OR have_sem_timedwait_rt)
            target_compile_definitions(os_features INTERFACE HAVE_SEMAPHORE)
        endif ()
    endif (semaphore_h_found)
endif (WITH_ASYNC)

## Note: We could use this to enforce better type safety with file I/O.
##
## _LARGEFILE64_SOURCE and _FILE_OFFSET_BITS for Linux
## check_type_size(off_t SIZE_OFF_T)
## if (SIZE_OFF_T)
##     target_compile_definitions(os_features INTERFACE SIZE_OFF_T=${SIZE_OFF_T})
## endif ()
##
## check_type_size(off64_t SIZE_OFF64_T)
## if (NOT SIZE_OFF64_T)
##     set(xxx_CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS})
##     list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE=1)
##     check_type_size(off64_t SIZE_OFF64_T)
##     set(xxx_CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS})
## 
##     if (SIZE_OFF64_T)
##         target_compile_definitions(os_features INTERFACE _FILE_OFFSET_BITS=64 _LARGEFILE64_SOURCE=1)
##     endif ()
## endif()
## 
## if (SIZE_OFF64_T)
##     target_compile_definitions(os_features INTERFACE SIZE_OFF64_T=${SIZE_OFF64_T})
## endif ()

if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID MATCHES ".*Clang")
    target_compile_definitions(os_features INTERFACE _GNU_SOURCE)
endif ()

## <sys/ioctl.h>
check_include_file(sys/ioctl.h have_sys_ioctl_h)
if (have_sys_ioctl_h)
    target_compile_definitions(os_features INTERFACE HAVE_SYS_IOCTL)
endif (have_sys_ioctl_h)

## <linux/cdrom.h>
check_include_file(linux/cdrom.h have_linux_cdrom_h)
if (have_linux_cdrom_h)
    target_compile_definitions(os_features INTERFACE HAVE_LINUX_CDROM)
endif (have_linux_cdrom_h)

## <utime.h>
check_include_file(utime.h have_utime_h)
if (have_utime_h)
    target_compile_definitions(os_features INTERFACE HAVE_UTIME)
endif (have_utime_h)

## <glob.h>
check_include_file(glob.h have_glob_h)
if (have_glob_h)
    target_compile_definitions(os_features INTERFACE HAVE_GLOB)
else ()
    ## <fnmatch.h>
    check_include_file(fnmatch.h have_fnmatch_h)
    if (have_fnmatch_h)
        target_compile_definitions(os_features INTERFACE HAVE_FNMATCH)
    endif (have_fnmatch_h)
endif (have_glob_h)

## <sys/mman.h> and shm_open
check_include_file(sys/mman.h have_sys_mman_h)
if (have_sys_mman_h)
    cmake_push_check_state()

    check_symbol_exists(shm_open sys/mman.h have_shm_open)

    if (NOT have_shm_open OR NEED_LIBRT)
        ## Linux: shm_open is in the rt library?
        set(CMAKE_REQUIRED_LIBRARIES rt)
        check_symbol_exists(shm_open sys/mman.h have_shm_open_lrt)
    endif (NOT have_shm_open OR NEED_LIBRT)

    if (have_shm_open OR have_shm_open_lrt)
        target_compile_definitions(os_features INTERFACE HAVE_SHM_OPEN)
    endif (have_shm_open OR have_shm_open_lrt)
    if (have_shm_open_lrt)
        set(NEED_LIBRT TRUE)
    endif (have_shm_open_lrt)

    cmake_pop_check_state()
endif (have_sys_mman_h)

IF (NEED_LIBRT)
    target_link_libraries(os_features INTERFACE rt)
ENDIF (NEED_LIBRT)

check_include_file(dlfcn.h have_dlfcn_h)
if (have_dlfcn_h)
    cmake_push_check_state()

    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_DL_LIBS})
    check_symbol_exists(dlopen dlfcn.h have_dlopen)

    if (have_dlopen)
        target_link_libraries(os_features INTERFACE ${CMAKE_DL_LIBS})

        set(dlext ${CMAKE_SHARED_LIBRARY_SUFFIX})
        string(REPLACE "." "" dlext "${dlext}")
        target_compile_definitions(os_features INTERFACE SIM_HAVE_DLOPEN=${dlext})
    endif (have_dlopen)

    cmake_pop_check_state()
endif (have_dlfcn_h)

if (NOT MSVC AND NOT (WIN32 AND CMAKE_C_COMPILER_ID MATCHES ".*Clang"))
    # Need the math library on non-Windows platforms
    target_link_libraries(os_features INTERFACE m)
endif ()

set(HAVE_TAP_NETWORK False)
set(HAVE_BSDTUNTAP False)

if (WITH_NETWORK)
    ## TAP/TUN devices
    if (WITH_TAP)
        check_include_file(linux/if_tun.h if_tun_found)

        if (NOT if_tun_found)
            check_include_file(net/if_tun.h net_if_tun_found)
            if (net_if_tun_found OR EXISTS /Library/Extensions/tap.kext)
                set(HAVE_BSDTUNTAP True)
            endif (net_if_tun_found OR EXISTS /Library/Extensions/tap.kext)
        endif (NOT if_tun_found)

        if (if_tun_found OR net_if_tun_found)
            set(HAVE_TAP_NETWORK True)
        endif (if_tun_found OR net_if_tun_found)
    endif (WITH_TAP)
endif (WITH_NETWORK)

## Cygwin also wants winmm. Note: Untested but should work.
if (CYGWIN)
  check_library_exists(winmm timeGetTime "" HAS_WINMM)
  if (HAS_WINMM)
    target_link_libraries(os_features INTERFACE ws2_32 wsock32 winmm)
    target_compile_definitions(os_features INTERFACE HAVE_WINMM)
  endif ()
endif ()

## inttypes.h for print formats:
check_include_file(inttypes.h have_inttypes_h)
if (have_inttypes_h)
    target_compile_definitions(os_features INTERFACE HAVE_INTTYPES_H)
endif ()

# libslirp poll/select/etc. detection
set(sim_use_select 0)
set(sim_use_poll   0)

set(check_poll     0)

if (USE_SELECT)
    set(sim_use_select 1)
else ()
    if (NOT WIN32)
        check_symbol_exists(poll "poll.h" have_poll_h)
        if (have_poll_h)
            set(sim_use_poll   1)
        elseif (USE_POLL)
            message(FATAL_ERROR "USE_POLL set, poll.h not detected. Select a different socket polling mechansim.")
        else ()
            set(sim_use_select 1)
        endif()
    else ()
        cmake_push_check_state()
        list(APPEND CMAKE_REQUIRED_LIBRARIES "ws2_32" "wsock32")

        check_symbol_exists(WSAPoll "winsock2.h;windows.h" have_wsa_poll)
        if (have_wsa_poll)
            set(sim_use_poll  1)
        else ()
            set(sim_use_select 1)
        endif ()

        cmake_pop_check_state()
    endif()
endif ()

target_compile_definitions(os_features INTERFACE
    SIM_USE_POLL=${sim_use_poll}
    SIM_USE_SELECT=${sim_use_select}
)

## Windows: winmm (for ms timer functions), socket functions (even when networking is
## disabled. Also squelch the deprecation warnings (these warnings can be enabled
## via the -DWINAPI_DEPRECATION:Bool=On flag at configure time.)
if (WIN32)
    target_link_libraries(os_features INTERFACE ws2_32 wsock32 winmm)
    target_compile_definitions(os_features INTERFACE HAVE_WINMM)
    if (NOT WINAPI_DEPRECATION)
        target_compile_definitions(os_features INTERFACE
            _CRT_NONSTDC_NO_WARNINGS
            _CRT_SECURE_NO_WARNINGS
        )
    endif ()
endif ()

## Sanitizer support
find_package(Sanitizers)
