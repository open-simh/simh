## VAX simulators
##
## This is an automagically generated file. Do NOT EDIT.
## Any changes you make will be overwritten!!
##
## Make changes to the SIMH top-level makefile and then run the
## "cmake/generate.py" script to regenerate these files.
##
##     cd cmake; python -m generate --help
##
## ------------------------------------------------------------

if (HAVE_UNITY_FRAMEWORK AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/unit-tests/CMakeLists.txt")
  add_subdirectory(unit-tests)
endif ()


set(VAX
    ${VAXD}/vax_cpu.c
    ${VAXD}/vax_cpu1.c
    ${VAXD}/vax_fpa.c
    ${VAXD}/vax_io.c
    ${VAXD}/vax_cis.c
    ${VAXD}/vax_octa.c
    ${VAXD}/vax_cmode.c
    ${VAXD}/vax_mmu.c
    ${VAXD}/vax_stddev.c
    ${VAXD}/vax_sysdev.c
    ${VAXD}/vax_sys.c
    ${VAXD}/vax_syscm.c
    ${VAXD}/vax_syslist.c
    ${VAXD}/vax_vc.c
    ${VAXD}/vax_lk.c
    ${VAXD}/vax_vs.c
    ${VAXD}/vax_2681.c
    ${PDP11D}/pdp11_rl.c
    ${PDP11D}/pdp11_rq.c
    ${PDP11D}/pdp11_ts.c
    ${PDP11D}/pdp11_dz.c
    ${PDP11D}/pdp11_lp.c
    ${PDP11D}/pdp11_tq.c
    ${PDP11D}/pdp11_xq.c
    ${PDP11D}/pdp11_vh.c
    ${PDP11D}/pdp11_cr.c
    ${PDP11D}/pdp11_td.c
    ${PDP11D}/pdp11_io_lib.c
    ${PDP11D}/pdp11_dup.c)

set(VAX420
    ${VAXD}/vax_cpu.c
    ${VAXD}/vax_cpu1.c
    ${VAXD}/vax_fpa.c
    ${VAXD}/vax_cis.c
    ${VAXD}/vax_octa.c
    ${VAXD}/vax_cmode.c
    ${VAXD}/vax_mmu.c
    ${VAXD}/vax_sys.c
    ${VAXD}/vax_syscm.c
    ${VAXD}/vax_watch.c
    ${VAXD}/vax_nar.c
    ${VAXD}/vax4xx_stddev.c
    ${VAXD}/vax420_sysdev.c
    ${VAXD}/vax420_syslist.c
    ${VAXD}/vax4xx_dz.c
    ${VAXD}/vax4xx_rd.c
    ${VAXD}/vax4xx_rz80.c
    ${VAXD}/vax_xs.c
    ${VAXD}/vax4xx_va.c
    ${VAXD}/vax4xx_vc.c
    ${VAXD}/vax4xx_ve.c
    ${VAXD}/vax_lk.c
    ${VAXD}/vax_vs.c
    ${VAXD}/vax_gpx.c)

set(VAX440
    ${VAXD}/vax_cpu.c
    ${VAXD}/vax_cpu1.c
    ${VAXD}/vax_fpa.c
    ${VAXD}/vax_cis.c
    ${VAXD}/vax_octa.c
    ${VAXD}/vax_cmode.c
    ${VAXD}/vax_mmu.c
    ${VAXD}/vax_sys.c
    ${VAXD}/vax_syscm.c
    ${VAXD}/vax_watch.c
    ${VAXD}/vax_nar.c
    ${VAXD}/vax4xx_stddev.c
    ${VAXD}/vax440_sysdev.c
    ${VAXD}/vax440_syslist.c
    ${VAXD}/vax4xx_dz.c
    ${VAXD}/vax_xs.c
    ${VAXD}/vax_lk.c
    ${VAXD}/vax_vs.c
    ${VAXD}/vax4xx_rz94.c)

set(VAX630
    ${VAXD}/vax_cpu.c
    ${VAXD}/vax_cpu1.c
    ${VAXD}/vax_fpa.c
    ${VAXD}/vax_cis.c
    ${VAXD}/vax_octa.c
    ${VAXD}/vax_cmode.c
    ${VAXD}/vax_mmu.c
    ${VAXD}/vax_sys.c
    ${VAXD}/vax_syscm.c
    ${VAXD}/vax_watch.c
    ${VAXD}/vax630_stddev.c
    ${VAXD}/vax630_sysdev.c
    ${VAXD}/vax630_io.c
    ${VAXD}/vax630_syslist.c
    ${VAXD}/vax_va.c
    ${VAXD}/vax_vc.c
    ${VAXD}/vax_lk.c
    ${VAXD}/vax_vs.c
    ${VAXD}/vax_2681.c
    ${VAXD}/vax_gpx.c
    ${PDP11D}/pdp11_rl.c
    ${PDP11D}/pdp11_rq.c
    ${PDP11D}/pdp11_ts.c
    ${PDP11D}/pdp11_dz.c
    ${PDP11D}/pdp11_lp.c
    ${PDP11D}/pdp11_tq.c
    ${PDP11D}/pdp11_xq.c
    ${PDP11D}/pdp11_vh.c
    ${PDP11D}/pdp11_cr.c
    ${PDP11D}/pdp11_td.c
    ${PDP11D}/pdp11_io_lib.c
    ${PDP11D}/pdp11_dup.c)


if (BUILD_WITH_VIDEO)
    set(INFOSERVER100_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(INFOSERVER100_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(infoserver100
    SOURCES
        ${VAX420}
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_420
        VAX_411
    FULL64
    ${INFOSERVER100_VIDEO}
    LABEL VAX
    TEST vax-diag)

add_simulator(infoserver1000
    SOURCES
        vax_cpu.c
        vax_cpu1.c
        vax_fpa.c
        vax_cis.c
        vax_octa.c
        vax_cmode.c
        vax_mmu.c
        vax_sys.c
        vax_syscm.c
        vax_watch.c
        vax_nar.c
        vax_xs.c
        vax4xx_rz94.c
        vax4nn_stddev.c
        is1000_sysdev.c
        is1000_syslist.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        VM_VAX
        IS_1000
    FULL64
    LABEL VAX
    TEST vax-diag)

if (BUILD_WITH_VIDEO)
    set(INFOSERVER150VXT_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(INFOSERVER150VXT_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(infoserver150vxt
    SOURCES
        ${VAX420}
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_420
        VAX_412
    FULL64
    ${INFOSERVER150VXT_VIDEO}
    LABEL VAX
    TEST vax-diag)

if (BUILD_WITH_VIDEO)
    set(MICROVAX1_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(MICROVAX1_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(microvax1
    SOURCES
        vax_cpu.c
        vax_cpu1.c
        vax_fpa.c
        vax_cis.c
        vax_octa.c
        vax_cmode.c
        vax_mmu.c
        vax_sys.c
        vax_syscm.c
        vax610_stddev.c
        vax610_sysdev.c
        vax610_io.c
        vax610_syslist.c
        vax610_mem.c
        vax_vc.c
        vax_lk.c
        vax_vs.c
        vax_2681.c
        ${PDP11D}/pdp11_rl.c
        ${PDP11D}/pdp11_rq.c
        ${PDP11D}/pdp11_ts.c
        ${PDP11D}/pdp11_dz.c
        ${PDP11D}/pdp11_lp.c
        ${PDP11D}/pdp11_tq.c
        ${PDP11D}/pdp11_xq.c
        ${PDP11D}/pdp11_vh.c
        ${PDP11D}/pdp11_cr.c
        ${PDP11D}/pdp11_td.c
        ${PDP11D}/pdp11_io_lib.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_610
    FULL64
    ${MICROVAX1_VIDEO}
    LABEL VAX
    TEST vax-diag)

if (BUILD_WITH_VIDEO)
    set(MICROVAX2_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(MICROVAX2_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(microvax2
    SOURCES
        ${VAX630}
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_630
    FULL64
    ${MICROVAX2_VIDEO}
    LABEL VAX
    TEST vax-diag)

if (BUILD_WITH_VIDEO)
    set(MICROVAX2000_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(MICROVAX2000_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(microvax2000
    SOURCES
        vax_cpu.c
        vax_cpu1.c
        vax_fpa.c
        vax_cis.c
        vax_octa.c
        vax_cmode.c
        vax_mmu.c
        vax_sys.c
        vax_syscm.c
        vax_watch.c
        vax_nar.c
        vax4xx_stddev.c
        vax410_sysdev.c
        vax410_syslist.c
        vax4xx_dz.c
        vax4xx_rd.c
        vax4xx_rz80.c
        vax_xs.c
        vax4xx_va.c
        vax4xx_vc.c
        vax_lk.c
        vax_vs.c
        vax_gpx.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        VM_VAX
        VAX_410
    FULL64
    ${MICROVAX2000_VIDEO}
    LABEL VAX
    TEST vax-diag)

if (BUILD_WITH_VIDEO)
    set(MICROVAX3100_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(MICROVAX3100_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(microvax3100
    SOURCES
        ${VAX420}
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_420
        VAX_41A
    FULL64
    ${MICROVAX3100_VIDEO}
    LABEL VAX
    TEST vax-diag)

if (BUILD_WITH_VIDEO)
    set(MICROVAX3100E_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(MICROVAX3100E_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(microvax3100e
    SOURCES
        ${VAX420}
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_420
        VAX_41D
    FULL64
    ${MICROVAX3100E_VIDEO}
    LABEL VAX
    TEST vax-diag)

add_simulator(microvax3100m80
    SOURCES
        ${VAX440}
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        VM_VAX
        VAX_440
        VAX_47
    FULL64
    LABEL VAX
    TEST vax-diag)

add_simulator(rtvax1000
    SOURCES
        ${VAX630}
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_620
    FULL64
    LABEL VAX
    TEST vax-diag)

if (BUILD_WITH_VIDEO)
    set(VAX_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(VAX_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(vax
    SOURCES
        ${VAX}
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PDP11D}
    DEFINES
        VM_VAX
    FULL64
    ${VAX_VIDEO}
    LABEL VAX
    TEST vax-diag)

set(vax_symlink_dir_src ${CMAKE_CURRENT_BINARY_DIR})
if (CMAKE_CONFIGURATION_TYPES)
    string(APPEND vax_symlink_dir_src "/$<CONFIG>")
endif (CMAKE_CONFIGURATION_TYPES)

if (NOT WIN32)
    add_custom_command(TARGET vax POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E create_symlink vax${CMAKE_EXECUTABLE_SUFFIX} microvax3900${CMAKE_EXECUTABLE_SUFFIX}
        COMMENT "Symlink vax${CMAKE_EXECUTABLE_SUFFIX} to microvax3900${CMAKE_EXECUTABLE_SUFFIX}"
        WORKING_DIRECTORY ${vax_symlink_dir_src})

    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink vax${CMAKE_EXECUTABLE_SUFFIX} microvax3900${CMAKE_EXECUTABLE_SUFFIX}         WORKING_DIRECTORY \"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")"
    )
else ()
    add_custom_command(TARGET vax POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different vax${CMAKE_EXECUTABLE_SUFFIX} microvax3900${CMAKE_EXECUTABLE_SUFFIX}
        COMMENT "Copy vax${CMAKE_EXECUTABLE_SUFFIX} to microvax3900${CMAKE_EXECUTABLE_SUFFIX}"
        WORKING_DIRECTORY ${vax_symlink_dir_src})

    install(FILES ${vax_symlink_dir_src}/microvax3900${CMAKE_EXECUTABLE_SUFFIX}
            DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}"
    )
endif ()



add_simulator(vax730
    SOURCES
        vax_cpu.c
        vax_cpu1.c
        vax_fpa.c
        vax_cis.c
        vax_octa.c
        vax_cmode.c
        vax_mmu.c
        vax_sys.c
        vax_syscm.c
        vax730_stddev.c
        vax730_sys.c
        vax730_mem.c
        vax730_uba.c
        vax730_rb.c
        vax730_syslist.c
        ${PDP11D}/pdp11_rl.c
        ${PDP11D}/pdp11_rq.c
        ${PDP11D}/pdp11_ts.c
        ${PDP11D}/pdp11_dz.c
        ${PDP11D}/pdp11_lp.c
        ${PDP11D}/pdp11_tq.c
        ${PDP11D}/pdp11_xu.c
        ${PDP11D}/pdp11_ry.c
        ${PDP11D}/pdp11_cr.c
        ${PDP11D}/pdp11_hk.c
        ${PDP11D}/pdp11_vh.c
        ${PDP11D}/pdp11_dmc.c
        ${PDP11D}/pdp11_td.c
        ${PDP11D}/pdp11_tc.c
        ${PDP11D}/pdp11_rk.c
        ${PDP11D}/pdp11_io_lib.c
        ${PDP11D}/pdp11_ch.c
        ${PDP11D}/pdp11_dup.c
    INCLUDES
        VAX
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_730
    FULL64
    LABEL VAX
    TEST vax-diag)

add_simulator(vax750
    SOURCES
        vax_cpu.c
        vax_cpu1.c
        vax_fpa.c
        vax_cis.c
        vax_octa.c
        vax_cmode.c
        vax_mmu.c
        vax_sys.c
        vax_syscm.c
        vax750_stddev.c
        vax750_cmi.c
        vax750_mem.c
        vax750_uba.c
        vax7x0_mba.c
        vax750_syslist.c
        ${PDP11D}/pdp11_rl.c
        ${PDP11D}/pdp11_rq.c
        ${PDP11D}/pdp11_ts.c
        ${PDP11D}/pdp11_dz.c
        ${PDP11D}/pdp11_lp.c
        ${PDP11D}/pdp11_tq.c
        ${PDP11D}/pdp11_xu.c
        ${PDP11D}/pdp11_ry.c
        ${PDP11D}/pdp11_cr.c
        ${PDP11D}/pdp11_hk.c
        ${PDP11D}/pdp11_rp.c
        ${PDP11D}/pdp11_tu.c
        ${PDP11D}/pdp11_vh.c
        ${PDP11D}/pdp11_dmc.c
        ${PDP11D}/pdp11_dup.c
        ${PDP11D}/pdp11_td.c
        ${PDP11D}/pdp11_tc.c
        ${PDP11D}/pdp11_rk.c
        ${PDP11D}/pdp11_io_lib.c
        ${PDP11D}/pdp11_ch.c
    INCLUDES
        VAX
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_750
    FULL64
    LABEL VAX
    TEST vax-diag)

add_simulator(vax780
    SOURCES
        vax_cpu.c
        vax_cpu1.c
        vax_fpa.c
        vax_cis.c
        vax_octa.c
        vax_cmode.c
        vax_mmu.c
        vax_sys.c
        vax_syscm.c
        vax780_stddev.c
        vax780_sbi.c
        vax780_mem.c
        vax780_uba.c
        vax7x0_mba.c
        vax780_fload.c
        vax780_syslist.c
        ${PDP11D}/pdp11_rl.c
        ${PDP11D}/pdp11_rq.c
        ${PDP11D}/pdp11_ts.c
        ${PDP11D}/pdp11_dz.c
        ${PDP11D}/pdp11_lp.c
        ${PDP11D}/pdp11_tq.c
        ${PDP11D}/pdp11_xu.c
        ${PDP11D}/pdp11_ry.c
        ${PDP11D}/pdp11_cr.c
        ${PDP11D}/pdp11_rp.c
        ${PDP11D}/pdp11_tu.c
        ${PDP11D}/pdp11_hk.c
        ${PDP11D}/pdp11_vh.c
        ${PDP11D}/pdp11_dmc.c
        ${PDP11D}/pdp11_dup.c
        ${PDP11D}/pdp11_td.c
        ${PDP11D}/pdp11_tc.c
        ${PDP11D}/pdp11_rk.c
        ${PDP11D}/pdp11_io_lib.c
        ${PDP11D}/pdp11_ch.c
    INCLUDES
        VAX
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_780
    FULL64
    LABEL VAX
    TEST vax-diag)

add_simulator(vax8200
    SOURCES
        vax_cpu.c
        vax_cpu1.c
        vax_fpa.c
        vax_cis.c
        vax_octa.c
        vax_cmode.c
        vax_mmu.c
        vax_sys.c
        vax_syscm.c
        vax_watch.c
        vax820_stddev.c
        vax820_bi.c
        vax820_mem.c
        vax820_uba.c
        vax820_ka.c
        vax820_syslist.c
        ${PDP11D}/pdp11_rl.c
        ${PDP11D}/pdp11_rq.c
        ${PDP11D}/pdp11_ts.c
        ${PDP11D}/pdp11_dz.c
        ${PDP11D}/pdp11_lp.c
        ${PDP11D}/pdp11_tq.c
        ${PDP11D}/pdp11_xu.c
        ${PDP11D}/pdp11_ry.c
        ${PDP11D}/pdp11_cr.c
        ${PDP11D}/pdp11_hk.c
        ${PDP11D}/pdp11_vh.c
        ${PDP11D}/pdp11_dmc.c
        ${PDP11D}/pdp11_td.c
        ${PDP11D}/pdp11_tc.c
        ${PDP11D}/pdp11_rk.c
        ${PDP11D}/pdp11_io_lib.c
        ${PDP11D}/pdp11_ch.c
        ${PDP11D}/pdp11_dup.c
    INCLUDES
        VAX
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_820
    FULL64
    LABEL VAX
    TEST vax-diag)

add_simulator(vax8600
    SOURCES
        vax_cpu.c
        vax_cpu1.c
        vax_fpa.c
        vax_cis.c
        vax_octa.c
        vax_cmode.c
        vax_mmu.c
        vax_sys.c
        vax_syscm.c
        vax860_stddev.c
        vax860_sbia.c
        vax860_abus.c
        vax780_uba.c
        vax7x0_mba.c
        vax860_syslist.c
        ${PDP11D}/pdp11_rl.c
        ${PDP11D}/pdp11_rq.c
        ${PDP11D}/pdp11_ts.c
        ${PDP11D}/pdp11_dz.c
        ${PDP11D}/pdp11_lp.c
        ${PDP11D}/pdp11_tq.c
        ${PDP11D}/pdp11_xu.c
        ${PDP11D}/pdp11_ry.c
        ${PDP11D}/pdp11_cr.c
        ${PDP11D}/pdp11_rp.c
        ${PDP11D}/pdp11_tu.c
        ${PDP11D}/pdp11_hk.c
        ${PDP11D}/pdp11_vh.c
        ${PDP11D}/pdp11_dmc.c
        ${PDP11D}/pdp11_dup.c
        ${PDP11D}/pdp11_td.c
        ${PDP11D}/pdp11_tc.c
        ${PDP11D}/pdp11_rk.c
        ${PDP11D}/pdp11_io_lib.c
        ${PDP11D}/pdp11_ch.c
    INCLUDES
        VAX
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_860
    FULL64
    LABEL VAX
    TEST vax-diag)

if (BUILD_WITH_VIDEO)
    set(VAXSTATION3100M30_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(VAXSTATION3100M30_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(vaxstation3100m30
    SOURCES
        ${VAX420}
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_420
        VAX_42A
    FULL64
    ${VAXSTATION3100M30_VIDEO}
    LABEL VAX
    TEST vax-diag)

if (BUILD_WITH_VIDEO)
    set(VAXSTATION3100M38_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(VAXSTATION3100M38_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(vaxstation3100m38
    SOURCES
        ${VAX420}
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_420
        VAX_42B
    FULL64
    ${VAXSTATION3100M38_VIDEO}
    LABEL VAX
    TEST vax-diag)

if (BUILD_WITH_VIDEO)
    set(VAXSTATION3100M76_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(VAXSTATION3100M76_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(vaxstation3100m76
    SOURCES
        vax_cpu.c
        vax_cpu1.c
        vax_fpa.c
        vax_cis.c
        vax_octa.c
        vax_cmode.c
        vax_mmu.c
        vax_sys.c
        vax_syscm.c
        vax_watch.c
        vax_nar.c
        vax4xx_stddev.c
        vax43_sysdev.c
        vax43_syslist.c
        vax4xx_dz.c
        vax4xx_rz80.c
        vax_xs.c
        vax4xx_vc.c
        vax4xx_ve.c
        vax_lk.c
        vax_vs.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        VM_VAX
        VAX_43
    FULL64
    ${VAXSTATION3100M76_VIDEO}
    LABEL VAX
    TEST vax-diag)

add_simulator(vaxstation4000m60
    SOURCES
        ${VAX440}
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        VM_VAX
        VAX_440
        VAX_46
    FULL64
    LABEL VAX
    TEST vax-diag)

add_simulator(vaxstation4000vlc
    SOURCES
        ${VAX440}
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        VM_VAX
        VAX_440
        VAX_48
    FULL64
    LABEL VAX
    TEST vax-diag)
