
# MIT TX-0
set(TX0_SOURCES
  tx0_cpu.c
  tx0_dpy.c
  tx0_stddev.c
  tx0_sys.c
  tx0_sys_orig.c
  )
add_simulator(tx0)
add_simulator_graphics(tx0)
if(BUILD_ALL OR SIMULATOR_TX0_ENABLE)
  if(SIMULATOR_TX0_GRAPHICS)
    target_sources(tx0 PUBLIC ${GRAPHICS_DISPLAY_SOURCES})
  endif()
endif()
