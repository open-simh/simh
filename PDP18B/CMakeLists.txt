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
add_simulator2(NAME pdp4
  DEFINITIONS PDP4
  SOURCES ${PDP18B_SOURCES}
)

# DEC PDP-7
add_simulator2(NAME pdp7
  DISPLAY
  # DISPLAY_340
  DEFINITIONS
    PDP7
    DISPLAY_TYPE=DIS_TYPE30
    PIX_SCALE=RES_HALF
  SOURCES ${PDP18B_SOURCES}
)

# DEC PDP-9
add_simulator2(NAME pdp9
  DEFINITIONS PDP9
  SOURCES ${PDP18B_SOURCES}
)

# DEC PDP-15
add_simulator2(NAME pdp15
  DEFINITIONS PDP15
  SOURCES ${PDP18B_SOURCES}
)
