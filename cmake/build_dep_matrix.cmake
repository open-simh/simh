#~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
# build_dep_matrix.cmake
#
# This is a minor hack to build all of the various library compile
# configurations. Might take a bit more time upfront to build the
# dependencies, but the user doesn't have to go backward and attempt
# to build the dependencies themselves.
#
# Author: B. Scott Michel
# "scooter me fecit"
#~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=


function(BuildDepMatrix dep pretty)
    cmake_parse_arguments(_BDM "" "RELEASE_BUILD;DEBUG_BUILD" "CMAKE_ARGS" ${ARGN})

    set(cmake_cfg_args
            "-G${CMAKE_GENERATOR}"
            "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
            "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")

    if (CMAKE_GENERATOR_PLATFORM)
        list(APPEND cmake_cfg_args "-A" "${CMAKE_GENERATOR_PLATFORM}")
    endif ()
    if (CMAKE_GENERATOR_TOOLSET)
        list(APPEND cmake_cfg_args "-T" "${CMAKE_GENERATOR_TOOLSET}")
    endif ()
    string(REPLACE ";" "$<SEMICOLON>" _amend_cmake_prefix_path "${SIMH_PREFIX_PATH_LIST}")
    string(REPLACE ";" "$<SEMICOLON>" _amend_cmake_include_path "${SIMH_INCLUDE_PATH_LIST}")

    list(APPEND cmake_cfg_args ${DEP_CMAKE_ARGS})
    list(APPEND cmake_cfg_args -DCMAKE_PREFIX_PATH=${_amend_cmake_prefix_path}
                               -DCMAKE_INCLUDE_PATH=${_amend_cmake_include_path}
                               ${_BDM_CMAKE_ARGS}
                               "<SOURCE_DIR>"
    )

    if (NOT _BDM_RELEASE_BUILD)
        set(_BDM_RELEASE_BUILD "Release")
    endif (NOT _BDM_RELEASE_BUILD)

    if (NOT _BDM_DEBUG_BUILD)
        set(_BDM_DEBUG_BUILD "Debug")
    endif (NOT _BDM_DEBUG_BUILD)

    set(dep_cmds)
    foreach (cfg IN ITEMS ${_BDM_DEBUG_BUILD} ${_BDM_RELEASE_BUILD})
        ## Set the MSVC runtime. Can't use a generator expression here,
        ## have to "nail it down."
        set(use_msvcrt "MultiThreaded")
        if (cfg STREQUAL ${_BDM_DEBUG_BUILD})
            string(APPEND use_msvcrt "Debug")
        endif ()
        if (BUILD_SHARED_DEPS)
            string(APPEND use_msvcrt "DLL")
        endif ()
        list(APPEND dep_cmds COMMAND ${CMAKE_COMMAND} -E echo "-- Building ${pretty} '${cfg}' configuration")
        list(APPEND dep_cmds COMMAND ${CMAKE_COMMAND} -E remove -f CMakeCache.txt)
        list(APPEND dep_cmds COMMAND ${CMAKE_COMMAND} -E remove_directory CMakeFiles)
        list(APPEND dep_cmds COMMAND ${CMAKE_COMMAND}
            ${cmake_cfg_args}
            -DCMAKE_BUILD_TYPE:STRING=${cfg}
            -DCMAKE_INSTALL_PREFIX:STRING=${SIMH_DEP_TOPDIR}
            -DCMAKE_POLICY_DEFAULT_CMP0091:STRING=NEW
            -DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=${use_msvcrt}
            )
        list(APPEND dep_cmds COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config "${cfg}" --clean-first)
        list(APPEND dep_cmds COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR> --config ${cfg})
    endforeach ()

    ## Unset CMAKE_MODULE_PATH temporarily for external projects
    set(_saved_cmake_module_path ${CMAKE_MODULE_PATH})
    set(CMAKE_MODULE_PATH "")

    ## message("${dep_cmds}")
    ExternalProject_Add_Step(${dep} build-dbg-release
        DEPENDEES configure
        WORKING_DIRECTORY <BINARY_DIR>
        ${dep_cmds}
    )

    set(CMAKE_MODULE_PATH ${_saved_cmake_module_path})
    unset(_saved_cmake_module_path)
endfunction ()

