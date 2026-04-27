## git-commit-id.cmake
##
## Get the current Git commit hash code and commit time, update
## .git-commit-id and .git-commit-id.h

set(GIT_COMMIT_ID ${GIT_COMMIT_DEST}/.git-commit-id)
set(GIT_COMMIT_ID_H ${GIT_COMMIT_DEST}/.git-commit-id.h)

find_package(Git QUIET)
if (GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} "log" "-1" "--pretty=%H"
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    RESULT_VARIABLE HAVE_GIT_COMMIT_HASH
                    OUTPUT_VARIABLE SIMH_GIT_COMMIT_HASH)
    execute_process(COMMAND ${GIT_EXECUTABLE} "log" "-1" "--pretty=%aI"
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    RESULT_VARIABLE HAVE_GIT_COMMIT_TIME
                    OUTPUT_VARIABLE SIMH_GIT_COMMIT_TIME)

    execute_process(COMMAND ${GIT_EXECUTABLE} "update-index" "--refresh" "--"
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    RESULT_VARIABLE HAVE_UNCOMMITTED_CHANGES
                    OUTPUT_VARIABLE SIMH_UNCOMMITTED_CHANGES)
endif ()

if (GIT_FOUND AND NOT HAVE_GIT_COMMIT_HASH AND NOT HAVE_GIT_COMMIT_TIME)
    string(STRIP ${SIMH_GIT_COMMIT_HASH} SIMH_GIT_COMMIT_HASH)
    string(STRIP ${SIMH_GIT_COMMIT_TIME} SIMH_GIT_COMMIT_TIME)
    string(REPLACE "T" " " SIMH_GIT_COMMIT_TIME ${SIMH_GIT_COMMIT_TIME})

    if (HAVE_UNCOMMITTED_CHANGES)
        string(APPEND SIMH_GIT_COMMIT_HASH "+uncommitted-changes")
    else ()
        message(STATUS "Clean working directory, no uncommitted changes.")
    endif ()

    set(WRITE_GIT_COMMIT_FILES True)
    if (EXISTS ${GIT_COMMIT_ID})
        set(EXISTING_GIT_COMMIT_HASH)
        set(EXISTING_GIT_COMMIT_TIME)
        file(STRINGS ${GIT_COMMIT_ID} git_info)
        foreach (inp IN LISTS git_info)
            if (inp MATCHES "SIM_GIT_COMMIT_ID (.*)")
                set(EXISTING_GIT_COMMIT_HASH ${CMAKE_MATCH_1})
            elseif (inp MATCHES "SIM_GIT_COMMIT_TIME (.*)")
                set(EXISTING_GIT_COMMIT_TIME ${CMAKE_MATCH_1})
            endif ()
        endforeach()
        if (EXISTING_GIT_COMMIT_HASH STREQUAL SIMH_GIT_COMMIT_HASH AND
                EXISTING_GIT_COMMIT_TIME STREQUAL SIMH_GIT_COMMIT_TIME)
            ## message(STATUS "GIT hash and time match, not writing files.")
            set(WRITE_GIT_COMMIT_FILES False)
        endif ()
    endif ()

    if (WRITE_GIT_COMMIT_FILES)
        message(STATUS "Updating GIT commit ID")
        message(STATUS "SIM_GIT_COMMIT_ID:   ${SIMH_GIT_COMMIT_HASH}")
        message(STATUS "SIM_GIT_COMMIT_TIME: ${SIMH_GIT_COMMIT_TIME}")

        message(STATUS "Writing ${GIT_COMMIT_ID}")
        file(WRITE ${GIT_COMMIT_ID}
            "SIM_GIT_COMMIT_ID ${SIMH_GIT_COMMIT_HASH}\n"
            "SIM_GIT_COMMIT_TIME ${SIMH_GIT_COMMIT_TIME}\n")

        message(STATUS "Writing ${GIT_COMMIT_ID_H}")
        file(WRITE ${GIT_COMMIT_ID_H}
            "#define SIM_GIT_COMMIT_ID ${SIMH_GIT_COMMIT_HASH}\n"
            "#define SIM_GIT_COMMIT_TIME ${SIMH_GIT_COMMIT_TIME}\n")
    ## else ()
        ## message(STATUS "No changes to ${GIT_COMMIT_ID}")
        ## message(STATUS "No changes to ${GIT_COMMIT_ID_H}")
    endif ()
else ()
    message(STATUS "SIM_GIT_COMMIT_ID not set.")
    message(STATUS "SIM_GIT_COMMIT_TIME not set.")

    if (NOT EXISTS ${GIT_COMMIT_ID_H})
        message(STATUS "Writing default ${GIT_COMMIT_ID_H}")
        file(WRITE ${GIT_COMMIT_ID_H}
            "#undef SIM_GIT_COMMIT_ID\n"
            "#undef SIM_GIT_COMMIT_TIME\n"
        )
    else ()
        message(STATUS "Leaving ${GIT_COMMIT_ID_H} intact")
    endif ()
endif()
