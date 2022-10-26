## PDQ-3 simulator
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

add_simulator(pdq3
    SOURCES
        pdq3_cpu.c
        pdq3_sys.c
        pdq3_stddev.c
        pdq3_mem.c
        pdq3_debug.c
        pdq3_fdc.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LABEL PDQ-3
    PKG_FAMILY experimental
    TEST pdq3)
