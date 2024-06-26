## I650 simulator
##
## This is an automagically generated file. Do NOT EDIT.
## Any changes you make will be overwritten!!
##
## Make changes to the SIMH top-level makefile and then run the
## "cmake/generate.py" script to regenerate these files.
##
##     cd cmake; python -m generate --help
##
## ------------------------------------------------------------

if (HAVE_UNITY_FRAMEWORK AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/unit-tests/CMakeLists.txt")
  add_subdirectory(unit-tests)
endif ()

add_simulator(i650
    SOURCES
        i650_cpu.c
        i650_cdr.c
        i650_cdp.c
        i650_dsk.c
        i650_mt.c
        i650_sys.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    FEATURE_INT64
    LABEL I650
    PKG_FAMILY ibm_family
    TEST i650)

if (WIN32)
    if (MSVC)
        set(I650_STACK_FLAG "/STACK:8388608")
    elseif (CMAKE_C_COMPILER_ID MATCHES ".*Clang")
        set(I650_STACK_FLAG "-Xlinker" "/STACK:8388608")
    else ()
        set(I650_STACK_FLAG "-Wl,--stack,8388608")
    endif ()
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.13")
        target_link_options(i650 PUBLIC "${I650_STACK_FLAG}")
    else ()
        set_property(TARGET i650 LINK_FLAGS " ${I650_STACK_FLAG}")
    endif ()
endif()