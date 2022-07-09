# DEC PDP-1
add_simulator2(NAME pdp1
  DISPLAY
  DEFINITIONS
    DISPLAY_TYPE=DIS_TYPE30
    PIX_SCALE=RES_HALF
  SOURCES
    pdp1_lp.c
    pdp1_cpu.c
    pdp1_stddev.c
    pdp1_sys.c
    pdp1_dt.c
    pdp1_drm.c
    pdp1_clk.c
    pdp1_dcs.c
    pdp1_dpy.c
)
