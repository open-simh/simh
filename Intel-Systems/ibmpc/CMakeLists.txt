
# IBM PC
set(IBMPC_SOURCES
  ../common/i8088.c
  ../common/i8237.c
  ../common/i8255.c
  ../common/i8253.c
  ../common/i8259.c
  ../common/pcbus.c
  ../common/pceprom.c
  ../common/pcram8.c
  ibmpc.c
  ibmpc_sys.c
  )
add_simulator(ibmpc)
