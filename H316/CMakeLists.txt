## H316 simulator
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

add_simulator(h316
    SOURCES
        h316_stddev.c
        h316_lp.c
        h316_cpu.c
        h316_sys.c
        h316_mt.c
        h316_fhd.c
        h316_dp.c
        h316_rtc.c
        h316_imp.c
        h316_hi.c
        h316_mi.c
        h316_udp.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        VM_IMPTIP
    LABEL H316
    PKG_FAMILY honeywell_family
    TEST h316)
