
# XXX
set(SDS_SOURCES
  ../sim_card.c
  sds_cpu.c
  sds_cp.c
  sds_cr.c
  sds_drm.c
  sds_dsk.c
  sds_io.c
  sds_lp.c
  sds_mt.c
  sds_mux.c
  sds_rad.c
  sds_stddev.c
  sds_sys.c
  )
set(SDS_DEFINITIONS
  USE_SIM_CARD
  )
add_simulator(sds)
