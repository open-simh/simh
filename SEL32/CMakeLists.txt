## SEL32 simulator
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

add_simulator(sel32
    SOURCES
        sel32_cpu.c
        sel32_sys.c
        sel32_chan.c
        sel32_iop.c
        sel32_com.c
        sel32_con.c
        sel32_clk.c
        sel32_mt.c
        sel32_lpr.c
        sel32_scfi.c
        sel32_fltpt.c
        sel32_disk.c
        sel32_hsdp.c
        sel32_mfp.c
        sel32_scsi.c
        sel32_ec.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        USE_INT32
        SEL32
    LABEL SEL32
    TEST sel32)
