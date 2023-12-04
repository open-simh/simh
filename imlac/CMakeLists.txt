## imlac simulator
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

add_simulator(imlac
    SOURCES
        imlac_sys.c
        imlac_cpu.c
        imlac_dp.c
        imlac_crt.c
        imlac_kbd.c
        imlac_tty.c
        imlac_pt.c
        imlac_bel.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    FEATURE_VIDEO
    FEATURE_DISPLAY
    USES_AIO
    LABEL imlac
    PKG_FAMILY imlac_family
    TEST imlac)
