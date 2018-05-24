
# IBM 701
set(I701_DEFINITIONS
  I701
  USE_INT64
  USE_SIM_CARD
  )
set(I701_SOURCES
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
add_simulator(i701)

# IBM 704
set(I704_DEFINITIONS
  I704
  USE_INT64
  USE_SIM_CARD
  )
set(I704_SOURCES
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
add_simulator(i704)

# IBM 70X0 common bits
set(I70X0_DEFINITIONS
  USE_SIM_CARD
  )
set(I70X0_SOURCES
  i7000_chron.c
  i7000_com.c
  i7000_dsk.c
  i7000_ht.c
  i7000_mt.c
  )

# IBM 7010
set(I7010_DEFINITIONS
  ${I70X0_DEFINITIONS}
  I7010
  )
set(I7010_SOURCES
  ${I70X0_SOURCES}
  i7000_cdp.c
  i7000_cdr.c
  i7000_con.c
  i7000_chan.c
  i7000_lpr.c
  i7010_chan.c
  i7010_cpu.c
  i7010_sys.c
  )
add_simulator(i7010)

# IBM 7070
set(I7070_DEFINITIONS
  ${I70X0_DEFINITIONS}
  I7070
  USE_INT64
  )
set(I7070_SOURCES
  ${I70X0_SOURCES}
  i7000_cdp.c
  i7000_cdr.c
  i7000_con.c
  i7000_chan.c
  i7000_lpr.c
  i7070_chan.c
  i7070_cpu.c
  i7070_sys.c
  )
add_simulator(i7070)

# IBM 7080
set(I7080_DEFINITIONS
  ${I70X0_DEFINITIONS}
  I7080
  )
set(I7080_SOURCES
  ${I70X0_SOURCES}
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
add_simulator(i7080)

# IBM 7090
set(I7090_DEFINITIONS
  ${I70X0_DEFINITIONS}
  I7090
  USE_INT64
  )
set(I7090_SOURCES
  ${I70X0_SOURCES}
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
add_simulator(i7090)
