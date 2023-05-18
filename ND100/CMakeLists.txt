## ND100 simulator
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

add_simulator(nd100
    SOURCES
        nd100_sys.c
        nd100_cpu.c
        nd100_floppy.c
        nd100_stddev.c
        nd100_mm.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LABEL ND100
    PKG_FAMILY norsk_family
    TEST nd100)
