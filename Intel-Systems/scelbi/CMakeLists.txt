## Intel-Systems/scelbi simulator
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

add_simulator(scelbi
    SOURCES
        ${SCELBIC}/i8008.c
        scelbi_sys.c
        scelbi_io.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LABEL Intel-Systems/scelbi
    TEST scelbi)
