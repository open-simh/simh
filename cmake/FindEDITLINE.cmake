#
# This module is designed to find/handle editline library
#
# Requirements:
# - CMake >= 2.8.3 (for new version of find_package_handle_standard_args)
#
# The following variables will be defined for your use:
#   - EDITLINE_INCLUDE_DIRS  : editline include directory
#   - EDITLINE_LIBRARIES     : editline libraries
#   - EDITLINE_VERSION       : complete version of editline (x.y)
#   - EDITLINE_MAJOR_VERSION : major version of editline
#   - EDITLINE_MINOR_VERSION : minor version of editline
#
# How to use:
#   1) Copy this file in the root of your project source directory
#   2) Then, tell CMake to search this non-standard module in your project directory by adding to your CMakeLists.txt:
#        set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
#   3) Finally call find_package(EditLine) once
#
# Here is a complete sample to build an executable:
#
#   set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
#
#   find_package(EditLine REQUIRED) # Note: name is case sensitive
#
#   include_directories(${EDITLINE_INCLUDE_DIRS})
#   add_executable(myapp myapp.c)
#   target_link_libraries(myapp ${EDITLINE_LIBRARIES})
#


#=============================================================================
# Copyright (c) 2014, julp
#
# Distributed under the OSI-approved BSD License
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#=============================================================================

########## Private ##########
if(NOT DEFINED EDITLINE_PUBLIC_VAR_NS)
    set(EDITLINE_PUBLIC_VAR_NS "EDITLINE")
endif(NOT DEFINED EDITLINE_PUBLIC_VAR_NS)
if(NOT DEFINED EDITLINE_PRIVATE_VAR_NS)
    set(EDITLINE_PRIVATE_VAR_NS "_${EDITLINE_PUBLIC_VAR_NS}")
endif(NOT DEFINED EDITLINE_PRIVATE_VAR_NS)

function(editline_debug _VARNAME)
    if(${EDITLINE_PUBLIC_VAR_NS}_DEBUG)
        if(DEFINED ${EDITLINE_PUBLIC_VAR_NS}_${_VARNAME})
            message("${EDITLINE_PUBLIC_VAR_NS}_${_VARNAME} = ${${EDITLINE_PUBLIC_VAR_NS}_${_VARNAME}}")
        else(DEFINED ${EDITLINE_PUBLIC_VAR_NS}_${_VARNAME})
            message("${EDITLINE_PUBLIC_VAR_NS}_${_VARNAME} = <UNDEFINED>")
        endif()
    endif()
endfunction()

# Alias all EditLine_FIND_X variables to EDITLINE_FIND_X
# Workaround for find_package: no way to force case of variable's names it creates (I don't want to change MY coding standard)
# ---
# NOTE: only prefix is considered, not full name of the variables to minimize conflicts with string(TOUPPER) for example
# EditLine_foo becomes EDITLINE_foo not EditLine_FOO as this is two different variables
set(${EDITLINE_PRIVATE_VAR_NS}_FIND_PKG_PREFIX "EditLine")
get_directory_property(${EDITLINE_PRIVATE_VAR_NS}_CURRENT_VARIABLES VARIABLES)
foreach(${EDITLINE_PRIVATE_VAR_NS}_VARNAME ${${EDITLINE_PRIVATE_VAR_NS}_CURRENT_VARIABLES})
    if(${EDITLINE_PRIVATE_VAR_NS}_VARNAME MATCHES "^${${EDITLINE_PRIVATE_VAR_NS}_FIND_PKG_PREFIX}")
        string(REGEX REPLACE "^${${EDITLINE_PRIVATE_VAR_NS}_FIND_PKG_PREFIX}" "${EDITLINE_PUBLIC_VAR_NS}" ${EDITLINE_PRIVATE_VAR_NS}_NORMALIZED_VARNAME ${${EDITLINE_PRIVATE_VAR_NS}_VARNAME})
        set(${${EDITLINE_PRIVATE_VAR_NS}_NORMALIZED_VARNAME} ${${${EDITLINE_PRIVATE_VAR_NS}_VARNAME}})
    endif()
endforeach()

########## Public ##########
find_path(
    ${EDITLINE_PUBLIC_VAR_NS}_INCLUDE_DIRS
    NAMES histedit.h
)

if(${EDITLINE_PUBLIC_VAR_NS}_INCLUDE_DIRS)

    find_library(
        ${EDITLINE_PUBLIC_VAR_NS}_LIBRARIES
        NAMES edit
    )

    find_library(
        ${EDITLINE_PUBLIC_VAR_NS}_TERMCAP
        NAMES termcap
    )

#     file(READ "${${EDITLINE_PUBLIC_VAR_NS}_INCLUDE_DIRS}/histedit.h" ${EDITLINE_PRIVATE_VAR_NS}_H_CONTENT)
#     string(REGEX REPLACE ".*# *define +LIBEDIT_MAJOR +([0-9]+).*" "\\1" ${EDITLINE_PUBLIC_VAR_NS}_MAJOR_VERSION ${${EDITLINE_PRIVATE_VAR_NS}_H_CONTENT})
#     string(REGEX REPLACE ".*# *define +LIBEDIT_MINOR +([0-9]+).*" "\\1" ${EDITLINE_PUBLIC_VAR_NS}_MINOR_VERSION ${${EDITLINE_PRIVATE_VAR_NS}_H_CONTENT})
#     set(${EDITLINE_PUBLIC_VAR_NS}_VERSION "${${EDITLINE_PUBLIC_VAR_NS}_MAJOR_VERSION}.${${EDITLINE_PUBLIC_VAR_NS}_MINOR_VERSION}")

    include(FindPackageHandleStandardArgs)
    if(${EDITLINE_PUBLIC_VAR_NS}_FIND_REQUIRED AND NOT ${EDITLINE_PUBLIC_VAR_NS}_FIND_QUIETLY)
        find_package_handle_standard_args(
            ${EDITLINE_PUBLIC_VAR_NS}
            REQUIRED_VARS ${EDITLINE_PUBLIC_VAR_NS}_LIBRARIES ${EDITLINE_PUBLIC_VAR_NS}_INCLUDE_DIRS
#             VERSION_VAR ${EDITLINE_PUBLIC_VAR_NS}_VERSION
        )
    else(${EDITLINE_PUBLIC_VAR_NS}_FIND_REQUIRED AND NOT ${EDITLINE_PUBLIC_VAR_NS}_FIND_QUIETLY)
        find_package_handle_standard_args(${EDITLINE_PUBLIC_VAR_NS} "editline not found" ${EDITLINE_PUBLIC_VAR_NS}_LIBRARIES ${EDITLINE_PUBLIC_VAR_NS}_INCLUDE_DIRS)
    endif()

else()

    if(${EDITLINE_PUBLIC_VAR_NS}_FIND_REQUIRED AND NOT ${EDITLINE_PUBLIC_VAR_NS}_FIND_QUIETLY)
        message(FATAL_ERROR "Could not find editline include directory")
    endif(${EDITLINE_PUBLIC_VAR_NS}_FIND_REQUIRED AND NOT ${EDITLINE_PUBLIC_VAR_NS}_FIND_QUIETLY)

endif()

mark_as_advanced(
    ${EDITLINE_PUBLIC_VAR_NS}_INCLUDE_DIRS
    ${EDITLINE_PUBLIC_VAR_NS}_LIBRARIES
)

if (${EDITLINE_PUBLIC_VAR_NS}_FOUND)
    add_library(Editline::Editline UNKNOWN IMPORTED)
    set_target_properties(Editline::Editline PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${${EDITLINE_PUBLIC_VAR_NS}_INCLUDE_DIRS}"
        INTERFACE_COMPILE_DEFINITIONS "HAVE_EDITLINE"
        INTERFACE_LINK_LIBRARIES "$<$<BOOL:${${EDITLINE_PUBLIC_VAR_NS}_TERMCAP}>:${${EDITLINE_PUBLIC_VAR_NS}_TERMCAP}>")
    set_property(TARGET Editline::Editline APPEND PROPERTY
        IMPORTED_LOCATION "${${EDITLINE_PUBLIC_VAR_NS}_LIBRARIES}")
endif ()

# IN (args)
editline_debug("FIND_REQUIRED")
editline_debug("FIND_QUIETLY")
editline_debug("FIND_VERSION")
# OUT
# Linking
editline_debug("INCLUDE_DIRS")
editline_debug("LIBRARIES")
editline_debug("TERMCAP")
# Version
# editline_debug("MAJOR_VERSION")
# editline_debug("MINOR_VERSION")
# editline_debug("VERSION")
