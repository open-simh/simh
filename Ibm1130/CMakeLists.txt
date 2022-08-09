## Ibm1130 simulator
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

add_simulator(ibm1130
    SOURCES
        ibm1130_cpu.c
        ibm1130_cr.c
        ibm1130_disk.c
        ibm1130_stddev.c
        ibm1130_sys.c
        ibm1130_gdu.c
        ibm1130_gui.c
        ibm1130_prt.c
        ibm1130_fmt.c
        ibm1130_ptrp.c
        ibm1130_plot.c
        ibm1130_sca.c
        ibm1130_t2741.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LABEL Ibm1130
    TEST ibm1130)
