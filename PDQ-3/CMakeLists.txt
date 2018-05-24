
# XXX
set(PDQ3_DEFINITIONS
  USE_SIM_IMD
  )
set(PDQ3_SOURCES
  pdq3_cpu.c
  pdq3_debug.c
  pdq3_fdc.c
  pdq3_mem.c
  pdq3_stddev.c
  pdq3_sys.c
  )
add_simulator(pdq3)
