## Put everything together into one nice function.

include (CTest)

set(SIMH_BINDIR "${CMAKE_SOURCE_DIR}/BIN")
if (MSVC)
  set(SIMH_BINDIR "${SIMH_BINDIR}/Win32/$<CONFIG>")
endif ()

## INTERFACE libraries: Interface libraries let us refer to the libraries,
## set includes and defines, flags in a consistent way.
##
## Here's the way it works:
##
## 1. Add the library as INTERFACE. The name isn't particularly important,
##    but it should be meaningful, like "thread_lib".
## 2. If specific features are enabled, add target attributes and properties,
##    such as defines, include directories to the interface library.
## 3. "Link" with the interface library. All of the flags, defines, includes
##    get picked up in the dependent.
##

## Simulator sources and library:
set(SIM_SOURCES
    ${CMAKE_SOURCE_DIR}/scp.c
    ${CMAKE_SOURCE_DIR}/sim_card.c
    ${CMAKE_SOURCE_DIR}/sim_console.c
    ${CMAKE_SOURCE_DIR}/sim_disk.c
    ${CMAKE_SOURCE_DIR}/sim_ether.c
    ${CMAKE_SOURCE_DIR}/sim_fio.c
    ${CMAKE_SOURCE_DIR}/sim_imd.c
    ${CMAKE_SOURCE_DIR}/sim_scsi.c
    ${CMAKE_SOURCE_DIR}/sim_serial.c
    ${CMAKE_SOURCE_DIR}/sim_sock.c
    ${CMAKE_SOURCE_DIR}/sim_tape.c
    ${CMAKE_SOURCE_DIR}/sim_timer.c
    ${CMAKE_SOURCE_DIR}/sim_tmxr.c
    ${CMAKE_SOURCE_DIR}/sim_video.c)

function(build_simcore _targ)
    cmake_parse_arguments(SIMH "VIDEO;INT64;ADDR64" "" "" ${ARGN})

    add_library(${_targ} STATIC ${SIM_SOURCES})

    # Components that need to be turned on while building the library, but
    # don't export out to the dependencies (hence PRIVATE.)
    set_target_properties(${_targ} PROPERTIES C_STANDARD 99)
    target_compile_definitions(${_targ} PRIVATE USE_SIM_CARD USE_SIM_IMD)
    target_compile_options(${_targ} PRIVATE ${EXTRA_CFLAGS})

    if (SIMH_INT64)
        target_compile_definitions(${_targ} PUBLIC USE_INT64)
    endif (SIMH_INT64)

    if (SIMH_ADDR64)
        target_compile_definitions(${_targ} PUBLIC USE_ADDR64)
    endif (SIMH_ADDR64)

    if (WITH_VIDEO AND SIMH_VIDEO)
        target_sources(${_targ} PRIVATE
            ${CMAKE_SOURCE_DIR}/display/display.c
            ${CMAKE_SOURCE_DIR}/display/sim_ws.c)
        target_compile_definitions(${_targ} PUBLIC USE_DISPLAY)
        target_link_libraries(${_targ} PUBLIC simh_video)
    endif (WITH_VIDEO AND SIMH_VIDEO)

    target_link_libraries(${_targ} PUBLIC
        simh_network
        simh_ncurses
        regexp_lib
        zlib_lib
        os_features
        thread_lib
    )

    if (WIN32)
	target_link_libraries(simh_network INTERFACE ws2_32 wsock32)
    endif (WIN32)

    # Define SIM_BUILD_TOOL for the simulator'
    if (NOT (HAVE_GIT_COMMIT_HASH OR HAVE_GIT_COMMIT_TIME))
        target_compile_definitions("${_targ}" PRIVATE
            "SIM_BUILD_TOOL=${CMAKE_GENERATOR} generated by CMake"
            "SIM_GIT_COMMIT_ID=${SIMH_GIT_COMMIT_HASH}"
            "SIM_GIT_COMMIT_TIME=${SIMH_GIT_COMMIT_TIME}"
        )
    endif (NOT (HAVE_GIT_COMMIT_HASH OR HAVE_GIT_COMMIT_TIME))

    # Create target cppcheck rule, if detected.
    if (ENABLE_CPPCHECK AND cppcheck_cmd)
        get_property(cppcheck_includes TARGET ${_targ} PROPERTY INCLUDE_DIRECTORIES)
        get_property(cppcheck_defines  TARGET ${_targ} PROPERTY COMPILE_DEFINITIONS)
        list(TRANSFORM cppcheck_includes PREPEND "-I")
        list(TRANSFORM cppcheck_defines  PREPEND "-D")

        add_custom_target("${_targ}_cppcheck"
            COMMAND ${cppcheck_cmd} ${cppcheck_defines} ${cppcheck_includes} ${SIM_SOURCES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

        add_dependencies(cppcheck "${_targ}_cppcheck")
    endif (ENABLE_CPPCHECK AND cppcheck_cmd)
endfunction(build_simcore _targ)


##+
## Basic template for simh executables and unit tests. Flags:
##
## INT64: Use the simhi64 library (USE_INT64)
## FULL64: Use the simhz64 library (USE_INT64, USE_ADDR64)
## BUILDROMS: Build the hardcoded boot rooms
## VIDEO: Add video support
## DEFINES: List of extra command line manifest constants ("-D" items)
## INCLUDES: List of extra include directories
## SOURCES: List of source files
##-

function (simh_executable_template _targ)
    cmake_parse_arguments(SIMH "INT64;FULL64;BUILDROMS;VIDEO" "TEST;SOURCE_DIR;LABEL" "DEFINES;INCLUDES;SOURCES" ${ARGN})

    if (NOT DEFINED SIMH_SOURCES)
        message(FATAL_ERROR "${_targ}: No source files?")
    endif (NOT DEFINED SIMH_SOURCES)

    add_executable("${_targ}" "${SIMH_SOURCES}")
    set_target_properties(${_targ} PROPERTIES C_STANDARD 99)
    target_compile_options(${_targ} PRIVATE ${EXTRA_CFLAGS})

    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
            target_compile_definitions(${_targ} PUBLIC _LARGEFILE64_SOURCE _FILE_OFFSET_BITS=64)
    elseif (MINGW)
        ## target_compile_options(${_targ} PRIVATE "-fms-extensions")
        target_link_options(${_targ}    PRIVATE "-mconsole")
    elseif (MSVC)
        target_link_options(${_targ} PRIVATE "/SUBSYSTEM:CONSOLE")
    endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    if (DEFINED SIMH_DEFINES)
        target_compile_definitions("${_targ}" PRIVATE "${SIMH_DEFINES}")
    endif (DEFINED SIMH_DEFINES)

    # This is a quick cheat to make sure that all of the include paths are
    # absolute paths.
    if (DEFINED SIMH_INCLUDES)
        set(_normalized_includes)
        foreach (inc IN LISTS SIMH_INCLUDES)
            if (NOT IS_ABSOLUTE "${inc}")
                get_filename_component(inc "${CMAKE_SOURCE_DIR}/${inc}" ABSOLUTE)
            endif ()
            list(APPEND _normalized_includes "${inc}")
        endforeach (inc IN LISTS ${SIMH_INCLUDES})

        target_include_directories("${_targ}" PRIVATE "${_normalized_includes}")
    endif (DEFINED SIMH_INCLUDES)

    set(SIMH_SIMLIB simhcore)
    set(SIMH_VIDLIB "")

    if (SIMH_INT64)
            set(SIMH_SIMLIB simhi64)
    elseif (SIMH_FULL64)
            set(SIMH_SIMLIB simhz64)
    endif ()
    if (SIMH_VIDEO)
            set(SIMH_VIDLIB "_video")
    endif (SIMH_VIDEO)

    target_link_libraries("${_targ}" PRIVATE "${SIMH_SIMLIB}${SIMH_VIDLIB}")
endfunction ()


function (add_simulator _targ)
    simh_executable_template(${_targ} "${ARGN}")
    cmake_parse_arguments(SIMH "INT64;FULL64;BUILDROMS;VIDEO" "TEST;SOURCE_DIR;LABEL" "DEFINES;INCLUDES;SOURCES" ${ARGN})

    # Remember to add the install rule, which defaults to ${CMAKE_SOURCE_DIR}/BIN.
    # Installs the executables.
    install(TARGETS "${_targ}" RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    ## Simulator-specific tests:
    if (DEFINED SIMH_TEST)
        set(test_fname ${CMAKE_CURRENT_SOURCE_DIR})
        if (DEFINED SIMH_SOURCE_DIR)
            ## Must be the "all-in-one" fragment... :-)
            set(test_fname ${SIMH_SOURCE_DIR})
        endif (DEFINED SIMH_SOURCE_DIR)

        string(APPEND test_fname "/tests/${SIMH_TEST}_test.ini")
        list(APPEND test_cmd "${_targ}" "RegisterSanityCheck")

        IF (EXISTS "${test_fname}")
            list(APPEND test_cmd "${test_fname}" "-v")
        ENDIF (EXISTS "${test_fname}")

        add_test(NAME "simh-${_targ}" COMMAND ${test_cmd})
        if (SIMH_LABEL)
            set_tests_properties("simh-${_targ}" PROPERTIES LABELS "simh-${SIMH_LABEL}")
        endif ()

        ## Make sure that the tests can find the DLLs/shared objects:
        file(TO_NATIVE_PATH "${SIMH_DEP_TOPDIR}/bin" native_dep_bindir)
        file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}" native_binary_dir)
        if (WIN32)
            set(test_path "PATH=${native_dep_bindir}\\\$<SEMICOLON>${native_binary_dir}\\$<SEMICOLON>")
            string(REPLACE ";" "\\\$<SEMICOLON>" escaped_path "$ENV{PATH}")
            string(APPEND test_path "${escaped_path}")
            set_property(TEST "simh-${_targ}" PROPERTY ENVIRONMENT "${test_path}")
        else (WIN32)
            set_property(TEST "simh-${_targ}" PROPERTY
                            ENVIRONMENT "LD_LIBRARY_PATH=${native_dep_bindir}:${native_binary_dir}:\$LD_LIBRARY_PATH"
            )
        endif (WIN32)
    endif (DEFINED SIMH_TEST)

    if (NOT DONT_USE_ROMS AND SIMH_BUILDROMS)
        add_dependencies(${_targ} BuildROMs)
    endif (NOT DONT_USE_ROMS AND SIMH_BUILDROMS)

    # Create target 'cppcheck' rule, if cppcheck detected:
    if (ENABLE_CPPCHECK AND cppcheck_cmd)
        get_property(cppcheck_includes TARGET ${_targ} PROPERTY INCLUDE_DIRECTORIES)
        get_property(cppcheck_defines  TARGET ${_targ} PROPERTY COMPILE_DEFINITIONS)
        list(TRANSFORM cppcheck_includes PREPEND "-I")
        list(TRANSFORM cppcheck_defines  PREPEND "-D")
        add_custom_target("${_targ}_cppcheck"
            COMMAND ${cppcheck_cmd} ${cppcheck_defines} ${cppcheck_includes} ${SIMH_SOURCES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

        add_dependencies(cppcheck "${_targ}_cppcheck")
    endif (ENABLE_CPPCHECK AND cppcheck_cmd)
endfunction ()


function(add_unit_test _targ)
    set(UNIT_TARGET "simbase-${_targ}")
    set(UNIT_TEST "simbase-${_targ}")

    simh_executable_template(${UNIT_TARGET} "${ARGN}")
    cmake_parse_arguments(SIMH "INT64;FULL64;BUILDROMS;VIDEO" "SOURCE_DIR;LABEL" "DEFINES;INCLUDES;SOURCES" ${ARGN})

    target_link_libraries(${UNIT_TARGET} PUBLIC unittest)
    add_test(NAME ${UNIT_TEST} COMMAND ${UNIT_TARGET})

    set(TEST_LABEL "simbase;unit")
    if (SIMH_LABEL)
      list(APPEND TEST_LABEL "simbase-${SIMH_LABEL}")
    endif ()
    set_tests_properties(${UNIT_TEST} PROPERTIES LABELS "${TEST_LABEL}")
endfunction ()

##~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
## Now build things!
##~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

add_custom_target(cppcheck)

build_simcore(simhcore)
build_simcore(simhi64        INT64)
build_simcore(simhz64        INT64 ADDR64)
build_simcore(simhcore_video VIDEO)
build_simcore(simhi64_video  VIDEO INT64)
build_simcore(simhz64_video  VIDEO INT64 ADDR64)

if (NOT DONT_USE_ROMS)
    add_executable(BuildROMs sim_BuildROMs.c)
    add_custom_command(
        OUTPUT
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka655x_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka620_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka630_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka610_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka410_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka411_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka412_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka41a_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka41d_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka42a_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka42b_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka43a_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka46a_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka47a_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka48a_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_is1000_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka410_xs_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka420_rdrz_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka420_rzrz_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka4xx_4pln_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka4xx_8pln_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka4xx_dz_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka4xx_spx_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka750_bin_new.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_ka750_bin_old.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_vcb02_bin.h
            ${CMAKE_SOURCE_DIR}/VAX/vax_vmb_exe.h
            ${CMAKE_SOURCE_DIR}/PDP11/pdp11_vt_lunar_rom.h
            ${CMAKE_SOURCE_DIR}/PDP11/pdp11_dazzle_dart_rom.h
            ${CMAKE_SOURCE_DIR}/PDP11/pdp11_11logo_rom.h
            ${CMAKE_SOURCE_DIR}/swtp6800/swtp6800/swtp_swtbug_bin.h
            ${CMAKE_SOURCE_DIR}/3B2/rom_400_bin.h
        MAIN_DEPENDENCY BuildROMs
        COMMAND BuildROMS
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif ()

## Front panel test.
##
## From all evidence in makefile, sim_frontpanel isn't used yet by any targets.
##
## Needs curses...
add_executable(frontpaneltest
    ${CMAKE_SOURCE_DIR}/frontpanel/FrontPanelTest.c
    ${CMAKE_SOURCE_DIR}/sim_sock.c
    ${CMAKE_SOURCE_DIR}/sim_frontpanel.c)

target_link_libraries(frontpaneltest PUBLIC thread_lib)

if (WIN32)
    target_link_libraries(frontpaneltest PUBLIC simh_network)

    if (MSVC)
            target_link_options(frontpaneltest PUBLIC "/SUBSYSTEM:CONSOLE")
    elseif (MINGW)
            target_link_options(frontpaneltest PUBLIC "-mconsole")
    endif ()
endif (WIN32)