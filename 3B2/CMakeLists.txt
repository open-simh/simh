
set(3B2_DEFINITIONS
  USE_ADDR64
  USE_INT64
  )

set(3B2_SOURCES
  3b2_ctc.c
  3b2_dmac.c
  3b2_if.c
  3b2_io.c
  3b2_iu.c
  3b2_ni.c
  3b2_ports.c
  )

set(3B2R2_DEFINITIONS ${3B2_DEFINITIONS})
set(3B2R2_SOURCES
  ${3B2_SOURCES}
  )

add_simulator(3b2r2)
if(SIMULATOR_3B2R2_ENABLE)
  target_compile_definitions(3b2r2 PUBLIC -DREV2)
endif()

set(3B2R3_DEFINITIONS ${3B2_DEFINITIONS})
set(3B2R3_SOURCES
  ${3B2_SOURCES}
  3b2_id.c
  )

add_simulator(3b2r3)
if(SIMULATOR_3B2R3_ENABLE)
  target_compile_definitions(3b2r3 PUBLIC -DREV3)
endif()

