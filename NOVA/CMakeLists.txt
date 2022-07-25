# common bits that don't depend on conditionally compiled features
add_library(nova_common STATIC
  nova_clk.c
  nova_dkp.c
  nova_dsk.c
  nova_lp.c
  nova_mta.c
  nova_plt.c
  nova_pt.c
  nova_qty.c
  nova_tt1.c
)
set_property(TARGET nova_common PROPERTY FOLDER "Simulators/Libraries")
target_include_directories(nova_common PUBLIC . ${SIMH_DIR})

# Data General Eclipse
add_simulator2(NAME eclipse
  DEFINITIONS ECLIPSE
  SOURCES
    nova_defs.h
    nova_sys.c
    eclipse_cpu.c
    eclipse_tt.c
  LIBRARIES nova_common
)

# Data General Nova
add_simulator2(NAME nova
  DEFINITIONS NOVA
  SOURCES
    nova_defs.h
    nova_sys.c
    nova_cpu.c
    nova_tt.c
  LIBRARIES nova_common
)
