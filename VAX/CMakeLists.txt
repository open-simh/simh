function(add_vax_simulator)
  cmake_parse_arguments(PARSE_ARGV 0 VAXSIM "" "NAME" "ALIASES;DEFINITIONS;SOURCES")
  add_simulator2(NAME ${VAXSIM_NAME}
    ALIASES ${VAXSIM_ALIASES}
    NETWORK
    ROMS
    SCSI
    DEFINITIONS
      VM_VAX
      USE_INT64
      USE_ADDR64
      ${VAXSIM_DEFINITIONS}
    INCLUDES
      ../PDP11
      ${VAXSIM_INCLUDES}
    SOURCES ${VAXSIM_SOURCES}
  )
endfunction()

add_vax_simulator(NAME vax
  ALIASES microvax3900
  DEFINITIONS USE_SIM_VIDEO
  SOURCES
    vax_cpu.c vax_cpu1.c vax_fpa.c
    vax_io.c vax_cis.c vax_octa.c
    vax_cmode.c vax_mmu.c vax_stddev.c
    vax_sysdev.c vax_sys.c vax_syscm.c
    vax_syslist.c vax_vc.c vax_lk.c
    vax_vs.c vax_2681.c
    ../PDP11/pdp11_rl.c ../PDP11/pdp11_rq.c ../PDP11/pdp11_ts.c
    ../PDP11/pdp11_dz.c ../PDP11/pdp11_lp.c ../PDP11/pdp11_tq.c
    ../PDP11/pdp11_xq.c ../PDP11/pdp11_vh.c ../PDP11/pdp11_cr.c
    ../PDP11/pdp11_td.c ../PDP11/pdp11_io_lib.c ../PDP11/pdp11_dup.c
)

# MicroVAX I
add_vax_simulator(NAME microvax1
  DEFINITIONS VAX_610 USE_SIM_VIDEO 
  SOURCES
    vax_cpu.c vax_cpu1.c vax_fpa.c
    vax_cis.c vax_octa.c vax_cmode.c
    vax_mmu.c vax_sys.c vax_syscm.c
    vax610_stddev.c vax610_sysdev.c vax610_io.c
    vax610_syslist.c vax610_mem.c vax_vc.c
    vax_lk.c vax_vs.c vax_2681.c
    ../PDP11/pdp11_rl.c ../PDP11/pdp11_rq.c ../PDP11/pdp11_ts.c
    ../PDP11/pdp11_dz.c ../PDP11/pdp11_lp.c ../PDP11/pdp11_tq.c
    ../PDP11/pdp11_xq.c ../PDP11/pdp11_vh.c ../PDP11/pdp11_cr.c
    ../PDP11/pdp11_td.c ../PDP11/pdp11_io_lib.c
)

# rtVAX 1000
add_vax_simulator(NAME rtvax1000
  DEFINITIONS VAX_620
  SOURCES
    vax_cpu.c vax_cpu1.c vax_fpa.c
    vax_cis.c vax_octa.c vax_cmode.c
    vax_mmu.c vax_sys.c vax_syscm.c
    vax_watch.c vax630_stddev.c vax630_sysdev.c
    vax630_io.c vax630_syslist.c vax_va.c
    vax_vc.c vax_lk.c vax_vs.c
    vax_2681.c vax_gpx.c
    ../PDP11/pdp11_rl.c ../PDP11/pdp11_rq.c ../PDP11/pdp11_ts.c
    ../PDP11/pdp11_dz.c ../PDP11/pdp11_lp.c ../PDP11/pdp11_tq.c
    ../PDP11/pdp11_xq.c ../PDP11/pdp11_vh.c ../PDP11/pdp11_cr.c
    ../PDP11/pdp11_td.c ../PDP11/pdp11_io_lib.c ../PDP11/pdp11_dup.c
)

# MicroVax II
add_vax_simulator(NAME microvax2
  DEFINITIONS VAX_630 USE_SIM_VIDEO
  SOURCES
    vax_cpu.c vax_cpu1.c vax_fpa.c
    vax_cis.c vax_octa.c vax_cmode.c
    vax_mmu.c vax_sys.c vax_syscm.c
    vax_watch.c vax630_stddev.c vax630_sysdev.c
    vax630_io.c vax630_syslist.c vax_va.c
    vax_vc.c vax_lk.c vax_vs.c
    vax_2681.c vax_gpx.c
    ../PDP11/pdp11_rl.c ../PDP11/pdp11_rq.c ../PDP11/pdp11_ts.c
    ../PDP11/pdp11_dz.c ../PDP11/pdp11_lp.c ../PDP11/pdp11_tq.c
    ../PDP11/pdp11_xq.c ../PDP11/pdp11_vh.c ../PDP11/pdp11_cr.c
    ../PDP11/pdp11_td.c ../PDP11/pdp11_io_lib.c ../PDP11/pdp11_dup.c
)

add_vax_simulator(NAME vax730
  DEFINITIONS VAX_730
  SOURCES
    vax_cpu.c vax_cpu1.c vax_fpa.c
    vax_cis.c vax_octa.c  vax_cmode.c
    vax_mmu.c vax_sys.c  vax_syscm.c
    vax730_stddev.c vax730_sys.c
    vax730_mem.c vax730_uba.c vax730_rb.c
    vax730_syslist.c
    ../PDP11/pdp11_rl.c ../PDP11/pdp11_rq.c ../PDP11/pdp11_ts.c
    ../PDP11/pdp11_dz.c ../PDP11/pdp11_lp.c ../PDP11/pdp11_tq.c
    ../PDP11/pdp11_xu.c ../PDP11/pdp11_ry.c ../PDP11/pdp11_cr.c
    ../PDP11/pdp11_hk.c ../PDP11/pdp11_vh.c ../PDP11/pdp11_dmc.c
    ../PDP11/pdp11_td.c ../PDP11/pdp11_tc.c ../PDP11/pdp11_rk.c
    ../PDP11/pdp11_io_lib.c ../PDP11/pdp11_ch.c ../PDP11/pdp11_dup.c
)

add_vax_simulator(NAME vax750
  DEFINITIONS VAX_750
  SOURCES
    vax_cpu.c vax_cpu1.c vax_fpa.c
    vax_cis.c vax_octa.c  vax_cmode.c
    vax_mmu.c vax_sys.c  vax_syscm.c
    vax750_stddev.c vax750_cmi.c
    vax750_mem.c vax750_uba.c vax7x0_mba.c
    vax750_syslist.c
    ../PDP11/pdp11_rl.c ../PDP11/pdp11_rq.c ../PDP11/pdp11_ts.c
    ../PDP11/pdp11_dz.c ../PDP11/pdp11_lp.c ../PDP11/pdp11_tq.c
    ../PDP11/pdp11_xu.c ../PDP11/pdp11_ry.c ../PDP11/pdp11_cr.c
    ../PDP11/pdp11_hk.c ../PDP11/pdp11_rp.c ../PDP11/pdp11_tu.c
    ../PDP11/pdp11_vh.c ../PDP11/pdp11_dmc.c ../PDP11/pdp11_dup.c
    ../PDP11/pdp11_td.c ../PDP11/pdp11_tc.c ../PDP11/pdp11_rk.c
    ../PDP11/pdp11_io_lib.c ../PDP11/pdp11_ch.c
)

add_vax_simulator(NAME vax780
  DEFINITIONS VAX_780
  SOURCES
    vax_cpu.c vax_cpu1.c vax_fpa.c
    vax_cis.c vax_octa.c  vax_cmode.c
    vax_mmu.c vax_sys.c  vax_syscm.c
    vax780_stddev.c vax780_sbi.c
    vax780_mem.c vax780_uba.c vax7x0_mba.c
    vax780_fload.c vax780_syslist.c
    ../PDP11/pdp11_rl.c ../PDP11/pdp11_rq.c ../PDP11/pdp11_ts.c
    ../PDP11/pdp11_dz.c ../PDP11/pdp11_lp.c ../PDP11/pdp11_tq.c
    ../PDP11/pdp11_xu.c ../PDP11/pdp11_ry.c ../PDP11/pdp11_cr.c
    ../PDP11/pdp11_rp.c ../PDP11/pdp11_tu.c ../PDP11/pdp11_hk.c
    ../PDP11/pdp11_vh.c ../PDP11/pdp11_dmc.c ../PDP11/pdp11_dup.c
    ../PDP11/pdp11_td.c ../PDP11/pdp11_tc.c ../PDP11/pdp11_rk.c
    ../PDP11/pdp11_io_lib.c ../PDP11/pdp11_ch.c
)

add_vax_simulator(NAME vax8200
  DEFINITIONS VAX_820
  SOURCES
    vax_cpu.c vax_cpu1.c vax_fpa.c
    vax_cis.c vax_octa.c  vax_cmode.c
    vax_mmu.c vax_sys.c  vax_syscm.c
    vax_watch.c vax820_stddev.c vax820_bi.c
    vax820_mem.c vax820_uba.c vax820_ka.c
    vax820_syslist.c
    ../PDP11/pdp11_rl.c ../PDP11/pdp11_rq.c ../PDP11/pdp11_ts.c
    ../PDP11/pdp11_dz.c ../PDP11/pdp11_lp.c ../PDP11/pdp11_tq.c
    ../PDP11/pdp11_xu.c ../PDP11/pdp11_ry.c ../PDP11/pdp11_cr.c
    ../PDP11/pdp11_hk.c ../PDP11/pdp11_vh.c ../PDP11/pdp11_dmc.c
    ../PDP11/pdp11_td.c ../PDP11/pdp11_tc.c ../PDP11/pdp11_rk.c
    ../PDP11/pdp11_io_lib.c ../PDP11/pdp11_ch.c ../PDP11/pdp11_dup.c
)

add_vax_simulator(NAME vax8600
  DEFINITIONS VAX_860
  SOURCES
    vax_cpu.c vax_cpu1.c vax_fpa.c
    vax_cis.c vax_octa.c  vax_cmode.c
    vax_mmu.c vax_sys.c  vax_syscm.c
    vax860_stddev.c vax860_sbia.c
    vax860_abus.c vax780_uba.c vax7x0_mba.c
    vax860_syslist.c
    ../PDP11/pdp11_rl.c ../PDP11/pdp11_rq.c ../PDP11/pdp11_ts.c
    ../PDP11/pdp11_dz.c ../PDP11/pdp11_lp.c ../PDP11/pdp11_tq.c
    ../PDP11/pdp11_xu.c ../PDP11/pdp11_ry.c ../PDP11/pdp11_cr.c
    ../PDP11/pdp11_rp.c ../PDP11/pdp11_tu.c ../PDP11/pdp11_hk.c
    ../PDP11/pdp11_vh.c ../PDP11/pdp11_dmc.c ../PDP11/pdp11_dup.c
    ../PDP11/pdp11_td.c ../PDP11/pdp11_tc.c ../PDP11/pdp11_rk.c
    ../PDP11/pdp11_io_lib.c ../PDP11/pdp11_ch.c
)

add_vax_simulator(NAME microvax2000
  DEFINITIONS VAX_410 USE_SIM_VIDEO
  SOURCES
    vax_cpu.c vax_cpu1.c vax_fpa.c
    vax_cis.c vax_octa.c vax_cmode.c
    vax_mmu.c vax_sys.c vax_syscm.c
    vax_watch.c vax_nar.c vax4xx_stddev.c
    vax410_sysdev.c vax410_syslist.c vax4xx_dz.c
    vax4xx_rd.c vax4xx_rz80.c vax_xs.c
    vax4xx_va.c vax4xx_vc.c vax_lk.c
    vax_vs.c vax_gpx.c
)

function(add_vax_420_simulator name define)
  add_vax_simulator(NAME ${name}
    DEFINITIONS VAX_420 USE_SIM_VIDEO ${define}
    SOURCES
      vax_cpu.c vax_cpu1.c vax_fpa.c
      vax_cis.c vax_octa.c vax_cmode.c
      vax_mmu.c vax_sys.c vax_syscm.c
      vax_watch.c vax_nar.c vax4xx_stddev.c
      vax420_sysdev.c vax420_syslist.c vax4xx_dz.c
      vax4xx_rd.c vax4xx_rz80.c vax_xs.c
      vax4xx_va.c vax4xx_vc.c vax4xx_ve.c
      vax_lk.c vax_vs.c vax_gpx.c
  )
endfunction()

add_vax_420_simulator(infoserver100     VAX_411)
add_vax_420_simulator(infoserver150vxt  VAX_412)
add_vax_420_simulator(microvax3100      VAX_41A)
add_vax_420_simulator(microvax3100e     VAX_41D)
add_vax_420_simulator(vaxstation3100m30 VAX_42A)
add_vax_420_simulator(vaxstation3100m38 VAX_42B)

add_vax_simulator(NAME vaxstation3100m76
  DEFINITIONS VAX_43 USE_SIM_VIDEO
  SOURCES
    vax_cpu.c vax_cpu1.c vax_fpa.c
    vax_cis.c vax_octa.c vax_cmode.c
    vax_mmu.c vax_sys.c vax_syscm.c
    vax_watch.c vax_nar.c vax4xx_stddev.c
    vax43_sysdev.c vax43_syslist.c vax4xx_dz.c
    vax4xx_rz80.c vax_xs.c vax4xx_vc.c
    vax4xx_ve.c vax_lk.c vax_vs.c
)

function(add_vax_440_simulator name define)
  add_vax_simulator(NAME ${name}
    DEFINITIONS VAX_440 ${define}
    SOURCES
      vax_cpu.c vax_cpu1.c vax_fpa.c
      vax_cis.c vax_octa.c vax_cmode.c
      vax_mmu.c vax_sys.c vax_syscm.c
      vax_watch.c vax_nar.c vax4xx_stddev.c
      vax440_sysdev.c vax440_syslist.c vax4xx_dz.c
      vax_xs.c vax_lk.c vax_vs.c vax4xx_rz94.c
  )
endfunction()

add_vax_440_simulator(vaxstation4000m60 VAX_46)
add_vax_440_simulator(microvax3100m80   VAX_47)
add_vax_440_simulator(vaxstation4000vlc VAX_48)

add_vax_simulator(NAME infoserver1000
  DEFINITIONS IS_1000
  SOURCES
    vax_cpu.c vax_cpu1.c vax_fpa.c
    vax_cis.c vax_octa.c vax_cmode.c
    vax_mmu.c vax_sys.c vax_syscm.c
    vax_watch.c vax_nar.c vax_xs.c
    vax4xx_rz94.c vax4nn_stddev.c
    is1000_sysdev.c is1000_syslist.c
)
