# IBM 650
add_simulator2(NAME i650
  DEFINITIONS
    USE_INT64
    USE_SIM_CARD
  SOURCES
    i650_cpu.c
    i650_cdr.c
    i650_cdp.c
	i650_dsk.c
    i650_mt.c
    i650_sys.c
)
