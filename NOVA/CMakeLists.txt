## NOVA simulators
##
## This is an automagically generated file. Do NOT EDIT.
## Any changes you make will be overwritten!!
##
## Make changes to the SIMH top-level makefile and then run the
## "cmake/generate.py" script to regenerate these files.
##
##     python -m cmake/generate --help
##
## ------------------------------------------------------------

if (HAVE_UNITY_FRAMEWORK AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/unit-tests/CMakeLists.txt")
  add_subdirectory(unit-tests)
endif ()

add_simulator(eclipse
    SOURCES
        eclipse_cpu.c
        eclipse_tt.c
        nova_sys.c
        nova_dkp.c
        nova_dsk.c
        nova_lp.c
        nova_mta.c
        nova_plt.c
        nova_pt.c
        nova_clk.c
        nova_tt1.c
        nova_qty.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        ECLIPSE
    INT64
    LABEL NOVA
    TEST eclipse)

add_simulator(nova
    SOURCES
        nova_sys.c
        nova_cpu.c
        nova_dkp.c
        nova_dsk.c
        nova_lp.c
        nova_mta.c
        nova_plt.c
        nova_pt.c
        nova_clk.c
        nova_tt.c
        nova_tt1.c
        nova_qty.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LABEL NOVA
    TEST nova)
