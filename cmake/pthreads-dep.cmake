#~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
# Manage the pthreads dependency
#
# (a) Try to locate the system's installed pthreads library, which is very
#     platform dependent (MSVC -> Pthreads4w, MinGW -> pthreads, *nix -> pthreads.)
# (b) MSVC: Build Pthreads4w as a dependent
#~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

add_library(thread_lib INTERFACE)

if (WITH_ASYNC)
    include(ExternalProject)

    if (MSVC)
        # Pthreads4w: pthreads for windows
        include (FindPthreads4w)

        if (PTW_FOUND)
            target_compile_definitions(thread_lib INTERFACE PTW32_STATIC_LIB)
            target_include_directories(thread_lib INTERFACE ${PTW_INCLUDE_DIRS})
            target_link_libraries(thread_lib INTERFACE ${PTW_C_LIBRARY})

            set(THREADING_PKG_STATUS "installed pthreads4w")
        else (PTW_FOUND)
            ## Would really like to build from the original jwinarske repo, but it
            ## ends up installing in ${CMAKE_INSTALL_PREFIX}/<x86|x86_64>> prefix.
            ## Which completely breaks how CMake Find*.cmake work.
            ##
            ## set(PTHREADS4W_URL "https://github.com/jwinarske/pthreads4w")
            set(PTHREADS4W_URL "https://github.com/bscottm/pthreads4w")

            ExternalProject_Add(pthreads4w-ext
                GIT_REPOSITORY ${PTHREADS4W_URL}
                GIT_TAG mingw
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
        endif (PTW_FOUND)
    else ()
        # Let CMake determine which threading library ought be used.
        set(THREADS_PREFER_PTHREAD_FLAG On)
        find_package(Threads)
        if (THREADS_FOUND)
              target_link_libraries(thread_lib INTERFACE Threads::Threads)
        endif (THREADS_FOUND)

        set(THREADING_PKG_STATUS "Platform-detected threading support")
    endif (MSVC)

    if (THREADS_FOUND OR PTW_FOUND)
        target_compile_definitions(thread_lib INTERFACE USE_READER_THREAD SIM_ASYNCH_IO)
    else ()
        target_compile_definitions(thread_lib INTERFACE DONT_USE_READER_THREAD)
    endif ()
else (WITH_ASYNC)
    target_compile_definitions(thread_lib INTERFACE DONT_USE_READER_THREAD)
    set(THREADING_PKG_STATUS "asynchronous I/O disabled.")
endif (WITH_ASYNC)
