## PDP18B simulators
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


set(PDP18B
    ${PDP18BD}/pdp18b_dt.c
    ${PDP18BD}/pdp18b_drm.c
    ${PDP18BD}/pdp18b_cpu.c
    ${PDP18BD}/pdp18b_lp.c
    ${PDP18BD}/pdp18b_mt.c
    ${PDP18BD}/pdp18b_rf.c
    ${PDP18BD}/pdp18b_rp.c
    ${PDP18BD}/pdp18b_stddev.c
    ${PDP18BD}/pdp18b_sys.c
    ${PDP18BD}/pdp18b_rb.c
    ${PDP18BD}/pdp18b_tt1.c
    ${PDP18BD}/pdp18b_fpp.c
    ${PDP18BD}/pdp18b_g2tty.c
    ${PDP18BD}/pdp18b_dr15.c)


add_simulator(pdp15
    SOURCES
        ${PDP18B}
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        PDP15
    LABEL PDP18B
    TEST pdp15)

add_simulator(pdp4
    SOURCES
        ${PDP18B}
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        PDP4
    LABEL PDP18B
    TEST pdp4)

if (BUILD_WITH_VIDEO)
    add_simulator(pdp7
        SOURCES
            ${PDP18B}
            pdp18b_dpy.c
            ${DISPLAY340}
        INCLUDES
            ${CMAKE_CURRENT_SOURCE_DIR}
        DEFINES
            PDP7
            DISPLAY_TYPE=DIS_TYPE30
            PIX_SCALE=RES_HALF
        VIDEO
        LABEL PDP18B
        TEST pdp7)
endif (BUILD_WITH_VIDEO)

add_simulator(pdp9
    SOURCES
        ${PDP18B}
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        PDP9
    LABEL PDP18B
    TEST pdp9)
