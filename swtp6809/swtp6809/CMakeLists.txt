## swtp6809/swtp6809 simulator
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

add_simulator(swtp6809mp-09
    SOURCES
        ${SWTP6809C}/mp-09.c
        ${SWTP6809C}/m6809.c
        ${SWTP6809C}/bootrom.c
        ${SWTP6809C}/dc-4.c
        mp-09_sys.c
        ${SWTP6809C}/mp-1m.c
        ${SWTP6809C}/mp-b3.c
        ${SWTP6809C}/mp-s.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    BUILDROMS
    LABEL swtp6809
    PKG_FAMILY swtp_family
    TEST swtp6809mp-09)
