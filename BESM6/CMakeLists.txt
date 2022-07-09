# TODO: SDL support
add_simulator2(NAME besm6
  DEFINITIONS
    USE_SIM_CARD
  SOURCES
    besm6_arith.c
    besm6_cpu.c
    besm6_disk.c
    besm6_drum.c
    besm6_mg.c
    besm6_mmu.c
    besm6_panel.c
    besm6_pl.c
    besm6_printer.c
    besm6_punch.c
    besm6_punchcard.c
    besm6_sys.c
    besm6_tty.c
    besm6_vu.c
)
