## AltairZ80 simulator
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

add_simulator(altairz80
    SOURCES
        altairz80_cpu.c
        altairz80_cpu_nommu.c
        s100_dj2d.c
        altairz80_dsk.c
        disasm.c
        altairz80_sio.c
        altairz80_sys.c
        altairz80_hdsk.c
        altairz80_net.c
        s100_hayes.c
        s100_2sio.c
        s100_pmmi.c
        flashwriter2.c
        i86_decode.c
        i86_ops.c
        i86_prim_ops.c
        i8272.c
        insnsd.c
        altairz80_mhdsk.c
        mfdc.c
        n8vem.c
        vfdhd.c
        s100_disk1a.c
        s100_disk2.c
        s100_disk3.c
        s100_fif.c
        s100_mdriveh.c
        s100_icom.c
        s100_jadedd.c
        s100_mdsa.c
        s100_mdsad.c
        s100_selchan.c
        s100_ss1.c
        s100_64fdc.c
        s100_scp300f.c
        s100_tarbell.c
        wd179x.c
        s100_hdc1001.c
        s100_if3.c
        s100_adcs6.c
        m68kcpu.c
        m68kdasm.c
        m68kasm.c
        m68kopac.c
        m68kopdm.c
        m68kopnz.c
        m68kops.c
        m68ksim.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LABEL AltairZ80
    TEST altairz80)
