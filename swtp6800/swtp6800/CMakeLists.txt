
# XXX
set(SWTP6800_SOURCES
../common/bootrom.c
../common/dc-4.c
../common/m6800.c
../common/m6810.c
../common/mp-8m.c
../common/mp-b2.c
../common/mp-s.c
)

# XXX
set(SWTP6800MPA_SOURCES
  ${SWTP6800_SOURCES}
  ../common/mp-a.c
  mp-a_sys.c
  )
add_simulator(swtp6800mpa)

# XXX
set(SWTP6800MPA2_SOURCES
  ${SWTP6800_SOURCES}
  ../common/i2716.c
  ../common/mp-a2.c
  mp-a2_sys.c
  )
add_simulator(swtp6800mpa2)
