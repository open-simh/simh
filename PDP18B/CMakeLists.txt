
# DEC 18-bit common bits
set(PDP18B_SOURCES
  pdp18b_cpu.c
  pdp18b_dr15.c
  pdp18b_drm.c
  pdp18b_dt.c
  pdp18b_fpp.c
  pdp18b_g2tty.c
  pdp18b_lp.c
  pdp18b_mt.c
  pdp18b_rb.c
  pdp18b_rf.c
  pdp18b_rp.c
  pdp18b_stddev.c
  pdp18b_sys.c
  pdp18b_tt1.c
  )

# DEC PDP-4
set(PDP4_DEFINITIONS PDP4)
set(PDP4_SOURCES ${PDP18B_SOURCES})
add_simulator(pdp4)

# DEC PDP-7
set(PDP7_DEFINITIONS PDP7)
set(PDP7_SOURCES ${PDP18B_SOURCES})
add_simulator(pdp7)

# DEC PDP-9
set(PDP9_DEFINITIONS PDP9)
set(PDP9_SOURCES ${PDP18B_SOURCES})
add_simulator(pdp9)

# DEC PDP-15
set(PDP15_DEFINITIONS PDP15)
set(PDP15_SOURCES ${PDP18B_SOURCES})
add_simulator(pdp15)
