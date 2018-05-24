
# common bits
set(DATAGENERAL_SOURCES
  nova_clk.c
  nova_dkp.c
  nova_dsk.c
  nova_lp.c
  nova_mta.c
  nova_plt.c
  nova_pt.c
  nova_qty.c
  nova_sys.c
  nova_tt1.c
  )

# Data General Eclipse
set(ECLIPSE_DEFINITIONS
  ECLIPSE
  )
set(ECLIPSE_SOURCES
  ${DATAGENERAL_SOURCES}
  eclipse_cpu.c
  eclipse_tt.c
  )
add_simulator(eclipse)

# Data General Nova
set(NOVA_DEFINITIONS
  NOVA
  )
set(NOVA_SOURCES
  ${DATAGENERAL_SOURCES}
  nova_cpu.c
  nova_tt.c
  )
add_simulator(nova)
