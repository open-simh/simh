
# DEC VAX common bits
set(VAX_DEFINITIONS
  VM_VAX
  USE_INT64
  USE_ADDR64
  )
set(VAX_INCLUDES
  ../PDP11
  )
set(VAX_SOURCES
  vax_cis.c
  vax_cmode.c
  vax_cpu1.c
  vax_cpu.c
  vax_fpa.c
  vax_mmu.c
  vax_octa.c
  vax_sys.c
  vax_syscm.c
  vax_watch.c
  )
function(add_vax_simulator NAME)
  string(TOUPPER ${NAME} NAMEU)
  add_simulator(${NAME})
  add_simulator_network(${NAME})
  if(BUILD_ALL OR SIMULATOR_${NAMEU}_ENABLE)
    target_compile_definitions(${NAME} PUBLIC ${VAX_DEFINITIONS})
    target_include_directories(${NAME} PUBLIC ${VAX_INCLUDES})
  endif()
endfunction()

# Common bits for bus-based machines
set(VAX_BUS_SOURCES
  ../PDP11/pdp11_rl.c
  ../PDP11/pdp11_rq.c
  ../PDP11/pdp11_ts.c
  ../PDP11/pdp11_dz.c
  ../PDP11/pdp11_lp.c
  ../PDP11/pdp11_tq.c
  ../PDP11/pdp11_td.c
  ../PDP11/pdp11_cr.c
  ../PDP11/pdp11_vh.c
  ../PDP11/pdp11_io_lib.c
  )

# DEC Q-bus common bits
set(VAX_QBUS_SOURCES
  ${VAX_BUS_SOURCES}
  ../PDP11/pdp11_xq.c
  )

# DEC Unibus common bits
set(VAX_UNIBUS_SOURCES
  ${VAX_BUS_SOURCES}
  ../PDP11/pdp11_ch.c
  ../PDP11/pdp11_xu.c
  ../PDP11/pdp11_ry.c
  ../PDP11/pdp11_hk.c
  ../PDP11/pdp11_dmc.c
  ../PDP11/pdp11_tc.c
  ../PDP11/pdp11_rk.c
  ../PDP11/pdp11_dup.c
  )

# Common definitions for VAX4XX variants
set(VAX4XX_SOURCES
  ../sim_scsi.c
  ${VAX_SOURCES}
  vax_lk.c
  vax_nar.c
  vax_vs.c
  vax_xs.c
  vax4xx_dz.c
  vax4xx_stddev.c
  )

# VAX410 - 
set(VAX410_DEFINITIONS
  VAX_410
  )
set(VAX410_SOURCES
  ${VAX4XX_SOURCES}
  vax_gpx.c
  vax4xx_rd.c
  vax4xx_rz80.c
  vax4xx_va.c
  vax4xx_vc.c
  vax410_sysdev.c
  vax410_syslist.c
  )
add_vax_simulator(vax410)
add_simulator_graphics(vax410)

# Common definitions for VAX420 variants
set(VAX420_DEFINITIONS
  VAX_420
  )
set(VAX420_SOURCES
  ${VAX4XX_SOURCES}
  vax_gpx.c
  vax4xx_rd.c
  vax4xx_rz80.c
  vax4xx_va.c
  vax4xx_vc.c
  vax4xx_ve.c
  vax420_sysdev.c
  vax420_syslist.c
  )

# VAX411 - 
set(VAX411_DEFINITIONS
  ${VAX420_DEFINITIONS}
  VAX_411
  )
set(VAX411_SOURCES
  ${VAX420_SOURCES}
  )
add_vax_simulator(vax411)
add_simulator_graphics(vax411)

# VAX412 - 
set(VAX412_DEFINITIONS
  ${VAX420_DEFINITIONS}
  VAX_412
  )
set(VAX412_SOURCES
  ${VAX420_SOURCES}
  )
add_vax_simulator(vax412)
add_simulator_graphics(vax412)

set(VAX41A_DEFINITIONS
  ${VAX420_DEFINITIONS}
  VAX_41A
  )
set(VAX41A_SOURCES
  ${VAX420_SOURCES}
  )
add_vax_simulator(vax41a)
add_simulator_graphics(vax41a)

set(VAX41D_DEFINITIONS
  ${VAX420_DEFINITIONS}
  VAX_41D
  )
set(VAX41D_SOURCES
  ${VAX420_SOURCES}
  )
add_vax_simulator(vax41d)
add_simulator_graphics(vax41d)

set(VAX42A_DEFINITIONS
  ${VAX420_DEFINITIONS}
  VAX_42A
  )
set(VAX42A_SOURCES
  ${VAX420_SOURCES}
  )
add_vax_simulator(vax42a)
add_simulator_graphics(vax42a)

set(VAX42B_DEFINITIONS
  ${VAX420_DEFINITIONS}
  VAX_42B
  )
set(VAX42B_SOURCES
  ${VAX420_SOURCES}
  )
add_vax_simulator(vax42b)
add_simulator_graphics(vax42b)

set(VAX440_DEFINITIONS
  VAX_440
  )
set(VAX440_SOURCES
  ${VAX4XX_SOURCES}
  vax4xx_rz94.c
  vax440_sysdev.c
  vax440_syslist.c
  )

set(VAX46_DEFINITIONS
  ${VAX440_DEFINITIONS}
  VAX_46
  )
set(VAX46_SOURCES
  ${VAX440_SOURCES}
  )
add_vax_simulator(vax46)
add_simulator_graphics(vax46)

set(VAX47_DEFINITIONS
  ${VAX440_DEFINITIONS}
  VAX_47
  )
set(VAX47_SOURCES
  ${VAX440_SOURCES}
  )
add_vax_simulator(vax47)
add_simulator_graphics(vax47)

set(VAX48_DEFINITIONS
  ${VAX440_DEFINITIONS}
  VAX_48
  )
set(VAX48_SOURCES
  ${VAX440_SOURCES}
  )
add_vax_simulator(vax48)
add_simulator_graphics(vax48)

set(IS1000_DEFINITIONS
  ${VAX_DEFINITIONS}
  IS_1000
  )
set(IS1000_SOURCES
  ../sim_scsi.c
  ${VAX_SOURCES}
  vax_nar.c
  vax_xs.c
  vax4xx_rz94.c
  vax4nn_stddev.c
  is1000_sysdev.c
  is1000_syslist.c
  )
add_vax_simulator(is1000)

# VAX6XX - Common definitions
set(VAX6XX_SOURCES
  ${VAX_SOURCES}
  ${VAX_QBUS_SOURCES}
  vax_2681.c
  vax_lk.c
  vax_vc.c
  vax_vs.c
  )

# VAX610 - DEC MicroVAX I
set(VAX610_DEFINITIONS
  VAX_610
  )
set(VAX610_SOURCES
  ${VAX6XX_SOURCES}
  vax610_io.c
  vax610_mem.c
  vax610_stddev.c
  vax610_sysdev.c
  vax610_syslist.c
  )
add_vax_simulator(vax610)
add_simulator_graphics(vax610)

# VAX630 - DEC MicroVAX II
set(VAX630_DEFINITIONS
  VAX_630
  )
set(VAX630_SOURCES
  ${VAX6XX_SOURCES}
  vax_gpx.c
  vax_va.c
  vax630_io.c
  vax630_stddev.c
  vax630_sysdev.c
  vax630_syslist.c
  ../PDP11/pdp11_dup.c
  )
add_vax_simulator(vax630)
add_simulator_graphics(vax630)

# VAX620 - DEC rtVAX 1000
set(VAX620_DEFINITIONS
  VAX_620
  )
set(VAX620_SOURCES
  ${VAX6XX_SOURCES}
  vax630_io.c
  vax630_stddev.c
  vax630_sysdev.c
  vax630_syslist.c
  )
add_vax_simulator(vax620)

# DEC VAX650 - MicroVAX III
set(VAX650_DEFINITIONS
  VAX_650
  )
set(VAX650_SOURCES
  ${VAX6XX_SOURCES}
  vax_io.c
  vax_stddev.c
  vax_sysdev.c
  vax_syslist.c
  ../PDP11/pdp11_dup.c
  )
add_vax_simulator(vax650)
add_simulator_graphics(vax650)

# DEC VAX7XX - Unibus VAXen
set(VAX7XX_SOURCES
  ${VAX_SOURCES}
  ${VAX_UNIBUS_SOURCES}
  )

# DEC VAX7XX - Massbus common bits
set(VAX7XX_MASSBUS_SOURCES
  vax7x0_mba.c
  ../PDP11/pdp11_rp.c
  ../PDP11/pdp11_tu.c
  )

# VAX730 - DEC VAX 11/730
set(VAX730_DEFINITIONS
  VAX_730
  )
set(VAX730_SOURCES
  ${VAX7XX_SOURCES}
  vax730_mem.c
  vax730_rb.c
  vax730_stddev.c
  vax730_sys.c
  vax730_syslist.c
  vax730_uba.c
  )
add_vax_simulator(vax730)

# VAX750 - DEC VAX 11/750
set(VAX750_DEFINITIONS
  VAX_750
  )
set(VAX750_SOURCES
  ${VAX7XX_SOURCES}
  ${VAX7XX_MASSBUS_SOURCES}
  vax750_cmi.c
  vax750_mem.c
  vax750_stddev.c
  vax750_syslist.c
  vax750_uba.c
  )
add_vax_simulator(vax750)

# VAX780 - DEC VAX 11/780
set(VAX780_DEFINITIONS
  VAX_780
  )
set(VAX780_SOURCES
  ${VAX7XX_SOURCES}
  ${VAX7XX_MASSBUS_SOURCES}
  vax780_fload.c
  vax780_mem.c
  vax780_sbi.c
  vax780_stddev.c
  vax780_syslist.c
  vax780_uba.c
  )
add_vax_simulator(vax780)

# VAX820 - DEC VAX 8200
set(VAX820_DEFINITIONS
  VAX_820
  )
set(VAX820_SOURCES
  ${VAX7XX_SOURCES}
  vax820_bi.c
  vax820_ka.c
  vax820_mem.c
  vax820_uba.c
  vax820_stddev.c
  vax820_syslist.c
  )
add_vax_simulator(vax820)

# VAX860 - DEC VAX 8600
set(VAX860_DEFINITIONS
  VAX_860
  )
set(VAX860_SOURCES
  ${VAX7XX_SOURCES}
  ${VAX7XX_MASSBUS_SOURCES}
  vax780_uba.c
  vax860_abus.c
  vax860_sbia.c
  vax860_stddev.c
  vax860_syslist.c
  )
add_vax_simulator(vax860)

