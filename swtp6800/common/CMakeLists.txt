add_library(swtp6800_common STATIC
  bootrom.c
  dc-4.c
  fd400.c
  i2716.c
  m6800.c
  m6810.c
  mp-8m.c
  mp-b2.c
  mp-s.c
)
target_include_directories(swtp6800_common PUBLIC ../swtp6800 ${SIMH_DIR})
set_property(TARGET swtp6800_common PROPERTY FOLDER "Simulators/Libraries")
