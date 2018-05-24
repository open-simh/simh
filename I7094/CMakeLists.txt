
# IBM 7094
set(I7094_DEFINITIONS
  USE_INT64
  )
set(I7094_SOURCES
  i7094_binloader.c
  i7094_cd.c
  i7094_clk.c
  i7094_com.c
  i7094_cpu1.c
  i7094_cpu.c
  i7094_drm.c
  i7094_dsk.c
  i7094_io.c
  i7094_lp.c
  i7094_mt.c
  i7094_sys.c
  )
add_simulator(i7094)
