## 3B2 simulator
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

add_simulator(3b2
    SOURCES
        3b2_cpu.c
        3b2_sys.c
        3b2_rev2_sys.c
        3b2_rev2_mmu.c
        3b2_rev2_mau.c
        3b2_rev2_csr.c
        3b2_rev2_timer.c
        3b2_stddev.c
        3b2_mem.c
        3b2_iu.c
        3b2_if.c
        3b2_id.c
        3b2_dmac.c
        3b2_io.c
        3b2_ports.c
        3b2_ctc.c
        3b2_ni.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        REV2
    FULL64
    LABEL 3B2
    TEST 3b2)
