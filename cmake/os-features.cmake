## Various and sundry operating system features.
##
## Author: B. Scott Michel
## "scooter me fecit"

include(CheckSymbolExists)
include(CMakePushCheckState)

include(pthreads-dep)

set(NEED_LIBRT FALSE)

add_library(os_features INTERFACE)
add_library(simh_ncurses INTERFACE)

## ncurses library:
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses)

if (CURSES_HAVE_NCURSES_H)
    target_compile_definitions(simh_ncurses INTERFACE HAVE_NCURSES)
    target_include_directories(simh_ncurses INTERFACE "${CURSES_INCLUDE_DIRS}")
    target_link_libraries(simh_ncurses INTERFACE "${CURSES_LIBRARIES}")
    set(NCURSES_PKG_STATUS "installed ncurses")
elseif (WIN32)
    set(NCURSES_PKG_STATUS "not supported on Win32")
else (CURSES_HAVE_NCURSES_H)
    set(NCURSES_PKG_STATUS "not found")
endif (CURSES_HAVE_NCURSES_H)

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
endif (have_glob_h)

## <fnmatch.h>
check_include_file(fnmatch.h have_fnmatch_h)
if (have_fnmatch_h)
    target_compile_definitions(os_features INTERFACE HAVE_FNMATCH)
endif (have_fnmatch_h)

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

if (NOT MSVC)
    # Need the math library on non-Windows platforms
    target_link_libraries(os_features INTERFACE m)
endif (NOT MSVC)

set(NETWORK_TUN_DEFS)

if (WITH_NETWORK)
    ## TAP/TUN devices
    if (WITH_TAP)
        check_include_file(linux/if_tun.h if_tun_found)

        if (NOT if_tun_found)
            check_include_file(net/if_tun.h net_if_tun_found)
            if (net_if_tun_found OR EXISTS /Library/Extensions/tap.kext)
                list(APPEND NETWORK_TUN_DEFS HAVE_BSDTUNTAP)
            endif (net_if_tun_found OR EXISTS /Library/Extensions/tap.kext)
        endif (NOT if_tun_found)

        if (if_tun_found OR net_if_tun_found)
            list(APPEND NETWORK_PKG_STATUS "TAP/TUN interface")
        endif (if_tun_found OR net_if_tun_found)
    endif (WITH_TAP)
endif (WITH_NETWORK)

## Windows: winmm (for ms timer functions)
## Linux: large file support
if (WIN32)
    target_link_libraries(os_features INTERFACE winmm)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_compile_definitions(os_features INTERFACE _LARGEFILE64_SOURCE _FILE_OFFSET_BITS=64)
endif ()
