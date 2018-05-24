
# IBM 650
set(I650_DEFINITIONS
USE_INT64
USE_SIM_CARD
)
set(I650_SOURCES
i650_cdp.c
i650_cdr.c
i650_cpu.c
i650_sys.c
)
add_simulator(i650)

