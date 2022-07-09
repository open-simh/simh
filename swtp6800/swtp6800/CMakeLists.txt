add_simulator2(NAME swtp6800mp-a
  SOURCES
    swtp_defs.h
    ../common/mp-a.c
    mp-a_sys.c
  LIBRARIES swtp6800_common
)

add_simulator2(NAME swtp6800mp-a2
  SOURCES
    swtp_defs.h
    ../common/mp-a2.c
    mp-a2_sys.c
  LIBRARIES swtp6800_common
)
