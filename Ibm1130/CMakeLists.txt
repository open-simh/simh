# IBM 1130
if(WIN32)
  set(IBM1130_DEFS GUI_SUPPORT)
  set(IBM1130_LIBS gdi32)
  set(IBM1130_RES ibm1130.rc)
else()
  set(IBM1130_DEFS "")
  set(IBM1130_LIBS "")
  set(IBM1130_RES "")
endif()
add_simulator2(NAME ibm1130
  DEFINITIONS ${IBM1130_DEFS}
  LIBRARIES ${IBM1130_LIBS}
  SOURCES
    ${IBM1130_RES}
    dmsr2v12phases.h
    dmsr2v12slet.h
    ibm1130_conin.h
    ibm1130_conout.h
    ibm1130_defs.h
    ibm1130_prtwheel.h
    ibm1130_cpu.c
    ibm1130_cr.c
    ibm1130_disk.c
    ibm1130_fmt.h
    ibm1130_fmt.c
    ibm1130_gdu.c
    ibm1130_gui.c
    ibm1130_plot.c
    ibm1130_prt.c
    ibm1130_ptrp.c
    ibm1130_sca.c
    ibm1130_stddev.c
    ibm1130_sys.c
    ibm1130_t2741.c
)
