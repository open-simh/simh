
# XXX
set(ALTAIRZ80_DEFINITIONS
  USE_SIM_IMD
  )
set(ALTAIRZ80_SOURCES
  altairz80_cpu.c
  altairz80_cpu_nommu.c
  altairz80_dsk.c
  altairz80_hdsk.c
  altairz80_mhdsk.c
  altairz80_net.c
  altairz80_sio.c
  altairz80_sys.c
  disasm.c
  flashwriter2.c
  i8272.c
  i86_decode.c
  i86_ops.c
  i86_prim_ops.c
  insnsd.c
  m68kasm.c
  m68kcpu.c
  m68kdasm.c
  m68kopac.c
  m68kopdm.c
  m68kopnz.c
  m68kops.c
  m68ksim.c
  mfdc.c
  n8vem.c
  s100_2sio.c
  s100_64fdc.c
  s100_adcs6.c
  s100_disk1a.c
  s100_disk2.c
  s100_disk3.c
  s100_dj2d.c
  s100_fif.c
  s100_hayes.c
  s100_hdc1001.c
  s100_icom.c
  s100_if3.c
  s100_jadedd.c
  s100_mdriveh.c
  s100_mdsa.c
  s100_mdsad.c
  s100_pmmi.c
  s100_scp300f.c
  s100_selchan.c
  s100_ss1.c
  s100_tarbell.c
  vfdhd.c
  wd179x.c
  )
add_simulator(altairz80)