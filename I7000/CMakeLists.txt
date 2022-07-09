# IBM 701
add_simulator2(NAME i701
  DEFINITIONS
    I701
    USE_INT64
    USE_SIM_CARD
  SOURCES
    i7000_defs.h
    i7090_defs.h
    i701_chan.c
    i701_cpu.c
    i701_sys.c
    i7000_mt.c
    i7090_cdr.c
    i7090_cdp.c
    i7000_chan.c
    i7090_drum.c
    i7090_lpr.c
)

# IBM 704
add_simulator2(NAME i704
  DEFINITIONS
    I704
    USE_INT64
    USE_SIM_CARD
  SOURCES
    i7000_defs.h
    i7090_defs.h
    i7000_chan.c
    i7090_cdr.c
    i7090_cdp.c
    i7090_chan.c
    i7090_cpu.c
    i7090_drum.c
    i7090_lpr.c
    i7000_mt.c
    i7090_sys.c
)

# IBM 70X0 common bits
set(I70X0_SOURCES
  i7000_chron.c
  i7000_com.c
  i7000_dsk.c
  i7000_ht.c
  i7000_mt.c
  )

# IBM 7010
add_simulator2(NAME i7010
  DEFINITIONS
    USE_SIM_CARD
    I7010
  SOURCES
    ${I70X0_SOURCES}
    i7000_defs.h
    i7010_defs.h
    i7000_cdp.c
    i7000_cdr.c
    i7000_con.c
    i7000_chan.c
    i7000_lpr.c
    i7010_chan.c
    i7010_cpu.c
    i7010_sys.c
)

# IBM 7070
add_simulator2(NAME i7070
  DEFINITIONS
    I7070
    USE_INT64
    USE_SIM_CARD
  SOURCES
    ${I70X0_SOURCES}
    i7000_defs.h
    i7070_defs.h
    i7000_cdp.c
    i7000_cdr.c
    i7000_con.c
    i7000_chan.c
    i7000_lpr.c
    i7070_chan.c
    i7070_cpu.c
    i7070_sys.c
)

# IBM 7080
add_simulator2(NAME i7080
  DEFINITIONS
    I7080
    USE_SIM_CARD
  SOURCES
    ${I70X0_SOURCES}
    i7000_defs.h
    i7080_defs.h
    i7000_cdp.c
    i7000_cdr.c
    i7000_con.c
    i7000_chan.c
    i7000_lpr.c
    i7080_chan.c
    i7080_cpu.c
    i7080_drum.c
    i7080_sys.c
)

# IBM 7090
add_simulator2(NAME i7090
  DEFINITIONS
    I7090
    USE_INT64
    USE_SIM_CARD
  SOURCES
    ${I70X0_SOURCES}
    i7000_defs.h
    i7090_defs.h
    i7000_chan.c
    i7090_cdp.c
    i7090_cdr.c
    i7090_chan.c
    i7090_cpu.c
    i7090_drum.c
    i7090_hdrum.c
    i7090_lpr.c
    i7090_sys.c
  )
