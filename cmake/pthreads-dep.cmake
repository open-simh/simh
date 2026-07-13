#~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
# Manage the pthreads dependency
#
# (a) Try to locate the system's installed pthreads library, which is very
#     platform dependent (MSVC -> Pthreads4w, MinGW -> pthreads, *nix -> pthreads.)
# (b) MSVC: Build Pthreads4w as a dependent
#~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

add_library(thread_lib INTERFACE)
set(AIO_FLAGS)

if (WITH_ASYNC)
    include(ExternalProject)

    if (MSVC OR (WIN32 AND CMAKE_C_COMPILER_ID MATCHES ".*Clang.*with MSVC-like command-line.*"))
        # Pthreads4w: pthreads for windows.
        if (USING_VCPKG)
            find_package(PThreads4W REQUIRED)
            target_link_libraries(thread_lib INTERFACE PThreads4W::PThreads4W)
            set(THREADING_PKG_STATUS "vcpkg PThreads4W")
        else ()
            find_package(PTW)

            if (PTW_FOUND)
                target_compile_definitions(thread_lib INTERFACE PTW32_STATIC_LIB)
                target_include_directories(thread_lib INTERFACE ${PTW_INCLUDE_DIRS})
                target_link_libraries(thread_lib INTERFACE ${PTW_C_LIBRARY})

                set(THREADING_PKG_STATUS "detected PTW/PThreads4W")
            else ()
                ## Would really like to build from the original jwinarske repo, but it
                ## ends up installing in ${CMAKE_INSTALL_PREFIX}/<x86|x86_64>> prefix.
                ## Which completely breaks how CMake Find*.cmake works.
                ##
                ## set(PTHREADS4W_URL "https://github.com/jwinarske/pthreads4w")
                ## set(PTHREADS4W_URL "https://github.com/bscottm/pthreads4w")
                set(PTHREADS4W_URL "https://github.com/bscottm/pthreads4w/archive/refs/tags/version-3.1.0-release.zip")

                ExternalProject_Add(pthreads4w-ext
                    URL ${PTHREADS4W_URL}
                    CONFIGURE_COMMAND ""
                    BUILD_COMMAND ""
                    INSTALL_COMMAND ""
                )

                BuildDepMatrix(pthreads4w-ext pthreads4w
                    # CMAKE_ARGS
                    #     -DDIST_ROOT=${SIMH_DEP_TOPDIR}
                )

                list(APPEND SIMH_BUILD_DEPS pthreads4w)
                list(APPEND SIMH_DEP_TARGETS pthreads4w-ext)
                message(STATUS "Building Pthreads4w from ${PTHREADS4W_URL}")
                set(THREADING_PKG_STATUS "pthreads4w source build")
            endif ()
        endif ()
    else ()
        # Let CMake determine which threading library ought be used.
        set(THREADS_PREFER_PTHREAD_FLAG On)
        find_package(Threads)
        if (THREADS_FOUND)
          target_link_libraries(thread_lib INTERFACE Threads::Threads)
        endif (THREADS_FOUND)

        set(THREADING_PKG_STATUS "Platform-detected threading support")
    endif ()

    if (THREADS_FOUND OR PTW_FOUND OR PThreads4W_FOUND)
        set(AIO_FLAGS USE_READER_THREAD SIM_ASYNCH_IO)
    else ()
        set(AIO_FLAGS DONT_USE_READER_THREAD)
    endif ()

    if (DONT_USE_AIO_INTRINSICS)
      target_compile_definitions(thread_lib INTERFACE DONT_USE_AIO_INTRINSICS)
    endif ()
else ()
    target_compile_definitions(thread_lib INTERFACE DONT_USE_READER_THREAD)
    set(THREADING_PKG_STATUS "asynchronous I/O disabled.")
endif()
