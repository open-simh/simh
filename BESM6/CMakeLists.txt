## BESM6 simulator
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

set(besm6_font)
set(cand_fonts
      "DejaVuSans.ttf"
      "LucidaSansRegular.ttf"
      "FreeSans.ttf"
      "AppleGothic.ttf"
      "tahoma.ttf")
set(cand_fontdirs
      "/usr/share/fonts"
      "/usr/lib/jvm"
      "/Library/Fonts"
      "/System/Library/Fonts"
      "/System/Library/Frameworks/JavaVM.framework/Versions"
      "$ENV{WINDIR}/Fonts")

foreach (fdir ${cand_fontdirs})
    foreach (font ${cand_fonts})
        if (EXISTS ${fdir}/${font})
            get_filename_component(fontfile ${fdir}/${font} ABSOLUTE)
            list(APPEND besm6_font ${fontfile})
        endif ()

        file(GLOB besm6_font_cand_1 LIST_DIRECTORIES FALSE "${fdir}/*/${font}")
        file(GLOB besm6_font_cand_2 LIST_DIRECTORIES FALSE "${fdir}/*/*/${font}")
        file(GLOB besm6_font_cand_3 LIST_DIRECTORIES FALSE "${fdir}/*/*/*/${font}")
        list(APPEND besm6_font ${besm6_font_cand_1} ${besm6_font_cand_2} ${besm6_font_cand_3})
    endforeach()
endforeach()

if (besm6_font)
    set(besm6_found_fonts "BESM6: Fonts found")
    foreach(bfont ${besm6_font})
        string(APPEND besm6_found_fonts "
      .. ${bfont}")
    endforeach ()
    message(STATUS ${besm6_found_fonts})
    unset(besm6_found_fonts)
    list(GET besm6_font 0 besm6_font)
    message(STATUS "BESM6: Using ${besm6_font}")
else ()
    set(besm6_no_fonts "BESM6: No applicable Cyrillic fonts found.")
    string(APPEND besm6_no_fonts "
    Font names tried:")
    foreach (font ${cand_fonts})
        string(APPEND besm6_no_fonts "
      ..  ${font}")
    endforeach ()
    string(APPEND besm6_no_fonts "

    Looked in:")
    foreach (fdir ${cand_fontdirs})
        string(APPEND besm6_no_fonts "
      ..  ${fdir}")
    endforeach()
    string(APPEND besm6_no_fonts "

BESM6: Not building with panel display.")
    message(STATUS ${besm6_no_fonts})
    unset(besm6_no_fonts)
endif ()

if (NOT (besm6_font AND WITH_VIDEO))
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
        FEATURE_INT64
        LABEL BESM6
        PKG_FAMILY default_family
        TEST besm6)
else ()
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
        FEATURE_INT64
        FEATURE_VIDEO
        FEATURE_DISPLAY
        BESM6_SDL_HACK
        LABEL BESM6
        PKG_FAMILY default_family
        TEST besm6)
endif()
unset(cand_fonts)
unset(cand_fontdirs)
