## SAGE simulator
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

add_simulator(sage
    SOURCES
        sage_cpu.c
        sage_sys.c
        sage_stddev.c
        sage_cons.c
        sage_fd.c
        sage_lp.c
        m68k_cpu.c
        m68k_mem.c
        m68k_scp.c
        m68k_parse.tab.c
        m68k_sys.c
        i8251.c
        i8253.c
        i8255.c
        i8259.c
        i8272.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        HAVE_INT64
    LABEL SAGE
    PKG_FAMILY experimental
    TEST sage)
