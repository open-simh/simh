## File link or copy
##
## Written initially for the VAX to link vax to microvax3900, this
## evolved into a more general-purpose utility.

if (NOT SRCFILE)
    message(FATAL_ERROR "SRCFILE not defined")
endif ()
if (NOT DSTFILE)
    mesasge(FATAL_ERROR "DSTFILE not defined")
endif ()
if (NOT WORKING_DIR)
    message(FATAL_ERROR "WORKING_DIR not defined")
endif ()

if (NOT EXISTS ${WORKING_DIR})
    message(FATAL_ERROR "Working directory does not exist: ${WORKING_DIR}")
endif ()

file(TO_NATIVE_PATH "${WORKING_DIR}/${SRCFILE}" _source)
file(TO_NATIVE_PATH "${WORKING_DIR}/${DSTFILE}" _dest)

if (EXISTS ${_dest})
    message("Removing destination ${_dest}")
    file(REMOVE ${_dest})
    if (EXISTS ${_dest})
        message(FATAL_ERROR "Could not remove ${_dest}")
    endif ()
endif ()

execute_process(
    COMMAND
        ${CMAKE_COMMAND} -E create_symlink ${SRCFILE} ${DSTFILE}
    WORKING_DIRECTORY
        ${WORKING_DIR}
    RESULT_VARIABLE
        _file_symlink
    ERROR_QUIET
)

if (NOT _file_symlink EQUAL 0)
    file(CREATE_LINK ${_source} ${_dest} COPY_ON_ERROR RESULT _result)
    if (NOT _result EQUAL 0)
        message(FATAL_ERROR "Could not link or copy ${_source} to ${_dest}")
    else ()
        message(":::: Hard link/copy ${_source} -> ${_dest}")
    endif ()
else ()
    message(":::: Symlink ${SRCFILE} -> ${DSTFILE} in ${WORKING_DIR}")
endif()
