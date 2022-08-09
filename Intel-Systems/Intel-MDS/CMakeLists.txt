## Intel-Systems/Intel-MDS simulator
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

add_simulator(intel-mds
    SOURCES
        ${INTELSYSC}/i8080.c
        imds_sys.c
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
        ${CMAKE_CURRENT_SOURCE_DIR}
    LABEL Intel-Systems/Intel-MDS
    TEST intel-mds)
