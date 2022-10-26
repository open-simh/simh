# Locate the PCRE library
#
# This module defines:
#
# ::
#
#   PCRE_LIBRARIES, the name of the library to link against
#   PCRE_INCLUDE_DIRS, where to find the headers
#   PCRE_FOUND, if false, do not try to link against
#
# Tweaks:
# 1. PCRE_PATH: A list of directories in which to search
# 2. PCRE_DIR: An environment variable to the directory where you've unpacked or installed PCRE.
#
# "scooter me fecit"

function(findpcre_debug _VARNAME)
    if(FINDPCRE_DEBUG)
        if(DEFINED PCRE_${_VARNAME})
            message("PCRE_${_VARNAME} = ${PCRE_${_VARNAME}}")
        else(DEFINED PCRE_${_VARNAME})
            message("PCRE_${_VARNAME} = <UNDEFINED>")
        endif(DEFINED PCRE_${_VARNAME})
    endif()
endfunction(findpcre_debug)

## Normal path to find the PCRE header and library:
find_path(PCRE_INCLUDE_DIR pcre.h
        HINTS
            ${PC_PCRE_INCLUDEDIR}
            ${PC_PCRE_INCLUDE_DIRS}
            ENV PCRE_DIR
        # path suffixes to search inside ENV{PCRE_DIR}
        PATHS ${PCRE_PATH}
        PATH_SUFFIXES
            pcre
            PCRE
        )

find_library(PCRE_LIBRARY_RELEASE
    NAMES
        pcre
        libpcre
    HINTS
        ${PC_PCRE_LIBDIR}
        ${PC_PCRE_LIBRARY_DIRS}
        ENV PCRE_DIR
    PATH_SUFFIXES
        ${LIB_PATH_SUFFIXES}
    PATHS
        ${PCRE_PATH}
)

find_library(PCRE_LIBRARY_DEBUG
    NAMES
        pcred
        libpcred
    HINTS
        ENV PCRE_DIR
    PATH_SUFFIXES
        ${LIB_PATH_SUFFIXES}
    PATHS
        ${PCRE_PATH}
)

if (PCRE_INCLUDE_DIR)
    if (EXISTS "${PCRE_INCLUDE_DIR}/pcre.h")
        file(STRINGS "${PCRE_INCLUDE_DIR}/pcre.h" PCRE_VERSION_MAJOR_LINE REGEX "^#define[ \t]+PCRE_MAJOR[ \t]+[0-9]+$")
        file(STRINGS "${PCRE_INCLUDE_DIR}/pcre.h" PCRE_VERSION_MINOR_LINE REGEX "^#define[ \t]+PCRE_MINOR[ \t]+[0-9]+$")
    endif ()

    string(REGEX REPLACE "^#define[ \t]+PCRE?_MAJOR[ \t]+([0-9]+)$" "\\1" PCRE_VERSION_MAJOR "${PCRE_VERSION_MAJOR_LINE}")
    string(REGEX REPLACE "^#define[ \t]+PCRE?_MINOR[ \t]+([0-9]+)$" "\\1" PCRE_VERSION_MINOR "${PCRE_VERSION_MINOR_LINE}")

    set(PCRE_VERSION_STRING "${PCRE_VERSION_MAJOR}.${PCRE_VERSION_MINOR}")
    unset(PCRE_VERSION_MAJOR_LINE)
    unset(PCRE_VERSION_MINOR_LINE)
    unset(PCRE_VERSION_MAJOR)
    unset(PCRE_VERSION_MINOR)
endif ()

include(SelectLibraryConfigurations)
select_library_configurations(PCRE)

set(PCRE_LIBRARIES ${PCRE_LIBRARY})
set(PCRE_INCLUDE_DIRS ${PCRE_INCLUDE_DIR})

findpcre_debug(LIBRARY)
findpcre_debug(LIBRARIES)
findpcre_debug(LIBRARY_DEBUG)
findpcre_debug(LIBRARY_RELEASE)
findpcre_debug(VERSION_STRING)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    PCRE
    REQUIRED_VARS
        PCRE_LIBRARY
        PCRE_INCLUDE_DIR
    VERSION_VAR
	    PCRE_VERSION_STRING
)
