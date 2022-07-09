# DEC PDP-10
add_simulator2(NAME pdp10
  NETWORK
  DEFINITIONS VM_PDP10 USE_INT64
  INCLUDES  ../PDP11
  SOURCES
    pdp10_cpu.c
    pdp10_fe.c
    pdp10_ksio.c
    pdp10_lp20.c
    pdp10_mdfp.c
    pdp10_pag.c
    pdp10_rp.c
    pdp10_sys.c
    pdp10_tim.c
    pdp10_tu.c
    pdp10_xtnd.c
    ../PDP11/pdp11_ch.c
    ../PDP11/pdp11_dz.c
    ../PDP11/pdp11_pt.c
    ../PDP11/pdp11_ry.c
    ../PDP11/pdp11_cr.c
    ../PDP11/pdp11_dup.c
    ../PDP11/pdp11_dmc.c
    ../PDP11/pdp11_kmc.c
    ../PDP11/pdp11_xu.c
)

set(SIMH_PANDA_LIGHTS OFF CACHE BOOL "Enable the Panda display on pdp10-ka, pdp10-ki simulators.")
function(add_panda_lights target)
  if(SIMH_PANDA_LIGHTS)
    target_compile_definitions(${target} PUBLIC PANDA_LIGHTS)
    target_sources(${target} PUBLIC ka10_lights.c)
    target_link_libraries(${target} PUBLIC usb-1.0)
  endif()
endfunction()

add_simulator2(NAME pdp10-ka
  DISPLAY
  # DISPLAY_340
  # DISPLAY_III
  NETWORK
  DEFINITIONS KA=1 USE_SIM_CARD USE_INT64
  SOURCES
    kx10_cpu.c kx10_sys.c kx10_df.c
    kx10_dp.c kx10_mt.c kx10_cty.c
    kx10_lp.c kx10_pt.c kx10_dc.c
    kx10_rp.c kx10_rc.c kx10_dt.c
    kx10_dk.c kx10_cr.c kx10_cp.c
    kx10_tu.c kx10_rs.c ka10_pd.c
    kx10_rh.c kx10_imp.c ka10_tk10.c
    ka10_mty.c ka10_imx.c ka10_ch10.c
    ka10_stk.c ka10_ten11.c ka10_auxcpu.c
    ka10_pmp.c ka10_dkb.c pdp6_dct.c
    pdp6_dtc.c pdp6_mtc.c pdp6_dsk.c
    pdp6_dcs.c ka10_dpk.c kx10_dpy.c
    ka10_ai.c ka10_iii.c kx10_disk.c
    ka10_pclk.c ka10_tv.c
)
add_panda_lights(pdp10-ka)

add_simulator2(NAME pdp10-ki
  DISPLAY
  # DISPLAY_340
  NETWORK
  DEFINITIONS KI=1 USE_INT64 USE_SIM_CARD
  SOURCES
    kx10_cpu.c kx10_sys.c kx10_df.c
    kx10_dp.c kx10_mt.c kx10_cty.c
    kx10_lp.c kx10_pt.c kx10_dc.c 
    kx10_rh.c kx10_rp.c kx10_rc.c
    kx10_dt.c kx10_dk.c kx10_cr.c
    kx10_cp.c kx10_tu.c kx10_rs.c
    kx10_imp.c kx10_dpy.c kx10_disk.c
)
add_panda_lights(pdp10-ki)

add_simulator2(NAME pdp10-kl
  NETWORK
  DEFINITIONS KL=1 USE_INT64 USE_SIM_CARD
  SOURCES
    kx10_cpu.c kx10_sys.c kx10_df.c
    kx10_dp.c kx10_mt.c kx10_lp.c
    kx10_pt.c kx10_dc.c kx10_rh.c
    kx10_dt.c kx10_cr.c kx10_cp.c
    kx10_rp.c kx10_tu.c kx10_rs.c
    kx10_imp.c kl10_fe.c ka10_pd.c
    ka10_ch10.c kl10_nia.c kx10_disk.c
)

add_simulator2(NAME pdp10-ks
  NETWORK
  DEFINITIONS KS=1 USE_INT64
  INCLUDES ../PDP11
  SOURCES
    kx10_cpu.c kx10_sys.c kx10_disk.c
    ks10_cty.c ks10_uba.c kx10_rh.c
    kx10_rp.c kx10_tu.c ks10_dz.c
    ks10_tcu.c ks10_lp.c ks10_ch11.c
    ks10_kmc.c ks10_dup.c kx10_imp.c
)

add_simulator2(NAME pdp6
  DISPLAY
  # DISPLAY_340
  DEFINITIONS PDP6=1 USE_INT64 USE_SIM_CARD
  SOURCES
    kx10_cpu.c kx10_sys.c kx10_cty.c
	kx10_lp.c kx10_pt.c kx10_cr.c
	kx10_cp.c pdp6_dct.c pdp6_dtc.c
	pdp6_mtc.c pdp6_dsk.c pdp6_dcs.c
	kx10_dpy.c pdp6_slave.c
)
