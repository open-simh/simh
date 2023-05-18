## alpha simulator
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

add_simulator(alpha
    SOURCES
        alpha_500au_syslist.c
        alpha_cpu.c
        alpha_ev5_cons.c
        alpha_ev5_pal.c
        alpha_ev5_tlb.c
        alpha_fpi.c
        alpha_fpv.c
        alpha_io.c
        alpha_mmu.c
        alpha_sys.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    FEATURE_FULL64
    LABEL alpha
    PKG_FAMILY experimental
    TEST alpha)
