## BESM6 simulator
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

set(besm6_font)
foreach (fdir IN ITEMS
           "/usr/share/fonts" "/Library/Fonts" "/usr/lib/jvm"
           "/System/Library/Frameworks/JavaVM.framework/Versions"
           "$ENV{WINDIR}/Fonts")
    foreach (font IN ITEMS
                "DejaVuSans.ttf" "LucidaSansRegular.ttf"
                "FreeSans.ttf" "AppleGothic.ttf" "tahoma.ttf")
        if (EXISTS ${fdir}/${font})
            get_filename_component(fontfile ${fdir}/${font} ABSOLUTE)
            list(APPEND besm6_font ${fontfile})
        endif ()
    endforeach()
endforeach()

if (besm6_font)
    list(LENGTH besm6_font besm6_font_len)
    if (besm6_font_len GREATER 1)
        message(STATUS "BESM6: Fonts found ${besm6_font}")
    endif ()
    list(GET besm6_font 0 besm6_font)
    message(STATUS "BESM6: Using ${besm6_font}")
endif (besm6_font)

if (besm6_font)
    if (BUILD_WITH_VIDEO)
        add_simulator(besm6
            SOURCES
                besm6_cpu.c
                besm6_sys.c
                besm6_mmu.c
                besm6_arith.c
                besm6_disk.c
                besm6_drum.c
                besm6_tty.c
                besm6_panel.c
                besm6_printer.c
                besm6_pl.c
                besm6_mg.c
                besm6_punch.c
                besm6_punchcard.c
                besm6_vu.c
            INCLUDES
                ${CMAKE_CURRENT_SOURCE_DIR}
            DEFINES
                FONTFILE=${besm6_font}
            INT64
            VIDEO
            LABEL BESM6
            TEST besm6)
    endif (BUILD_WITH_VIDEO)
endif ()
