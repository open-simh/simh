
# XXX
set(SAGE_DEFINITIONS
  HAVE_INT64
  USE_SIM_IMD
  )
set(SAGE_SOURCES
  i8251.c
  i8253.c
  i8255.c
  i8259.c
  i8272.c
  m68k_cpu.c
  m68k_mem.c
  m68k_parse.tab.c
  m68k_scp.c
  m68k_sys.c
  sage_aux.c
  sage_cons.c
  sage_cpu.c
  sage_fd.c
  sage_hd.c
  sage_ieee.c
  sage_lp.c
  sage_stddev.c
  sage_sys.c
  )
add_simulator(sage)
