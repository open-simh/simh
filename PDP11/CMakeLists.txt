## PDP11 simulators
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

if (BUILD_WITH_VIDEO)
    add_simulator(pdp11
        SOURCES
            pdp11_fp.c
            pdp11_cpu.c
            pdp11_dz.c
            pdp11_cis.c
            pdp11_lp.c
            pdp11_rk.c
            pdp11_rl.c
            pdp11_rp.c
            pdp11_rx.c
            pdp11_stddev.c
            pdp11_sys.c
            pdp11_tc.c
            pdp11_tm.c
            pdp11_ts.c
            pdp11_io.c
            pdp11_rq.c
            pdp11_tq.c
            pdp11_pclk.c
            pdp11_ry.c
            pdp11_pt.c
            pdp11_hk.c
            pdp11_xq.c
            pdp11_xu.c
            pdp11_vh.c
            pdp11_rh.c
            pdp11_tu.c
            pdp11_cpumod.c
            pdp11_cr.c
            pdp11_rf.c
            pdp11_dl.c
            pdp11_ta.c
            pdp11_rc.c
            pdp11_kg.c
            pdp11_ke.c
            pdp11_dc.c
            pdp11_dmc.c
            pdp11_kmc.c
            pdp11_dup.c
            pdp11_rs.c
            pdp11_vt.c
            pdp11_td.c
            pdp11_io_lib.c
            pdp11_rom.c
            pdp11_ch.c
            ${DISPLAYVT}
            pdp11_ng.c
            pdp11_daz.c
            ${DISPLAYNG}
        INCLUDES
            ${CMAKE_CURRENT_SOURCE_DIR}
        DEFINES
            VM_PDP11
        VIDEO
        LABEL PDP11
        TEST pdp11)
endif (BUILD_WITH_VIDEO)

add_simulator(uc15
    SOURCES
        ${UC15D}/pdp11_cis.c
        ${UC15D}/pdp11_cpu.c
        ${UC15D}/pdp11_cpumod.c
        ${UC15D}/pdp11_cr.c
        ${UC15D}/pdp11_fp.c
        ${UC15D}/pdp11_io.c
        ${UC15D}/pdp11_io_lib.c
        ${UC15D}/pdp11_lp.c
        ${UC15D}/pdp11_rh.c
        ${UC15D}/pdp11_rk.c
        ${UC15D}/pdp11_stddev.c
        ${UC15D}/pdp11_sys.c
        ${UC15D}/pdp11_uc15.c
    INCLUDES
        ${UC15D}
        ${PDP18BD}
    DEFINES
        VM_PDP11
        UC15
    LABEL PDP11
    TEST uc15)
