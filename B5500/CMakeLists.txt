add_simulator2(NAME b5500
  DEFINITIONS
    B5500
    USE_INT64
    USE_SIM_CARD
  SOURCES
    b5500_cpu.c
    b5500_dk.c
    b5500_dr.c
    b5500_dtc.c
    b5500_io.c
    b5500_mt.c
    b5500_sys.c
    b5500_urec.c
)
