
# XXX
set(ALPHA_DEFINITIONS
  USE_INT64
  USE_ADDR64
  )
set(ALPHA_SOURCES
  alpha_500au_syslist.c
  alpha_cpu.c
  alpha_ev5_cons.c
  alpha_ev5_pal.c
  alpha_ev5_tlb.c
  alpha_fpi.c
  alpha_fpv.c
  alpha_io.c
  alpha_mmu.c
  alpha_sys.c
)
add_simulator(alpha)
