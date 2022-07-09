add_simulator2(NAME sel32
  NETWORK
  DEFINITIONS USE_INT32 SEL32
  SOURCES
    sel32_cpu.c sel32_sys.c sel32_chan.c
    sel32_iop.c sel32_com.c sel32_con.c
    sel32_clk.c sel32_mt.c sel32_lpr.c
    sel32_scfi.c sel32_fltpt.c sel32_disk.c
    sel32_hsdp.c sel32_mfp.c sel32_scsi.c
    sel32_ec.c
)
