## LGP simulator
##
## This is an automagically generated file. Do NOT EDIT.
## Any changes you make will be overwritten!!
##
## Make changes to the SIMH top-level makefile and then run the
## "cmake/generate.py" script to regenerate these files.
##
##     python -m cmake/generate --help
##
## ------------------------------------------------------------

if (HAVE_UNITY_FRAMEWORK AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/unit-tests/CMakeLists.txt")
  add_subdirectory(unit-tests)
endif ()

add_simulator(lgp
    SOURCES
        lgp_cpu.c
        lgp_stddev.c
        lgp_sys.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LABEL LGP
    TEST lgp)
