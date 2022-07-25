add_simulator2(NAME sage
  EXPERIMENTAL
  DEFINITIONS HAVE_INT64
  SOURCES
    sage_defs.h
    chip_defs.h
    i8251.c
    i8253.c
    i8255.c
    i8259.c
    i8272.c
    m68k_cpu.h
    m68k_cpu.c
    m68k_mem.c
    m68k_parse.tab.h
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
