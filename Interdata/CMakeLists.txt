# Interdata common bits
add_library(interdata_common STATIC
  id_defs.h
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
set_property(TARGET interdata_common PROPERTY FOLDER "Simulators/Libraries")
target_include_directories(interdata_common PUBLIC . ${SIMH_DIR})

# Interdata 16-bit
add_simulator2(NAME id16
  SOURCES
    id16_cpu.c
    id16_dboot.c
    id16_sys.c
  LIBRARIES interdata_common
)

# Interdata 32-bit
add_simulator2(NAME id32
  SOURCES
    id32_cpu.c
    id32_dboot.c
    id32_sys.c
  LIBRARIES interdata_common
)
