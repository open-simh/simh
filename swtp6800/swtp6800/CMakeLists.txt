## swtp6800/swtp6800 simulators
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

add_simulator(swtp6800mp-a
    SOURCES
        ${SWTP6800C}/mp-a.c
        ${SWTP6800C}/m6800.c
        ${SWTP6800C}/m6810.c
        ${SWTP6800C}/bootrom.c
        ${SWTP6800C}/dc-4.c
        mp-a_sys.c
        ${SWTP6800C}/mp-8m.c
        ${SWTP6800C}/fd400.c
        ${SWTP6800C}/mp-b2.c
        ${SWTP6800C}/mp-s.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    BUILDROMS
    LABEL swtp6800
    PKG_FAMILY swtp_family
    TEST swtp6800mp-a)

add_simulator(swtp6800mp-a2
    SOURCES
        ${SWTP6800C}/mp-a2.c
        ${SWTP6800C}/m6800.c
        ${SWTP6800C}/m6810.c
        ${SWTP6800C}/bootrom.c
        ${SWTP6800C}/dc-4.c
        mp-a2_sys.c
        ${SWTP6800C}/mp-8m.c
        ${SWTP6800C}/i2716.c
        ${SWTP6800C}/fd400.c
        ${SWTP6800C}/mp-s.c
        ${SWTP6800C}/mp-b2.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    BUILDROMS
    LABEL swtp6800
    PKG_FAMILY swtp_family
    TEST swtp6800mp-a2)
