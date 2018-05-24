
# DEC PDP-1
set(PDP1_SOURCES
  pdp1_clk.c
  pdp1_cpu.c
  pdp1_dcs.c
  pdp1_dpy.c
  pdp1_drm.c
  pdp1_dt.c
  pdp1_lp.c
  pdp1_stddev.c
  pdp1_sys.c
  )
add_simulator(pdp1)
add_simulator_graphics(pdp1)
if(BUILD_ALL OR SIMULATOR_PDP1_ENABLE)
  if(SIMULATOR_PDP1_GRAPHICS)
    target_sources(pdp1 PUBLIC ${GRAPHICS_DISPLAY_SOURCES})
    target_compile_definitions(pdp1 PUBLIC
	  ${GRAPHICS_DISPLAY_DEFINITIONS}
	  DISPLAY_TYPE=DIS_TYPE30
	  PIX_SCALE=RES_HALF
	  )
  endif()
endif()
