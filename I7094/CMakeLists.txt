## I7094 simulator
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

add_simulator(i7094
    SOURCES
        i7094_cpu.c
        i7094_cpu1.c
        i7094_io.c
        i7094_cd.c
        i7094_clk.c
        i7094_com.c
        i7094_drm.c
        i7094_dsk.c
        i7094_sys.c
        i7094_lp.c
        i7094_mt.c
        i7094_binloader.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    INT64
    LABEL I7094
    TEST i7094)
