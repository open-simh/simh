
# Interdata common bits
set(INTERDATA_SOURCES
  id_dp.c
  id_fd.c
  id_fp.c
  id_idc.c
  id_io.c
  id_lp.c
  id_mt.c
  id_pas.c
  id_pt.c
  id_tt.c
  id_ttp.c
  id_uvc.c
)

# Interdata 16-bit
set(ID16_SOURCES
  ${INTERDATA_SOURCES}
  id16_cpu.c
  id16_dboot.c
  id16_sys.c
  )
add_simulator(id16)

# Interdata 32-bit
set(ID32_SOURCES
  ${INTERDATA_SOURCES}
  id32_cpu.c
  id32_dboot.c
  id32_sys.c
  )
add_simulator(id32)
