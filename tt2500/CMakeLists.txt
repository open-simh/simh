## tt2500 simulator
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

if (BUILD_WITH_VIDEO)
    add_simulator(tt2500
        SOURCES
            tt2500_sys.c
            tt2500_cpu.c
            tt2500_dpy.c
            tt2500_crt.c
            tt2500_tv.c
            tt2500_key.c
            tt2500_uart.c
            tt2500_rom.c
        INCLUDES
            ${CMAKE_CURRENT_SOURCE_DIR}
        VIDEO
        LABEL tt2500
        TEST tt2500)
endif (BUILD_WITH_VIDEO)
