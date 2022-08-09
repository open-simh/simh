## I7000 simulators
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
    TEST i701)

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
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        I7010
    LABEL I7000
    TEST i7010)

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
    TEST i704)

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
    TEST i7070)

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
    TEST i7080)

add_simulator(i7090
    SOURCES
        i7090_cpu.c
        i7090_sys.c
        i7090_chan.c
        i7090_cdr.c
        i7090_cdp.c
        i7090_lpr.c
        i7000_chan.c
        i7000_mt.c
        i7090_drum.c
        i7090_hdrum.c
        i7000_chron.c
        i7000_dsk.c
        i7000_com.c
        i7000_ht.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        I7090
    INT64
    LABEL I7000
    TEST i7090)
