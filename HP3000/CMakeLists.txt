## HP3000 simulator
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

add_simulator(hp3000
    SOURCES
        hp_disclib.c
        hp_tapelib.c
        hp3000_atc.c
        hp3000_clk.c
        hp3000_cpu.c
        hp3000_cpu_base.c
        hp3000_cpu_fp.c
        hp3000_cpu_cis.c
        hp3000_ds.c
        hp3000_iop.c
        hp3000_lp.c
        hp3000_mem.c
        hp3000_mpx.c
        hp3000_ms.c
        hp3000_scmb.c
        hp3000_sel.c
        hp3000_sys.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LABEL HP3000
    TEST hp3000)
