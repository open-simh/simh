
# XXX
set(B5500_DEFINITIONS
  B5500
  USE_INT64
  USE_SIM_CARD
  )
set(B5500_SOURCES
  b5500_cpu.c
  b5500_dk.c
  b5500_dr.c
  b5500_dtc.c
  b5500_io.c
  b5500_mt.c
  b5500_sys.c
  b5500_urec.c
  )
add_simulator(b5500)
