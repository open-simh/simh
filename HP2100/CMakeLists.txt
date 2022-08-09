## HP2100 simulator
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

add_simulator(hp2100
    SOURCES
        hp2100_baci.c
        hp2100_cpu.c
        hp2100_cpu_fp.c
        hp2100_cpu_fpp.c
        hp2100_cpu0.c
        hp2100_cpu1.c
        hp2100_cpu2.c
        hp2100_cpu3.c
        hp2100_cpu4.c
        hp2100_cpu5.c
        hp2100_cpu6.c
        hp2100_cpu7.c
        hp2100_di.c
        hp2100_di_da.c
        hp2100_disclib.c
        hp2100_dma.c
        hp2100_dp.c
        hp2100_dq.c
        hp2100_dr.c
        hp2100_ds.c
        hp2100_ipl.c
        hp2100_lps.c
        hp2100_lpt.c
        hp2100_mc.c
        hp2100_mem.c
        hp2100_mpx.c
        hp2100_ms.c
        hp2100_mt.c
        hp2100_mux.c
        hp2100_pif.c
        hp2100_pt.c
        hp2100_sys.c
        hp2100_tbg.c
        hp2100_tty.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        HAVE_INT64
    LABEL HP2100
    TEST hp2100)
