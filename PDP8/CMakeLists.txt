## PDP8 simulator
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

add_simulator(pdp8
    SOURCES
        pdp8_cpu.c
        pdp8_clk.c
        pdp8_df.c
        pdp8_dt.c
        pdp8_lp.c
        pdp8_mt.c
        pdp8_pt.c
        pdp8_rf.c
        pdp8_rk.c
        pdp8_rx.c
        pdp8_sys.c
        pdp8_tt.c
        pdp8_ttx.c
        pdp8_rl.c
        pdp8_tsc.c
        pdp8_td.c
        pdp8_ct.c
        pdp8_fpp.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LABEL PDP8
    TEST pdp8)
