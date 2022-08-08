## ------------------------------------------------------------
## This is an automagically generated file. Do NOT EDIT.
## Any changes you make will be overwritten!!
##
## Make changes to the SIMH top-level makefile and use the
## cmake/generate.py script to regenerate these files.
##
##     python -m cmake/generate --help
##
## ------------------------------------------------------------
set(ALTAIRD    "${CMAKE_SOURCE_DIR}/ALTAIR")
set(ALTAIRZ80D "${CMAKE_SOURCE_DIR}/AltairZ80")
set(ATT3B2D    "${CMAKE_SOURCE_DIR}/3B2")
set(B5500D     "${CMAKE_SOURCE_DIR}/B5500")
set(BESM6D     "${CMAKE_SOURCE_DIR}/BESM6")
set(CDC1700D   "${CMAKE_SOURCE_DIR}/CDC1700")
set(GRID       "${CMAKE_SOURCE_DIR}/GRI")
set(H316D      "${CMAKE_SOURCE_DIR}/H316")
set(HP2100D    "${CMAKE_SOURCE_DIR}/HP2100")
set(HP3000D    "${CMAKE_SOURCE_DIR}/HP3000")
set(I1401D     "${CMAKE_SOURCE_DIR}/I1401")
set(I1620D     "${CMAKE_SOURCE_DIR}/I1620")
set(I650D      "${CMAKE_SOURCE_DIR}/I650")
set(I7000D     "${CMAKE_SOURCE_DIR}/I7000")
set(I7010D     "${CMAKE_SOURCE_DIR}/I7000")
set(I7094D     "${CMAKE_SOURCE_DIR}/I7094")
set(IBM1130D   "${CMAKE_SOURCE_DIR}/Ibm1130")
set(ID16D      "${CMAKE_SOURCE_DIR}/Interdata")
set(ID32D      "${CMAKE_SOURCE_DIR}/Interdata")
set(IMLACD     "${CMAKE_SOURCE_DIR}/imlac")
set(INTELSYSC  "${CMAKE_SOURCE_DIR}/Intel-Systems/common")
set(KA10D      "${CMAKE_SOURCE_DIR}/PDP10")
set(KI10D      "${CMAKE_SOURCE_DIR}/PDP10")
set(KL10D      "${CMAKE_SOURCE_DIR}/PDP10")
set(KS10D      "${CMAKE_SOURCE_DIR}/PDP10")
set(LGPD       "${CMAKE_SOURCE_DIR}/LGP")
set(NOVAD      "${CMAKE_SOURCE_DIR}/NOVA")
set(PDP10D     "${CMAKE_SOURCE_DIR}/PDP10")
set(PDP11D     "${CMAKE_SOURCE_DIR}/PDP11")
set(PDP18BD    "${CMAKE_SOURCE_DIR}/PDP18B")
set(PDP1D      "${CMAKE_SOURCE_DIR}/PDP1")
set(PDP6D      "${CMAKE_SOURCE_DIR}/PDP10")
set(PDP8D      "${CMAKE_SOURCE_DIR}/PDP8")
set(S3D        "${CMAKE_SOURCE_DIR}/S3")
set(SDSD       "${CMAKE_SOURCE_DIR}/SDS")
set(SIGMAD     "${CMAKE_SOURCE_DIR}/sigma")
set(SSEMD      "${CMAKE_SOURCE_DIR}/SSEM")
set(TT2500D    "${CMAKE_SOURCE_DIR}/tt2500")
set(TX0D       "${CMAKE_SOURCE_DIR}/TX-0")
set(UC15D      "${CMAKE_SOURCE_DIR}/PDP11")
set(VAXD       "${CMAKE_SOURCE_DIR}/VAX")

set(DISPLAYD   "${CMAKE_SOURCE_DIR}/display")
set(DISPLAY340 "${DISPLAYD}/type340.c")
set(DISPLAYIII "${DISPLAYD}/iii.c")
set(DISPLAYNG  "${DISPLAYD}/ng.c")
set(DISPLAYVT  "${DISPLAYD}/vt11.c")

set(INTELSYSD  "${CMAKE_SOURCE_DIR}/Intel-Systems")
set(INTEL_MDSD "${INTELSYSD}/Intel-MDS")
set(SCELBIC    "${INTELSYSD}/common")
set(SCELBID    "${INTELSYSD}/scelbi")

## ----------------------------------------

add_simulator(3b2
    SOURCES
        ${ATT3B2D}/3b2_cpu.c
        ${ATT3B2D}/3b2_sys.c
        ${ATT3B2D}/3b2_rev2_sys.c
        ${ATT3B2D}/3b2_rev2_mmu.c
        ${ATT3B2D}/3b2_rev2_mau.c
        ${ATT3B2D}/3b2_rev2_csr.c
        ${ATT3B2D}/3b2_rev2_timer.c
        ${ATT3B2D}/3b2_stddev.c
        ${ATT3B2D}/3b2_mem.c
        ${ATT3B2D}/3b2_iu.c
        ${ATT3B2D}/3b2_if.c
        ${ATT3B2D}/3b2_id.c
        ${ATT3B2D}/3b2_dmac.c
        ${ATT3B2D}/3b2_io.c
        ${ATT3B2D}/3b2_ports.c
        ${ATT3B2D}/3b2_ctc.c
        ${ATT3B2D}/3b2_ni.c
    INCLUDES
        ${ATT3B2D}
    DEFINES
        REV2
    FULL64
    LABEL 3B2
    TEST 3b2
    SOURCE_DIR ${ATT3B2D})

## ----------------------------------------

add_simulator(altair
    SOURCES
        ${ALTAIRD}/altair_sio.c
        ${ALTAIRD}/altair_cpu.c
        ${ALTAIRD}/altair_dsk.c
        ${ALTAIRD}/altair_sys.c
    INCLUDES
        ${ALTAIRD}
    LABEL ALTAIR
    TEST altair
    SOURCE_DIR ${ALTAIRD})

## ----------------------------------------

add_simulator(altairz80
    SOURCES
        ${ALTAIRZ80D}/altairz80_cpu.c
        ${ALTAIRZ80D}/altairz80_cpu_nommu.c
        ${ALTAIRZ80D}/s100_dj2d.c
        ${ALTAIRZ80D}/altairz80_dsk.c
        ${ALTAIRZ80D}/disasm.c
        ${ALTAIRZ80D}/altairz80_sio.c
        ${ALTAIRZ80D}/altairz80_sys.c
        ${ALTAIRZ80D}/altairz80_hdsk.c
        ${ALTAIRZ80D}/altairz80_net.c
        ${ALTAIRZ80D}/s100_hayes.c
        ${ALTAIRZ80D}/s100_2sio.c
        ${ALTAIRZ80D}/s100_pmmi.c
        ${ALTAIRZ80D}/flashwriter2.c
        ${ALTAIRZ80D}/i86_decode.c
        ${ALTAIRZ80D}/i86_ops.c
        ${ALTAIRZ80D}/i86_prim_ops.c
        ${ALTAIRZ80D}/i8272.c
        ${ALTAIRZ80D}/insnsd.c
        ${ALTAIRZ80D}/altairz80_mhdsk.c
        ${ALTAIRZ80D}/mfdc.c
        ${ALTAIRZ80D}/n8vem.c
        ${ALTAIRZ80D}/vfdhd.c
        ${ALTAIRZ80D}/s100_disk1a.c
        ${ALTAIRZ80D}/s100_disk2.c
        ${ALTAIRZ80D}/s100_disk3.c
        ${ALTAIRZ80D}/s100_fif.c
        ${ALTAIRZ80D}/s100_mdriveh.c
        ${ALTAIRZ80D}/s100_icom.c
        ${ALTAIRZ80D}/s100_jadedd.c
        ${ALTAIRZ80D}/s100_mdsa.c
        ${ALTAIRZ80D}/s100_mdsad.c
        ${ALTAIRZ80D}/s100_selchan.c
        ${ALTAIRZ80D}/s100_ss1.c
        ${ALTAIRZ80D}/s100_64fdc.c
        ${ALTAIRZ80D}/s100_scp300f.c
        ${ALTAIRZ80D}/s100_tarbell.c
        ${ALTAIRZ80D}/wd179x.c
        ${ALTAIRZ80D}/s100_hdc1001.c
        ${ALTAIRZ80D}/s100_if3.c
        ${ALTAIRZ80D}/s100_adcs6.c
        ${ALTAIRZ80D}/m68kcpu.c
        ${ALTAIRZ80D}/m68kdasm.c
        ${ALTAIRZ80D}/m68kasm.c
        ${ALTAIRZ80D}/m68kopac.c
        ${ALTAIRZ80D}/m68kopdm.c
        ${ALTAIRZ80D}/m68kopnz.c
        ${ALTAIRZ80D}/m68kops.c
        ${ALTAIRZ80D}/m68ksim.c
    INCLUDES
        ${ALTAIRZ80D}
    LABEL AltairZ80
    TEST altairz80
    SOURCE_DIR ${ALTAIRZ80D})

## ----------------------------------------

add_simulator(b5500
    SOURCES
        ${B5500D}/b5500_cpu.c
        ${B5500D}/b5500_io.c
        ${B5500D}/b5500_sys.c
        ${B5500D}/b5500_dk.c
        ${B5500D}/b5500_mt.c
        ${B5500D}/b5500_urec.c
        ${B5500D}/b5500_dr.c
        ${B5500D}/b5500_dtc.c
    INCLUDES
        ..
    DEFINES
        B5500
    INT64
    LABEL B5500
    TEST b5500
    SOURCE_DIR ${B5500D})

## ----------------------------------------

set(besm6_font)
foreach (fdir IN ITEMS
           "/usr/share/fonts" "/Library/Fonts" "/usr/lib/jvm"
           "/System/Library/Frameworks/JavaVM.framework/Versions"
           "$ENV{WINDIR}/Fonts")
    foreach (font IN ITEMS
                "DejaVuSans.ttf" "LucidaSansRegular.ttf"
                "FreeSans.ttf" "AppleGothic.ttf" "tahoma.ttf")
        if (EXISTS ${fdir}/${font})
            get_filename_component(fontfile ${fdir}/${font} ABSOLUTE)
            list(APPEND besm6_font ${fontfile})
        endif ()
    endforeach()
endforeach()

if (besm6_font)
    list(LENGTH besm6_font besm6_font_len)
    if (besm6_font_len GREATER 1)
        message(STATUS "BESM6: Fonts found ${besm6_font}")
    endif ()
    list(GET besm6_font 0 besm6_font)
    message(STATUS "BESM6: Using ${besm6_font}")
endif (besm6_font)

if (besm6_font)
    if (BUILD_WITH_VIDEO)
        add_simulator(besm6
            SOURCES
                ${BESM6D}/besm6_cpu.c
                ${BESM6D}/besm6_sys.c
                ${BESM6D}/besm6_mmu.c
                ${BESM6D}/besm6_arith.c
                ${BESM6D}/besm6_disk.c
                ${BESM6D}/besm6_drum.c
                ${BESM6D}/besm6_tty.c
                ${BESM6D}/besm6_panel.c
                ${BESM6D}/besm6_printer.c
                ${BESM6D}/besm6_pl.c
                ${BESM6D}/besm6_mg.c
                ${BESM6D}/besm6_punch.c
                ${BESM6D}/besm6_punchcard.c
                ${BESM6D}/besm6_vu.c
            INCLUDES
                ${BESM6D}
            DEFINES
                FONTFILE=${besm6_font}
            INT64
            VIDEO
            LABEL BESM6
            TEST besm6
            SOURCE_DIR ${BESM6D})
    endif (BUILD_WITH_VIDEO)
endif ()

## ----------------------------------------

add_simulator(cdc1700
    SOURCES
        ${CDC1700D}/cdc1700_cpu.c
        ${CDC1700D}/cdc1700_dis.c
        ${CDC1700D}/cdc1700_io.c
        ${CDC1700D}/cdc1700_sys.c
        ${CDC1700D}/cdc1700_dev1.c
        ${CDC1700D}/cdc1700_mt.c
        ${CDC1700D}/cdc1700_dc.c
        ${CDC1700D}/cdc1700_iofw.c
        ${CDC1700D}/cdc1700_lp.c
        ${CDC1700D}/cdc1700_dp.c
        ${CDC1700D}/cdc1700_cd.c
        ${CDC1700D}/cdc1700_sym.c
        ${CDC1700D}/cdc1700_rtc.c
        ${CDC1700D}/cdc1700_drm.c
        ${CDC1700D}/cdc1700_msos5.c
    INCLUDES
        ${CDC1700D}
    LABEL CDC1700
    TEST cdc1700
    SOURCE_DIR ${CDC1700D})

## ----------------------------------------

add_simulator(gri
    SOURCES
        ${GRID}/gri_cpu.c
        ${GRID}/gri_stddev.c
        ${GRID}/gri_sys.c
    INCLUDES
        ${GRID}
    LABEL GRI
    TEST gri
    SOURCE_DIR ${GRID})

## ----------------------------------------

add_simulator(h316
    SOURCES
        ${H316D}/h316_stddev.c
        ${H316D}/h316_lp.c
        ${H316D}/h316_cpu.c
        ${H316D}/h316_sys.c
        ${H316D}/h316_mt.c
        ${H316D}/h316_fhd.c
        ${H316D}/h316_dp.c
        ${H316D}/h316_rtc.c
        ${H316D}/h316_imp.c
        ${H316D}/h316_hi.c
        ${H316D}/h316_mi.c
        ${H316D}/h316_udp.c
    INCLUDES
        ${H316D}
    DEFINES
        VM_IMPTIP
    LABEL H316
    TEST h316
    SOURCE_DIR ${H316D})

## ----------------------------------------

add_simulator(hp2100
    SOURCES
        ${HP2100D}/hp2100_baci.c
        ${HP2100D}/hp2100_cpu.c
        ${HP2100D}/hp2100_cpu_fp.c
        ${HP2100D}/hp2100_cpu_fpp.c
        ${HP2100D}/hp2100_cpu0.c
        ${HP2100D}/hp2100_cpu1.c
        ${HP2100D}/hp2100_cpu2.c
        ${HP2100D}/hp2100_cpu3.c
        ${HP2100D}/hp2100_cpu4.c
        ${HP2100D}/hp2100_cpu5.c
        ${HP2100D}/hp2100_cpu6.c
        ${HP2100D}/hp2100_cpu7.c
        ${HP2100D}/hp2100_di.c
        ${HP2100D}/hp2100_di_da.c
        ${HP2100D}/hp2100_disclib.c
        ${HP2100D}/hp2100_dma.c
        ${HP2100D}/hp2100_dp.c
        ${HP2100D}/hp2100_dq.c
        ${HP2100D}/hp2100_dr.c
        ${HP2100D}/hp2100_ds.c
        ${HP2100D}/hp2100_ipl.c
        ${HP2100D}/hp2100_lps.c
        ${HP2100D}/hp2100_lpt.c
        ${HP2100D}/hp2100_mc.c
        ${HP2100D}/hp2100_mem.c
        ${HP2100D}/hp2100_mpx.c
        ${HP2100D}/hp2100_ms.c
        ${HP2100D}/hp2100_mt.c
        ${HP2100D}/hp2100_mux.c
        ${HP2100D}/hp2100_pif.c
        ${HP2100D}/hp2100_pt.c
        ${HP2100D}/hp2100_sys.c
        ${HP2100D}/hp2100_tbg.c
        ${HP2100D}/hp2100_tty.c
    INCLUDES
        ${HP2100D}
    DEFINES
        HAVE_INT64
    LABEL HP2100
    TEST hp2100
    SOURCE_DIR ${HP2100D})

## ----------------------------------------

add_simulator(hp3000
    SOURCES
        ${HP3000D}/hp_disclib.c
        ${HP3000D}/hp_tapelib.c
        ${HP3000D}/hp3000_atc.c
        ${HP3000D}/hp3000_clk.c
        ${HP3000D}/hp3000_cpu.c
        ${HP3000D}/hp3000_cpu_base.c
        ${HP3000D}/hp3000_cpu_fp.c
        ${HP3000D}/hp3000_cpu_cis.c
        ${HP3000D}/hp3000_ds.c
        ${HP3000D}/hp3000_iop.c
        ${HP3000D}/hp3000_lp.c
        ${HP3000D}/hp3000_mem.c
        ${HP3000D}/hp3000_mpx.c
        ${HP3000D}/hp3000_ms.c
        ${HP3000D}/hp3000_scmb.c
        ${HP3000D}/hp3000_sel.c
        ${HP3000D}/hp3000_sys.c
    INCLUDES
        ${HP3000D}
    LABEL HP3000
    TEST hp3000
    SOURCE_DIR ${HP3000D})

## ----------------------------------------

add_simulator(i1401
    SOURCES
        ${I1401D}/i1401_lp.c
        ${I1401D}/i1401_cpu.c
        ${I1401D}/i1401_iq.c
        ${I1401D}/i1401_cd.c
        ${I1401D}/i1401_mt.c
        ${I1401D}/i1401_dp.c
        ${I1401D}/i1401_sys.c
    INCLUDES
        ${I1401D}
    LABEL I1401
    TEST i1401
    SOURCE_DIR ${I1401D})

## ----------------------------------------

add_simulator(i1620
    SOURCES
        ${I1620D}/i1620_cd.c
        ${I1620D}/i1620_dp.c
        ${I1620D}/i1620_pt.c
        ${I1620D}/i1620_tty.c
        ${I1620D}/i1620_cpu.c
        ${I1620D}/i1620_lp.c
        ${I1620D}/i1620_fp.c
        ${I1620D}/i1620_sys.c
    INCLUDES
        ${I1620D}
    LABEL I1620
    TEST i1620
    SOURCE_DIR ${I1620D})

## ----------------------------------------

add_simulator(i650
    SOURCES
        ${I650D}/i650_cpu.c
        ${I650D}/i650_cdr.c
        ${I650D}/i650_cdp.c
        ${I650D}/i650_dsk.c
        ${I650D}/i650_mt.c
        ${I650D}/i650_sys.c
    INCLUDES
        ${I650D}
    INT64
    LABEL I650
    TEST i650
    SOURCE_DIR ${I650D})

if (WIN32)
    if (MSVC)
        set(I650_STACK_FLAG "/STACK:8388608")
    else ()
        set(I650_STACK_FLAG "-Wl,--stack,8388608")
    endif ()
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.13")
        target_link_options(i650 PUBLIC "${I650_STACK_FLAG}")
    else ()
        set_property(TARGET i650 LINK_FLAGS " ${I650_STACK_FLAG}")
    endif ()
endif()
## ----------------------------------------

add_simulator(i701
    SOURCES
        ${I7000D}/i701_cpu.c
        ${I7000D}/i701_sys.c
        ${I7000D}/i701_chan.c
        ${I7000D}/i7090_cdr.c
        ${I7000D}/i7090_cdp.c
        ${I7000D}/i7090_lpr.c
        ${I7000D}/i7000_mt.c
        ${I7000D}/i7090_drum.c
        ${I7000D}/i7000_chan.c
    INCLUDES
        ${I7000D}
    DEFINES
        I701
    INT64
    LABEL I7000
    TEST i701
    SOURCE_DIR ${I701D})

add_simulator(i7010
    SOURCES
        ${I7000D}/i7010_cpu.c
        ${I7000D}/i7010_sys.c
        ${I7000D}/i7010_chan.c
        ${I7000D}/i7000_cdp.c
        ${I7000D}/i7000_cdr.c
        ${I7000D}/i7000_con.c
        ${I7000D}/i7000_chan.c
        ${I7000D}/i7000_lpr.c
        ${I7000D}/i7000_mt.c
        ${I7000D}/i7000_chron.c
        ${I7000D}/i7000_dsk.c
        ${I7000D}/i7000_com.c
        ${I7000D}/i7000_ht.c
    INCLUDES
        ${I7010D}
    DEFINES
        I7010
    LABEL I7000
    TEST i7010
    SOURCE_DIR ${I7010D})

add_simulator(i704
    SOURCES
        ${I7000D}/i7090_cpu.c
        ${I7000D}/i7090_sys.c
        ${I7000D}/i7090_chan.c
        ${I7000D}/i7090_cdr.c
        ${I7000D}/i7090_cdp.c
        ${I7000D}/i7090_lpr.c
        ${I7000D}/i7000_mt.c
        ${I7000D}/i7090_drum.c
        ${I7000D}/i7000_chan.c
    INCLUDES
        ${I7000D}
    DEFINES
        I704
    INT64
    LABEL I7000
    TEST i704
    SOURCE_DIR ${I704D})

add_simulator(i7070
    SOURCES
        ${I7000D}/i7070_cpu.c
        ${I7000D}/i7070_sys.c
        ${I7000D}/i7070_chan.c
        ${I7000D}/i7000_cdp.c
        ${I7000D}/i7000_cdr.c
        ${I7000D}/i7000_con.c
        ${I7000D}/i7000_chan.c
        ${I7000D}/i7000_lpr.c
        ${I7000D}/i7000_mt.c
        ${I7000D}/i7000_chron.c
        ${I7000D}/i7000_dsk.c
        ${I7000D}/i7000_com.c
        ${I7000D}/i7000_ht.c
    INCLUDES
        ${I7000D}
    DEFINES
        I7070
    INT64
    LABEL I7000
    TEST i7070
    SOURCE_DIR ${I7070D})

add_simulator(i7080
    SOURCES
        ${I7000D}/i7080_cpu.c
        ${I7000D}/i7080_sys.c
        ${I7000D}/i7080_chan.c
        ${I7000D}/i7080_drum.c
        ${I7000D}/i7000_cdp.c
        ${I7000D}/i7000_cdr.c
        ${I7000D}/i7000_con.c
        ${I7000D}/i7000_chan.c
        ${I7000D}/i7000_lpr.c
        ${I7000D}/i7000_mt.c
        ${I7000D}/i7000_chron.c
        ${I7000D}/i7000_dsk.c
        ${I7000D}/i7000_com.c
        ${I7000D}/i7000_ht.c
    INCLUDES
        ${I7000D}
    DEFINES
        I7080
    LABEL I7000
    TEST i7080
    SOURCE_DIR ${I7080D})

add_simulator(i7090
    SOURCES
        ${I7000D}/i7090_cpu.c
        ${I7000D}/i7090_sys.c
        ${I7000D}/i7090_chan.c
        ${I7000D}/i7090_cdr.c
        ${I7000D}/i7090_cdp.c
        ${I7000D}/i7090_lpr.c
        ${I7000D}/i7000_chan.c
        ${I7000D}/i7000_mt.c
        ${I7000D}/i7090_drum.c
        ${I7000D}/i7090_hdrum.c
        ${I7000D}/i7000_chron.c
        ${I7000D}/i7000_dsk.c
        ${I7000D}/i7000_com.c
        ${I7000D}/i7000_ht.c
    INCLUDES
        ${I7000D}
    DEFINES
        I7090
    INT64
    LABEL I7000
    TEST i7090
    SOURCE_DIR ${I7000D})

## ----------------------------------------

add_simulator(i7094
    SOURCES
        ${I7094D}/i7094_cpu.c
        ${I7094D}/i7094_cpu1.c
        ${I7094D}/i7094_io.c
        ${I7094D}/i7094_cd.c
        ${I7094D}/i7094_clk.c
        ${I7094D}/i7094_com.c
        ${I7094D}/i7094_drm.c
        ${I7094D}/i7094_dsk.c
        ${I7094D}/i7094_sys.c
        ${I7094D}/i7094_lp.c
        ${I7094D}/i7094_mt.c
        ${I7094D}/i7094_binloader.c
    INCLUDES
        ${I7094D}
    INT64
    LABEL I7094
    TEST i7094
    SOURCE_DIR ${I7094D})

## ----------------------------------------

add_simulator(ibm1130
    SOURCES
        ${IBM1130D}/ibm1130_cpu.c
        ${IBM1130D}/ibm1130_cr.c
        ${IBM1130D}/ibm1130_disk.c
        ${IBM1130D}/ibm1130_stddev.c
        ${IBM1130D}/ibm1130_sys.c
        ${IBM1130D}/ibm1130_gdu.c
        ${IBM1130D}/ibm1130_gui.c
        ${IBM1130D}/ibm1130_prt.c
        ${IBM1130D}/ibm1130_fmt.c
        ${IBM1130D}/ibm1130_ptrp.c
        ${IBM1130D}/ibm1130_plot.c
        ${IBM1130D}/ibm1130_sca.c
        ${IBM1130D}/ibm1130_t2741.c
    INCLUDES
        ${IBM1130D}
    LABEL Ibm1130
    TEST ibm1130
    SOURCE_DIR ${IBM1130D})

## ----------------------------------------

add_simulator(intel-mds
    SOURCES
        ${INTELSYSC}/i8080.c
        ${INTEL_MDSD}/imds_sys.c
        ${INTELSYSC}/i3214.c
        ${INTELSYSC}/i8251.c
        ${INTELSYSC}/i8253.c
        ${INTELSYSC}/i8255.c
        ${INTELSYSC}/i8259.c
        ${INTELSYSC}/ieprom.c
        ${INTELSYSC}/ioc-cont.c
        ${INTELSYSC}/ipc-cont.c
        ${INTELSYSC}/iram8.c
        ${INTELSYSC}/isbc064.c
        ${INTELSYSC}/isbc202.c
        ${INTELSYSC}/isbc201.c
        ${INTELSYSC}/isbc206.c
        ${INTELSYSC}/isbc464.c
        ${INTELSYSC}/isbc208.c
        ${INTELSYSC}/port.c
        ${INTELSYSC}/irq.c
        ${INTELSYSC}/multibus.c
        ${INTELSYSC}/mem.c
        ${INTELSYSC}/sys.c
        ${INTELSYSC}/zx200a.c
    INCLUDES
        ${INTEL_MDSD}
    LABEL Intel-Systems/Intel-MDS
    TEST intel-mds
    SOURCE_DIR ${INTEL_MDSD})

## ----------------------------------------

add_simulator(scelbi
    SOURCES
        ${SCELBIC}/i8008.c
        ${SCELBID}/scelbi_sys.c
        ${SCELBID}/scelbi_io.c
    INCLUDES
        ${SCELBID}
    LABEL Intel-Systems/scelbi
    TEST scelbi
    SOURCE_DIR ${SCELBID})

## ----------------------------------------

add_simulator(id16
    SOURCES
        ${ID16D}/id16_cpu.c
        ${ID16D}/id16_sys.c
        ${ID16D}/id_dp.c
        ${ID16D}/id_fd.c
        ${ID16D}/id_fp.c
        ${ID16D}/id_idc.c
        ${ID16D}/id_io.c
        ${ID16D}/id_lp.c
        ${ID16D}/id_mt.c
        ${ID16D}/id_pas.c
        ${ID16D}/id_pt.c
        ${ID16D}/id_tt.c
        ${ID16D}/id_uvc.c
        ${ID16D}/id16_dboot.c
        ${ID16D}/id_ttp.c
    INCLUDES
        ${ID16D}
    LABEL Interdata
    TEST id16
    SOURCE_DIR ${ID32D})

add_simulator(id32
    SOURCES
        ${ID32D}/id32_cpu.c
        ${ID32D}/id32_sys.c
        ${ID32D}/id_dp.c
        ${ID32D}/id_fd.c
        ${ID32D}/id_fp.c
        ${ID32D}/id_idc.c
        ${ID32D}/id_io.c
        ${ID32D}/id_lp.c
        ${ID32D}/id_mt.c
        ${ID32D}/id_pas.c
        ${ID32D}/id_pt.c
        ${ID32D}/id_tt.c
        ${ID32D}/id_uvc.c
        ${ID32D}/id32_dboot.c
        ${ID32D}/id_ttp.c
    INCLUDES
        ${ID32D}
    LABEL Interdata
    TEST id32
    SOURCE_DIR ${ID32D})

## ----------------------------------------

add_simulator(lgp
    SOURCES
        ${LGPD}/lgp_cpu.c
        ${LGPD}/lgp_stddev.c
        ${LGPD}/lgp_sys.c
    INCLUDES
        ${LGPD}
    LABEL LGP
    TEST lgp
    SOURCE_DIR ${LGPD})

## ----------------------------------------

add_simulator(eclipse
    SOURCES
        ${NOVAD}/eclipse_cpu.c
        ${NOVAD}/eclipse_tt.c
        ${NOVAD}/nova_sys.c
        ${NOVAD}/nova_dkp.c
        ${NOVAD}/nova_dsk.c
        ${NOVAD}/nova_lp.c
        ${NOVAD}/nova_mta.c
        ${NOVAD}/nova_plt.c
        ${NOVAD}/nova_pt.c
        ${NOVAD}/nova_clk.c
        ${NOVAD}/nova_tt1.c
        ${NOVAD}/nova_qty.c
    INCLUDES
        ${NOVAD}
    DEFINES
        ECLIPSE
    INT64
    LABEL NOVA
    TEST eclipse
    SOURCE_DIR ${NOVAD})

add_simulator(nova
    SOURCES
        ${NOVAD}/nova_sys.c
        ${NOVAD}/nova_cpu.c
        ${NOVAD}/nova_dkp.c
        ${NOVAD}/nova_dsk.c
        ${NOVAD}/nova_lp.c
        ${NOVAD}/nova_mta.c
        ${NOVAD}/nova_plt.c
        ${NOVAD}/nova_pt.c
        ${NOVAD}/nova_clk.c
        ${NOVAD}/nova_tt.c
        ${NOVAD}/nova_tt1.c
        ${NOVAD}/nova_qty.c
    INCLUDES
        ${NOVAD}
    LABEL NOVA
    TEST nova
    SOURCE_DIR ${NOVAD})

## ----------------------------------------

if (BUILD_WITH_VIDEO)
    add_simulator(pdp1
        SOURCES
            ${PDP1D}/pdp1_lp.c
            ${PDP1D}/pdp1_cpu.c
            ${PDP1D}/pdp1_stddev.c
            ${PDP1D}/pdp1_sys.c
            ${PDP1D}/pdp1_dt.c
            ${PDP1D}/pdp1_drm.c
            ${PDP1D}/pdp1_clk.c
            ${PDP1D}/pdp1_dcs.c
            ${PDP1D}/pdp1_dpy.c
        INCLUDES
            ${PDP1D}
        DEFINES
            DISPLAY_TYPE=DIS_TYPE30
            PIX_SCALE=RES_HALF
        VIDEO
        LABEL PDP1
        TEST pdp1
        SOURCE_DIR ${PDP1D})
endif (BUILD_WITH_VIDEO)

## ----------------------------------------


set(DISPLAY340
    ${DISPLAYD}/type340.c)


add_simulator(pdp10
    SOURCES
        ${PDP10D}/pdp10_fe.c
        ${PDP11D}/pdp11_dz.c
        ${PDP10D}/pdp10_cpu.c
        ${PDP10D}/pdp10_ksio.c
        ${PDP10D}/pdp10_lp20.c
        ${PDP10D}/pdp10_mdfp.c
        ${PDP10D}/pdp10_pag.c
        ${PDP10D}/pdp10_rp.c
        ${PDP10D}/pdp10_sys.c
        ${PDP10D}/pdp10_tim.c
        ${PDP10D}/pdp10_tu.c
        ${PDP10D}/pdp10_xtnd.c
        ${PDP11D}/pdp11_pt.c
        ${PDP11D}/pdp11_ry.c
        ${PDP11D}/pdp11_cr.c
        ${PDP11D}/pdp11_dup.c
        ${PDP11D}/pdp11_dmc.c
        ${PDP11D}/pdp11_kmc.c
        ${PDP11D}/pdp11_xu.c
        ${PDP11D}/pdp11_ch.c
    INCLUDES
        ${PDP10D}
        ${PDP11D}
    DEFINES
        VM_PDP10
    INT64
    LABEL PDP10
    TEST pdp10
    SOURCE_DIR ${PDP10D})

if (BUILD_WITH_VIDEO)
    add_simulator(pdp10-ka
        SOURCES
            ${KA10D}/kx10_cpu.c
            ${KA10D}/kx10_sys.c
            ${KA10D}/kx10_df.c
            ${KA10D}/kx10_dp.c
            ${KA10D}/kx10_mt.c
            ${KA10D}/kx10_cty.c
            ${KA10D}/kx10_lp.c
            ${KA10D}/kx10_pt.c
            ${KA10D}/kx10_dc.c
            ${KA10D}/kx10_rp.c
            ${KA10D}/kx10_rc.c
            ${KA10D}/kx10_dt.c
            ${KA10D}/kx10_dk.c
            ${KA10D}/kx10_cr.c
            ${KA10D}/kx10_cp.c
            ${KA10D}/kx10_tu.c
            ${KA10D}/kx10_rs.c
            ${KA10D}/ka10_pd.c
            ${KA10D}/kx10_rh.c
            ${KA10D}/kx10_imp.c
            ${KA10D}/ka10_tk10.c
            ${KA10D}/ka10_mty.c
            ${KA10D}/ka10_imx.c
            ${KA10D}/ka10_ch10.c
            ${KA10D}/ka10_stk.c
            ${KA10D}/ka10_ten11.c
            ${KA10D}/ka10_auxcpu.c
            ${KA10D}/ka10_pmp.c
            ${KA10D}/ka10_dkb.c
            ${KA10D}/pdp6_dct.c
            ${KA10D}/pdp6_dtc.c
            ${KA10D}/pdp6_mtc.c
            ${KA10D}/pdp6_dsk.c
            ${KA10D}/pdp6_dcs.c
            ${KA10D}/ka10_dpk.c
            ${KA10D}/kx10_dpy.c
            ${KA10D}/ka10_ai.c
            ${KA10D}/ka10_iii.c
            ${KA10D}/kx10_disk.c
            ${KA10D}/ka10_pclk.c
            ${KA10D}/ka10_tv.c
            ${DISPLAY340}
            ${DISPLAYIII}
        INCLUDES
            ${KA10D}
        DEFINES
            KA=1
        INT64
        VIDEO
        LABEL PDP10
        TEST ka10
        SOURCE_DIR ${PDP10D})
endif (BUILD_WITH_VIDEO)

if (PANDA_LIGHTS)
  target_sources(pdp10-ka PUBLIC ${PDP10D}/ka10_lights.c)
  target_compile_definitions(pdp10-ka PUBLIC PANDA_LIGHTS)
  target_link_libraries(pdp10-ka PUBLIC usb-1.0)
endif (PANDA_LIGHTS)

if (BUILD_WITH_VIDEO)
    add_simulator(pdp10-ki
        SOURCES
            ${KI10D}/kx10_cpu.c
            ${KI10D}/kx10_sys.c
            ${KI10D}/kx10_df.c
            ${KI10D}/kx10_dp.c
            ${KI10D}/kx10_mt.c
            ${KI10D}/kx10_cty.c
            ${KI10D}/kx10_lp.c
            ${KI10D}/kx10_pt.c
            ${KI10D}/kx10_dc.c
            ${KI10D}/kx10_rh.c
            ${KI10D}/kx10_rp.c
            ${KI10D}/kx10_rc.c
            ${KI10D}/kx10_dt.c
            ${KI10D}/kx10_dk.c
            ${KI10D}/kx10_cr.c
            ${KI10D}/kx10_cp.c
            ${KI10D}/kx10_tu.c
            ${KI10D}/kx10_rs.c
            ${KI10D}/kx10_imp.c
            ${KI10D}/kx10_dpy.c
            ${KI10D}/kx10_disk.c
            ${DISPLAY340}
        INCLUDES
            ${KI10D}
        DEFINES
            KI=1
        INT64
        VIDEO
        LABEL PDP10
        TEST ki10
        SOURCE_DIR ${PDP10D})
endif (BUILD_WITH_VIDEO)

add_simulator(pdp10-kl
    SOURCES
        ${KL10D}/kx10_cpu.c
        ${KL10D}/kx10_sys.c
        ${KL10D}/kx10_df.c
        ${KA10D}/kx10_dp.c
        ${KA10D}/kx10_mt.c
        ${KA10D}/kx10_lp.c
        ${KA10D}/kx10_pt.c
        ${KA10D}/kx10_dc.c
        ${KL10D}/kx10_rh.c
        ${KA10D}/kx10_dt.c
        ${KA10D}/kx10_cr.c
        ${KA10D}/kx10_cp.c
        ${KL10D}/kx10_rp.c
        ${KL10D}/kx10_tu.c
        ${KL10D}/kx10_rs.c
        ${KL10D}/kx10_imp.c
        ${KL10D}/kl10_fe.c
        ${KL10D}/ka10_pd.c
        ${KL10D}/ka10_ch10.c
        ${KL10D}/kl10_nia.c
        ${KL10D}/kx10_disk.c
    INCLUDES
        ${KL10D}
    DEFINES
        KL=1
    INT64
    LABEL PDP10
    TEST kl10
    SOURCE_DIR ${PDP10D})

add_simulator(pdp10-ks
    SOURCES
        ${KS10D}/kx10_cpu.c
        ${KS10D}/kx10_sys.c
        ${KS10D}/kx10_disk.c
        ${KS10D}/ks10_cty.c
        ${KS10D}/ks10_uba.c
        ${KS10D}/kx10_rh.c
        ${KS10D}/kx10_rp.c
        ${KS10D}/kx10_tu.c
        ${KS10D}/ks10_dz.c
        ${KS10D}/ks10_tcu.c
        ${KS10D}/ks10_lp.c
        ${KS10D}/ks10_ch11.c
        ${KS10D}/ks10_kmc.c
        ${KS10D}/ks10_dup.c
        ${KS10D}/kx10_imp.c
    INCLUDES
        ${KS10D}
        ${PDP11D}
    DEFINES
        KS=1
    INT64
    LABEL PDP10
    TEST ks10
    SOURCE_DIR ${PDP10D})

if (BUILD_WITH_VIDEO)
    add_simulator(pdp6
        SOURCES
            ${PDP6D}/kx10_cpu.c
            ${PDP6D}/kx10_sys.c
            ${PDP6D}/kx10_cty.c
            ${PDP6D}/kx10_lp.c
            ${PDP6D}/kx10_pt.c
            ${PDP6D}/kx10_cr.c
            ${PDP6D}/kx10_cp.c
            ${PDP6D}/pdp6_dct.c
            ${PDP6D}/pdp6_dtc.c
            ${PDP6D}/pdp6_mtc.c
            ${PDP6D}/pdp6_dsk.c
            ${PDP6D}/pdp6_dcs.c
            ${PDP6D}/kx10_dpy.c
            ${PDP6D}/pdp6_slave.c
            ${DISPLAY340}
        INCLUDES
            ${PDP6D}
        DEFINES
            PDP6=1
        INT64
        VIDEO
        LABEL PDP10
        TEST pdp6
        SOURCE_DIR ${PDP10D})
endif (BUILD_WITH_VIDEO)

## ----------------------------------------

if (BUILD_WITH_VIDEO)
    add_simulator(pdp11
        SOURCES
            ${PDP11D}/pdp11_fp.c
            ${PDP11D}/pdp11_cpu.c
            ${PDP11D}/pdp11_dz.c
            ${PDP11D}/pdp11_cis.c
            ${PDP11D}/pdp11_lp.c
            ${PDP11D}/pdp11_rk.c
            ${PDP11D}/pdp11_rl.c
            ${PDP11D}/pdp11_rp.c
            ${PDP11D}/pdp11_rx.c
            ${PDP11D}/pdp11_stddev.c
            ${PDP11D}/pdp11_sys.c
            ${PDP11D}/pdp11_tc.c
            ${PDP11D}/pdp11_tm.c
            ${PDP11D}/pdp11_ts.c
            ${PDP11D}/pdp11_io.c
            ${PDP11D}/pdp11_rq.c
            ${PDP11D}/pdp11_tq.c
            ${PDP11D}/pdp11_pclk.c
            ${PDP11D}/pdp11_ry.c
            ${PDP11D}/pdp11_pt.c
            ${PDP11D}/pdp11_hk.c
            ${PDP11D}/pdp11_xq.c
            ${PDP11D}/pdp11_xu.c
            ${PDP11D}/pdp11_vh.c
            ${PDP11D}/pdp11_rh.c
            ${PDP11D}/pdp11_tu.c
            ${PDP11D}/pdp11_cpumod.c
            ${PDP11D}/pdp11_cr.c
            ${PDP11D}/pdp11_rf.c
            ${PDP11D}/pdp11_dl.c
            ${PDP11D}/pdp11_ta.c
            ${PDP11D}/pdp11_rc.c
            ${PDP11D}/pdp11_kg.c
            ${PDP11D}/pdp11_ke.c
            ${PDP11D}/pdp11_dc.c
            ${PDP11D}/pdp11_dmc.c
            ${PDP11D}/pdp11_kmc.c
            ${PDP11D}/pdp11_dup.c
            ${PDP11D}/pdp11_rs.c
            ${PDP11D}/pdp11_vt.c
            ${PDP11D}/pdp11_td.c
            ${PDP11D}/pdp11_io_lib.c
            ${PDP11D}/pdp11_rom.c
            ${PDP11D}/pdp11_ch.c
            ${DISPLAYVT}
            ${PDP11D}/pdp11_ng.c
            ${PDP11D}/pdp11_daz.c
            ${DISPLAYNG}
        INCLUDES
            ${PDP11D}
        DEFINES
            VM_PDP11
        VIDEO
        LABEL PDP11
        TEST pdp11
        SOURCE_DIR ${PDP11D})
endif (BUILD_WITH_VIDEO)

add_simulator(uc15
    SOURCES
        ${UC15D}/pdp11_cis.c
        ${UC15D}/pdp11_cpu.c
        ${UC15D}/pdp11_cpumod.c
        ${UC15D}/pdp11_cr.c
        ${UC15D}/pdp11_fp.c
        ${UC15D}/pdp11_io.c
        ${UC15D}/pdp11_io_lib.c
        ${UC15D}/pdp11_lp.c
        ${UC15D}/pdp11_rh.c
        ${UC15D}/pdp11_rk.c
        ${UC15D}/pdp11_stddev.c
        ${UC15D}/pdp11_sys.c
        ${UC15D}/pdp11_uc15.c
    INCLUDES
        ${UC15D}
        ${PDP18BD}
    DEFINES
        VM_PDP11
        UC15
    LABEL PDP11
    TEST uc15
    SOURCE_DIR ${PDP11D})

## ----------------------------------------


set(PDP18B
    ${PDP18BD}/pdp18b_dt.c
    ${PDP18BD}/pdp18b_drm.c
    ${PDP18BD}/pdp18b_cpu.c
    ${PDP18BD}/pdp18b_lp.c
    ${PDP18BD}/pdp18b_mt.c
    ${PDP18BD}/pdp18b_rf.c
    ${PDP18BD}/pdp18b_rp.c
    ${PDP18BD}/pdp18b_stddev.c
    ${PDP18BD}/pdp18b_sys.c
    ${PDP18BD}/pdp18b_rb.c
    ${PDP18BD}/pdp18b_tt1.c
    ${PDP18BD}/pdp18b_fpp.c
    ${PDP18BD}/pdp18b_g2tty.c
    ${PDP18BD}/pdp18b_dr15.c)


add_simulator(pdp15
    SOURCES
        ${PDP18B}
    INCLUDES
        ${PDP18BD}
    DEFINES
        PDP15
    LABEL PDP18B
    TEST pdp15
    SOURCE_DIR ${PDP18BD})

add_simulator(pdp4
    SOURCES
        ${PDP18B}
    INCLUDES
        ${PDP18BD}
    DEFINES
        PDP4
    LABEL PDP18B
    TEST pdp4
    SOURCE_DIR ${PDP18BD})

if (BUILD_WITH_VIDEO)
    add_simulator(pdp7
        SOURCES
            ${PDP18B}
            ${PDP18BD}/pdp18b_dpy.c
            ${DISPLAY340}
        INCLUDES
            ${PDP18BD}
        DEFINES
            PDP7
            DISPLAY_TYPE=DIS_TYPE30
            PIX_SCALE=RES_HALF
        VIDEO
        LABEL PDP18B
        TEST pdp7
        SOURCE_DIR ${PDP18BD})
endif (BUILD_WITH_VIDEO)

add_simulator(pdp9
    SOURCES
        ${PDP18B}
    INCLUDES
        ${PDP18BD}
    DEFINES
        PDP9
    LABEL PDP18B
    TEST pdp9
    SOURCE_DIR ${PDP18BD})

## ----------------------------------------

add_simulator(pdp8
    SOURCES
        ${PDP8D}/pdp8_cpu.c
        ${PDP8D}/pdp8_clk.c
        ${PDP8D}/pdp8_df.c
        ${PDP8D}/pdp8_dt.c
        ${PDP8D}/pdp8_lp.c
        ${PDP8D}/pdp8_mt.c
        ${PDP8D}/pdp8_pt.c
        ${PDP8D}/pdp8_rf.c
        ${PDP8D}/pdp8_rk.c
        ${PDP8D}/pdp8_rx.c
        ${PDP8D}/pdp8_sys.c
        ${PDP8D}/pdp8_tt.c
        ${PDP8D}/pdp8_ttx.c
        ${PDP8D}/pdp8_rl.c
        ${PDP8D}/pdp8_tsc.c
        ${PDP8D}/pdp8_td.c
        ${PDP8D}/pdp8_ct.c
        ${PDP8D}/pdp8_fpp.c
    INCLUDES
        ${PDP8D}
    LABEL PDP8
    TEST pdp8
    SOURCE_DIR ${PDP8D})

## ----------------------------------------

add_simulator(s3
    SOURCES
        ${S3D}/s3_cd.c
        ${S3D}/s3_cpu.c
        ${S3D}/s3_disk.c
        ${S3D}/s3_lp.c
        ${S3D}/s3_pkb.c
        ${S3D}/s3_sys.c
    INCLUDES
        ${S3D}
    LABEL S3
    TEST s3
    SOURCE_DIR ${S3D})

## ----------------------------------------

add_simulator(sds
    SOURCES
        ${SDSD}/sds_cpu.c
        ${SDSD}/sds_drm.c
        ${SDSD}/sds_dsk.c
        ${SDSD}/sds_io.c
        ${SDSD}/sds_lp.c
        ${SDSD}/sds_mt.c
        ${SDSD}/sds_mux.c
        ${SDSD}/sds_rad.c
        ${SDSD}/sds_stddev.c
        ${SDSD}/sds_sys.c
        ${SDSD}/sds_cp.c
        ${SDSD}/sds_cr.c
    INCLUDES
        ${SDSD}
    LABEL SDS
    TEST sds
    SOURCE_DIR ${SDSD})

## ----------------------------------------

add_simulator(ssem
    SOURCES
        ${SSEMD}/ssem_cpu.c
        ${SSEMD}/ssem_sys.c
    INCLUDES
        ${SSEMD}
    LABEL SSEM
    TEST ssem
    SOURCE_DIR ${SSEMD})

## ----------------------------------------

if (BUILD_WITH_VIDEO)
    add_simulator(tx-0
        SOURCES
            ${TX0D}/tx0_cpu.c
            ${TX0D}/tx0_dpy.c
            ${TX0D}/tx0_stddev.c
            ${TX0D}/tx0_sys.c
            ${TX0D}/tx0_sys_orig.c
        INCLUDES
            ${TX0D}
        VIDEO
        LABEL TX-0
        TEST tx-0
        SOURCE_DIR ${TX0D})
endif (BUILD_WITH_VIDEO)

## ----------------------------------------


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
        ${VAXD}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_420
        VAX_411
    FULL64
    ${INFOSERVER100_VIDEO}
    LABEL VAX
    TEST vax-diag
    SOURCE_DIR ${VAXD})

add_simulator(infoserver1000
    SOURCES
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
        ${VAXD}/vax_xs.c
        ${VAXD}/vax4xx_rz94.c
        ${VAXD}/vax4nn_stddev.c
        ${VAXD}/is1000_sysdev.c
        ${VAXD}/is1000_syslist.c
    INCLUDES
        ${VAXD}
    DEFINES
        VM_VAX
        IS_1000
    FULL64
    LABEL VAX
    TEST vax-diag
    SOURCE_DIR ${VAXD})

if (BUILD_WITH_VIDEO)
    set(INFOSERVER150VXT_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(INFOSERVER150VXT_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(infoserver150vxt
    SOURCES
        ${VAX420}
    INCLUDES
        ${VAXD}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_420
        VAX_412
    FULL64
    ${INFOSERVER150VXT_VIDEO}
    LABEL VAX
    TEST vax-diag
    SOURCE_DIR ${VAXD})

if (BUILD_WITH_VIDEO)
    set(MICROVAX1_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(MICROVAX1_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(microvax1
    SOURCES
        ${VAXD}/vax_cpu.c
        ${VAXD}/vax_cpu1.c
        ${VAXD}/vax_fpa.c
        ${VAXD}/vax_cis.c
        ${VAXD}/vax_octa.c
        ${VAXD}/vax_cmode.c
        ${VAXD}/vax_mmu.c
        ${VAXD}/vax_sys.c
        ${VAXD}/vax_syscm.c
        ${VAXD}/vax610_stddev.c
        ${VAXD}/vax610_sysdev.c
        ${VAXD}/vax610_io.c
        ${VAXD}/vax610_syslist.c
        ${VAXD}/vax610_mem.c
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
    INCLUDES
        ${VAXD}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_610
    FULL64
    ${MICROVAX1_VIDEO}
    LABEL VAX
    TEST vax-diag
    SOURCE_DIR ${VAXD})

if (BUILD_WITH_VIDEO)
    set(MICROVAX2_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(MICROVAX2_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(microvax2
    SOURCES
        ${VAX630}
    INCLUDES
        ${VAXD}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_630
    FULL64
    ${MICROVAX2_VIDEO}
    LABEL VAX
    TEST vax-diag
    SOURCE_DIR ${VAXD})

if (BUILD_WITH_VIDEO)
    set(MICROVAX2000_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(MICROVAX2000_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(microvax2000
    SOURCES
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
        ${VAXD}/vax410_sysdev.c
        ${VAXD}/vax410_syslist.c
        ${VAXD}/vax4xx_dz.c
        ${VAXD}/vax4xx_rd.c
        ${VAXD}/vax4xx_rz80.c
        ${VAXD}/vax_xs.c
        ${VAXD}/vax4xx_va.c
        ${VAXD}/vax4xx_vc.c
        ${VAXD}/vax_lk.c
        ${VAXD}/vax_vs.c
        ${VAXD}/vax_gpx.c
    INCLUDES
        ${VAXD}
    DEFINES
        VM_VAX
        VAX_410
    FULL64
    ${MICROVAX2000_VIDEO}
    LABEL VAX
    TEST vax-diag
    SOURCE_DIR ${VAXD})

if (BUILD_WITH_VIDEO)
    set(MICROVAX3100_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(MICROVAX3100_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(microvax3100
    SOURCES
        ${VAX420}
    INCLUDES
        ${VAXD}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_420
        VAX_41A
    FULL64
    ${MICROVAX3100_VIDEO}
    LABEL VAX
    TEST vax-diag
    SOURCE_DIR ${VAXD})

if (BUILD_WITH_VIDEO)
    set(MICROVAX3100E_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(MICROVAX3100E_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(microvax3100e
    SOURCES
        ${VAX420}
    INCLUDES
        ${VAXD}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_420
        VAX_41D
    FULL64
    ${MICROVAX3100E_VIDEO}
    LABEL VAX
    TEST vax-diag
    SOURCE_DIR ${VAXD})

add_simulator(microvax3100m80
    SOURCES
        ${VAX440}
    INCLUDES
        ${VAXD}
    DEFINES
        VM_VAX
        VAX_440
        VAX_47
    FULL64
    LABEL VAX
    TEST vax-diag
    SOURCE_DIR ${VAXD})

add_simulator(rtvax1000
    SOURCES
        ${VAX630}
    INCLUDES
        ${VAXD}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_620
    FULL64
    LABEL VAX
    TEST vax-diag
    SOURCE_DIR ${VAXD})

if (BUILD_WITH_VIDEO)
    set(VAX_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(VAX_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(vax
    SOURCES
        ${VAX}
    INCLUDES
        ${VAXD}
        ${PDP11D}
    DEFINES
        VM_VAX
    FULL64
    ${VAX_VIDEO}
    LABEL VAX
    TEST vax-diag
    SOURCE_DIR ${VAXD})

set(vax_symlink_dir_src ${CMAKE_CURRENT_BINARY_DIR})
if (CMAKE_CONFIGURATION_TYPES)
    string(APPEND vax_symlink_dir_src "/$<CONFIG>")
endif (CMAKE_CONFIGURATION_TYPES)
add_custom_command(TARGET vax POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_if_different vax${CMAKE_EXECUTABLE_SUFFIX} microvax3900${CMAKE_EXECUTABLE_SUFFIX} COMMENT "Copy vax${CMAKE_EXECUTABLE_SUFFIX} to microvax3900${CMAKE_EXECUTABLE_SUFFIX}" WORKING_DIRECTORY ${vax_symlink_dir_src})
install(FILES ${vax_symlink_dir_src}/microvax3900${CMAKE_EXECUTABLE_SUFFIX}
        DESTINATION ${CMAKE_INSTALL_BINDIR})

add_simulator(vax730
    SOURCES
        ${VAXD}/vax_cpu.c
        ${VAXD}/vax_cpu1.c
        ${VAXD}/vax_fpa.c
        ${VAXD}/vax_cis.c
        ${VAXD}/vax_octa.c
        ${VAXD}/vax_cmode.c
        ${VAXD}/vax_mmu.c
        ${VAXD}/vax_sys.c
        ${VAXD}/vax_syscm.c
        ${VAXD}/vax730_stddev.c
        ${VAXD}/vax730_sys.c
        ${VAXD}/vax730_mem.c
        ${VAXD}/vax730_uba.c
        ${VAXD}/vax730_rb.c
        ${VAXD}/vax730_syslist.c
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
    TEST vax-diag
    SOURCE_DIR ${VAXD})

add_simulator(vax750
    SOURCES
        ${VAXD}/vax_cpu.c
        ${VAXD}/vax_cpu1.c
        ${VAXD}/vax_fpa.c
        ${VAXD}/vax_cis.c
        ${VAXD}/vax_octa.c
        ${VAXD}/vax_cmode.c
        ${VAXD}/vax_mmu.c
        ${VAXD}/vax_sys.c
        ${VAXD}/vax_syscm.c
        ${VAXD}/vax750_stddev.c
        ${VAXD}/vax750_cmi.c
        ${VAXD}/vax750_mem.c
        ${VAXD}/vax750_uba.c
        ${VAXD}/vax7x0_mba.c
        ${VAXD}/vax750_syslist.c
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
    TEST vax-diag
    SOURCE_DIR ${VAXD})

add_simulator(vax780
    SOURCES
        ${VAXD}/vax_cpu.c
        ${VAXD}/vax_cpu1.c
        ${VAXD}/vax_fpa.c
        ${VAXD}/vax_cis.c
        ${VAXD}/vax_octa.c
        ${VAXD}/vax_cmode.c
        ${VAXD}/vax_mmu.c
        ${VAXD}/vax_sys.c
        ${VAXD}/vax_syscm.c
        ${VAXD}/vax780_stddev.c
        ${VAXD}/vax780_sbi.c
        ${VAXD}/vax780_mem.c
        ${VAXD}/vax780_uba.c
        ${VAXD}/vax7x0_mba.c
        ${VAXD}/vax780_fload.c
        ${VAXD}/vax780_syslist.c
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
    TEST vax-diag
    SOURCE_DIR ${VAXD})

add_simulator(vax8200
    SOURCES
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
        ${VAXD}/vax820_stddev.c
        ${VAXD}/vax820_bi.c
        ${VAXD}/vax820_mem.c
        ${VAXD}/vax820_uba.c
        ${VAXD}/vax820_ka.c
        ${VAXD}/vax820_syslist.c
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
    TEST vax-diag
    SOURCE_DIR ${VAXD})

add_simulator(vax8600
    SOURCES
        ${VAXD}/vax_cpu.c
        ${VAXD}/vax_cpu1.c
        ${VAXD}/vax_fpa.c
        ${VAXD}/vax_cis.c
        ${VAXD}/vax_octa.c
        ${VAXD}/vax_cmode.c
        ${VAXD}/vax_mmu.c
        ${VAXD}/vax_sys.c
        ${VAXD}/vax_syscm.c
        ${VAXD}/vax860_stddev.c
        ${VAXD}/vax860_sbia.c
        ${VAXD}/vax860_abus.c
        ${VAXD}/vax780_uba.c
        ${VAXD}/vax7x0_mba.c
        ${VAXD}/vax860_syslist.c
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
    TEST vax-diag
    SOURCE_DIR ${VAXD})

if (BUILD_WITH_VIDEO)
    set(VAXSTATION3100M30_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(VAXSTATION3100M30_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(vaxstation3100m30
    SOURCES
        ${VAX420}
    INCLUDES
        ${VAXD}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_420
        VAX_42A
    FULL64
    ${VAXSTATION3100M30_VIDEO}
    LABEL VAX
    TEST vax-diag
    SOURCE_DIR ${VAXD})

if (BUILD_WITH_VIDEO)
    set(VAXSTATION3100M38_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(VAXSTATION3100M38_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(vaxstation3100m38
    SOURCES
        ${VAX420}
    INCLUDES
        ${VAXD}
        ${PDP11D}
    DEFINES
        VM_VAX
        VAX_420
        VAX_42B
    FULL64
    ${VAXSTATION3100M38_VIDEO}
    LABEL VAX
    TEST vax-diag
    SOURCE_DIR ${VAXD})

if (BUILD_WITH_VIDEO)
    set(VAXSTATION3100M76_VIDEO "VIDEO")
else (BUILD_WITH_VIDEO)
    set(VAXSTATION3100M76_VIDEO "")
endif (BUILD_WITH_VIDEO)

add_simulator(vaxstation3100m76
    SOURCES
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
        ${VAXD}/vax43_sysdev.c
        ${VAXD}/vax43_syslist.c
        ${VAXD}/vax4xx_dz.c
        ${VAXD}/vax4xx_rz80.c
        ${VAXD}/vax_xs.c
        ${VAXD}/vax4xx_vc.c
        ${VAXD}/vax4xx_ve.c
        ${VAXD}/vax_lk.c
        ${VAXD}/vax_vs.c
    INCLUDES
        ${VAXD}
    DEFINES
        VM_VAX
        VAX_43
    FULL64
    ${VAXSTATION3100M76_VIDEO}
    LABEL VAX
    TEST vax-diag
    SOURCE_DIR ${VAXD})

add_simulator(vaxstation4000m60
    SOURCES
        ${VAX440}
    INCLUDES
        ${VAXD}
    DEFINES
        VM_VAX
        VAX_440
        VAX_46
    FULL64
    LABEL VAX
    TEST vax-diag
    SOURCE_DIR ${VAXD})

add_simulator(vaxstation4000vlc
    SOURCES
        ${VAX440}
    INCLUDES
        ${VAXD}
    DEFINES
        VM_VAX
        VAX_440
        VAX_48
    FULL64
    LABEL VAX
    TEST vax-diag
    SOURCE_DIR ${VAXD})

## ----------------------------------------

if (BUILD_WITH_VIDEO)
    add_simulator(imlac
        SOURCES
            ${IMLACD}/imlac_sys.c
            ${IMLACD}/imlac_cpu.c
            ${IMLACD}/imlac_dp.c
            ${IMLACD}/imlac_crt.c
            ${IMLACD}/imlac_kbd.c
            ${IMLACD}/imlac_tty.c
            ${IMLACD}/imlac_pt.c
            ${IMLACD}/imlac_bel.c
        INCLUDES
            ${IMLACD}
        VIDEO
        LABEL imlac
        TEST imlac
        SOURCE_DIR ${IMLACD})
endif (BUILD_WITH_VIDEO)

## ----------------------------------------

add_simulator(sigma
    SOURCES
        ${SIGMAD}/sigma_cpu.c
        ${SIGMAD}/sigma_sys.c
        ${SIGMAD}/sigma_cis.c
        ${SIGMAD}/sigma_coc.c
        ${SIGMAD}/sigma_dk.c
        ${SIGMAD}/sigma_dp.c
        ${SIGMAD}/sigma_fp.c
        ${SIGMAD}/sigma_io.c
        ${SIGMAD}/sigma_lp.c
        ${SIGMAD}/sigma_map.c
        ${SIGMAD}/sigma_mt.c
        ${SIGMAD}/sigma_pt.c
        ${SIGMAD}/sigma_rad.c
        ${SIGMAD}/sigma_rtc.c
        ${SIGMAD}/sigma_tt.c
    INCLUDES
        ${SIGMAD}
    LABEL sigma
    TEST sigma
    SOURCE_DIR ${SIGMAD})

## ----------------------------------------

if (BUILD_WITH_VIDEO)
    add_simulator(tt2500
        SOURCES
            ${TT2500D}/tt2500_sys.c
            ${TT2500D}/tt2500_cpu.c
            ${TT2500D}/tt2500_dpy.c
            ${TT2500D}/tt2500_crt.c
            ${TT2500D}/tt2500_tv.c
            ${TT2500D}/tt2500_key.c
            ${TT2500D}/tt2500_uart.c
            ${TT2500D}/tt2500_rom.c
        INCLUDES
            ${TT2500D}
        VIDEO
        LABEL tt2500
        TEST tt2500
        SOURCE_DIR ${TT2500D})
endif (BUILD_WITH_VIDEO)
