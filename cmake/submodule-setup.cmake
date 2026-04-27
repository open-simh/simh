##-=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
## Git submodule setup.
##
## This script executes "git submodule init" for SIMH's submodule dependencies.
##-=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=

## Standard submodules: (none at the moment)
set(SIMH_GIT_SUBMODS "")

find_package(Git REQUIRED)
if (NOT GIT_FOUND)
    message(FATAL_ERROR "Git is required to configure open-simh.")
endif ()

## On Windows (MS Visual C), we definitely want vcpkg and capture its status
## unless there is an existing vcpkg installation specified by the user. Don't
## try anything if the generator's toolset happens to be an XP toolset; vcpkg
## doesn't support XP.
##
## Visual Studio's "vcpkg install" requires a 'builtin-baseline' in the vcpkg.json
## manifest. This complicates SIMH's maintenance since builtin-baseline fixes the
## Git hash (and version) of the packages and needs to be periodically updated. We
## can detect if Visual Studio is configuring SIMH via the VisualStudioVersion
## environment variable as a workaround.
if (MSVC AND NOT CMAKE_GENERATOR_TOOLSET MATCHES "v[0-9][0-9][0-9]_xp" AND
    (DEFINED ENV{VisualStudioVersion} OR NOT DEFINED ENV{VCPKG_ROOT}))
    ## Yup. Gonna use vcpkg.
    list(APPEND SIMH_GIT_SUBMODS "vcpkg")

    ## Set the VCPKG_ROOT environment variable
    set(ENV{VCPKG_ROOT} ${CMAKE_SOURCE_DIR}/vcpkg)
    message(STATUS "Using vcpkg submodule as VCPKG_ROOT ($ENV{VCPKG_ROOT})")
endif ()

if (NOT SIMH_GIT_SUBMODS)
    return ()
endif ()

message(STATUS "Updating Git submodules: ${SIMH_GIT_SUBMODS}")
execute_process(COMMAND "${GIT_EXECUTABLE}" "submodule" "update" "--init" "--recursive" ${SIMH_GIT_SUBMODS}
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                RESULT_VARIABLE GIT_SUBMODULE_INIT_RESULT)

if (NOT GIT_SUBMODULE_INIT_RESULT)
    set(do_vcpkg_boot FALSE)
    execute_process(COMMAND ${GIT_EXECUTABLE} "submodule" "status" "vcpkg"
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    OUTPUT_VARIABLE VCPKG_STATUS_OUTPUT
                    RESULT_VARIABLE VCPKG_STATUS_RESULT)

    if (NOT VCPKG_STATUS_RESULT)
        string(SUBSTRING VCPKG_STATUS_OUTPUT 0 1 VCPKG_STATUS_IND)
        ## If the first character is "-" or "+", we'll need to run the bootstrap batch file.
        if (VCPKG_STATUS_IND STREQUAL "-" OR VCPKG_STATUS_IND STREQUAL "+")
            set(do_vcpkg_boot TRUE)
        endif ()
    endif ()

    if (do_vcpkg_boot)
        message(STATUS "Bootstrapping vcpkg")
        execute_process(COMMAND "bootstrap-vcpkg.bat"
                        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/vcpkg"
                        RESULT_VARIABLE VCPKG_BOOT_RESULT)
        if (VCPKG_BOOT_RESULT)
            message(FATAL_ERROR "Failed to bootstrap vcpkg (status ${VCPKG_BOOT_RESULT})."
                "\n"
                "Output:\n${VCPKG_BOOT_OUTPUT}"
                "\n"
                "Error:\n${VCPKG_BOOT_ERROR}")
        endif ()
    endif ()
else ()
    message(WARNING "Failed to initialize git submodules.")
endif ()
