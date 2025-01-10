## PDP10 simulators
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

add_simulator(pdp10
    SOURCES
        pdp10_fe.c
        ${PDP11D}/pdp11_dz.c
        pdp10_cpu.c
        pdp10_ksio.c
        pdp10_lp20.c
        pdp10_mdfp.c
        pdp10_pag.c
        pdp10_rp.c
        pdp10_sys.c
        pdp10_tim.c
        pdp10_tu.c
        pdp10_xtnd.c
        ${PDP11D}/pdp11_pt.c
        ${PDP11D}/pdp11_ry.c
        ${PDP11D}/pdp11_cr.c
        ${PDP11D}/pdp11_dup.c
        ${PDP11D}/pdp11_dmc.c
        ${PDP11D}/pdp11_kmc.c
        ${PDP11D}/pdp11_xu.c
        ${PDP11D}/pdp11_ch.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PDP11D}
    DEFINES
        VM_PDP10
    FEATURE_INT64
    USES_AIO
    LABEL PDP10
    NO_INSTALL
    TEST pdp10)


option(PANDA_LIGHTS
       "Enable (=1)/disable (=0) KA-10/KI-11 simulator's Panda display. (def: disabled)"
       FALSE)
option(PIDP10
       "Enable (=1)/disable (=0) PIDP10 display options (def: disabled)"
       FALSE)

### Ensure that the options are mutually exclusive:
if (PANDA_LIGHTS AND PIDP10)
  message(FATAL_ERROR "PANDA_LIGHTS and PIDP10 options are mutually exclusive. Choose one.")
endif ()


add_simulator(pdp10-ka
    SOURCES
        ${KA10D}/kx10_cpu.c
        ${KA10D}/kx10_sys.c
        ${KA10D}/kx10_df.c
        ${KA10D}/kx10_dp.c
        ${KA10D}/kx10_mt.c
        ${KA10D}/kx10_cty.c
        ${KA10D}/kx10_lp.c
        ${KA10D}/kx10_pt.c
        ${KA10D}/kx10_dc.c
        ${KA10D}/kx10_rp.c
        ${KA10D}/kx10_rc.c
        ${KA10D}/kx10_dt.c
        ${KA10D}/kx10_dk.c
        ${KA10D}/kx10_cr.c
        ${KA10D}/kx10_cp.c
        ${KA10D}/kx10_tu.c
        ${KA10D}/kx10_rs.c
        ${KA10D}/ka10_pd.c
        ${KA10D}/kx10_rh.c
        ${KA10D}/kx10_imp.c
        ${KA10D}/ka10_tk10.c
        ${KA10D}/ka10_mty.c
        ${KA10D}/ka10_imx.c
        ${KA10D}/ka10_ch10.c
        ${KA10D}/ka10_stk.c
        ${KA10D}/ka10_ten11.c
        ${KA10D}/ka10_auxcpu.c
        ${KA10D}/ka10_pmp.c
        ${KA10D}/ka10_dkb.c
        ${KA10D}/pdp6_dct.c
        ${KA10D}/pdp6_dtc.c
        ${KA10D}/pdp6_mtc.c
        ${KA10D}/pdp6_dsk.c
        ${KA10D}/pdp6_dcs.c
        ${KA10D}/ka10_dpk.c
        ${KA10D}/kx10_dpy.c
        ${KA10D}/ka10_ai.c
        ${KA10D}/ka10_iii.c
        ${KA10D}/kx10_disk.c
        ${KA10D}/ka10_pclk.c
        ${KA10D}/ka10_tv.c
        ${KA10D}/ka10_dd.c
        ${KA10D}/kx10_ddc.c
        ${DISPLAY340}
        ${DISPLAYIII}
    INCLUDES
        ${KA10D}
    DEFINES
        KA=1
    FEATURE_INT64
    FEATURE_VIDEO
    FEATURE_DISPLAY
    USES_AIO
    LABEL PDP10
    PKG_FAMILY pdp10_family
    TEST ka10)

if (PANDA_LIGHTS)
  target_sources(pdp10-ka PUBLIC ${PDP10D}/kx10_lights.c)
  target_compile_definitions(pdp10-ka PUBLIC PANDA_LIGHTS)
  target_link_libraries(pdp10-ka PUBLIC usb-1.0)
endif ()
if (PIDP10)
  target_sources(pdp10-ka PUBLIC ${PDP10D}/ka10_pipanel.c)
  target_compile_definitions(pdp10-ka PUBLIC PIDP10=1)
endif ()

add_simulator(pdp10-ki
    SOURCES
        ${KI10D}/kx10_cpu.c
        ${KI10D}/kx10_sys.c
        ${KI10D}/kx10_df.c
        ${KI10D}/kx10_dp.c
        ${KI10D}/kx10_mt.c
        ${KI10D}/kx10_cty.c
        ${KI10D}/kx10_lp.c
        ${KI10D}/kx10_pt.c
        ${KI10D}/kx10_dc.c
        ${KI10D}/kx10_rh.c
        ${KI10D}/kx10_rp.c
        ${KI10D}/kx10_rc.c
        ${KI10D}/kx10_dt.c
        ${KI10D}/kx10_dk.c
        ${KI10D}/kx10_cr.c
        ${KI10D}/kx10_cp.c
        ${KI10D}/kx10_tu.c
        ${KI10D}/kx10_rs.c
        ${KI10D}/kx10_imp.c
        ${KI10D}/kx10_dpy.c
        ${KI10D}/kx10_disk.c
        ${KI10D}/kx10_ddc.c
        ${KI10D}/kx10_tym.c
        ${DISPLAY340}
    INCLUDES
        ${KI10D}
    DEFINES
        KI=1
    FEATURE_INT64
    FEATURE_VIDEO
    FEATURE_DISPLAY
    USES_AIO
    LABEL PDP10
    PKG_FAMILY pdp10_family
    TEST ki10)

add_simulator(pdp10-kl
    SOURCES
        ${KL10D}/kx10_cpu.c
        ${KL10D}/kx10_sys.c
        ${KL10D}/kx10_df.c
        ${KA10D}/kx10_dp.c
        ${KA10D}/kx10_mt.c
        ${KA10D}/kx10_lp.c
        ${KA10D}/kx10_pt.c
        ${KA10D}/kx10_dc.c
        ${KL10D}/kx10_rh.c
        ${KA10D}/kx10_dt.c
        ${KA10D}/kx10_cr.c
        ${KA10D}/kx10_cp.c
        ${KL10D}/kx10_rp.c
        ${KL10D}/kx10_tu.c
        ${KL10D}/kx10_rs.c
        ${KL10D}/kx10_imp.c
        ${KL10D}/kl10_fe.c
        ${KL10D}/ka10_pd.c
        ${KL10D}/ka10_ch10.c
        ${KL10D}/kl10_nia.c
        ${KL10D}/kx10_disk.c
    INCLUDES
        ${KL10D}
    DEFINES
        KL=1
    FEATURE_INT64
    USES_AIO
    LABEL PDP10
    PKG_FAMILY pdp10_family
    TEST kl10)

add_simulator(pdp10-ks
    SOURCES
        ${KS10D}/kx10_cpu.c
        ${KS10D}/kx10_sys.c
        ${KS10D}/kx10_disk.c
        ${KS10D}/ks10_cty.c
        ${KS10D}/ks10_uba.c
        ${KS10D}/kx10_rh.c
        ${KS10D}/kx10_rp.c
        ${KS10D}/kx10_tu.c
        ${KS10D}/ks10_dz.c
        ${KS10D}/ks10_tcu.c
        ${KS10D}/ks10_lp.c
        ${KS10D}/ks10_ch11.c
        ${KS10D}/ks10_kmc.c
        ${KS10D}/ks10_dup.c
        ${KS10D}/kx10_imp.c
    INCLUDES
        ${KS10D}
        ${PDP11D}
    DEFINES
        KS=1
    FEATURE_INT64
    USES_AIO
    LABEL PDP10
    PKG_FAMILY pdp10_family
    TEST ks10)

add_simulator(pdp6
    SOURCES
        ${PDP6D}/kx10_cpu.c
        ${PDP6D}/kx10_sys.c
        ${PDP6D}/kx10_cty.c
        ${PDP6D}/kx10_lp.c
        ${PDP6D}/kx10_pt.c
        ${PDP6D}/kx10_cr.c
        ${PDP6D}/kx10_cp.c
        ${PDP6D}/pdp6_dct.c
        ${PDP6D}/pdp6_dtc.c
        ${PDP6D}/pdp6_mtc.c
        ${PDP6D}/pdp6_dsk.c
        ${PDP6D}/pdp6_dcs.c
        ${PDP6D}/kx10_dpy.c
        ${PDP6D}/pdp6_slave.c
        ${PDP6D}/pdp6_ge.c
        ${DISPLAY340}
    INCLUDES
        ${PDP6D}
    DEFINES
        PDP6=1
    FEATURE_INT64
    FEATURE_VIDEO
    FEATURE_DISPLAY
    USES_AIO
    LABEL PDP10
    PKG_FAMILY decpdp_family
    TEST pdp6)
