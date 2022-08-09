## Interdata simulators
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

add_simulator(id16
    SOURCES
        ${ID16D}/id16_cpu.c
        ${ID16D}/id16_sys.c
        ${ID16D}/id_dp.c
        ${ID16D}/id_fd.c
        ${ID16D}/id_fp.c
        ${ID16D}/id_idc.c
        ${ID16D}/id_io.c
        ${ID16D}/id_lp.c
        ${ID16D}/id_mt.c
        ${ID16D}/id_pas.c
        ${ID16D}/id_pt.c
        ${ID16D}/id_tt.c
        ${ID16D}/id_uvc.c
        ${ID16D}/id16_dboot.c
        ${ID16D}/id_ttp.c
    INCLUDES
        ${ID16D}
    LABEL Interdata
    TEST id16)

add_simulator(id32
    SOURCES
        id32_cpu.c
        id32_sys.c
        id_dp.c
        id_fd.c
        id_fp.c
        id_idc.c
        id_io.c
        id_lp.c
        id_mt.c
        id_pas.c
        id_pt.c
        id_tt.c
        id_uvc.c
        id32_dboot.c
        id_ttp.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LABEL Interdata
    TEST id32)
