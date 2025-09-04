# Locate the PCAP library
#
# This module defines:
#
# ::
#
#   PCAP_LIBRARIES, the name of the library to link against
#   PCAP_INCLUDE_DIRS, where to find the headers
#   PCAP_FOUND, if false, do not try to link against
#   PCAP_VERSION_STRING - human-readable string containing the version of SDL_ttf
#
# Tweaks:
# 1. PCAP_PATH: A list of directories in which to search
# 2. PCAP_DIR: An environment variable to the directory where you've unpacked or installed PCAP.
#
# "scooter me fecit"

find_path(PCAP_INCLUDE_DIR 
    NAMES
        pcap.h
    HINTS
        ENV PCAP_DIR
    PATHS
        pcap
        PCAP
        ${PCAP_PATH}
    )

# if (CMAKE_SIZEOF_VOID_P EQUAL 8)
#     set(LIB_PATH_SUFFIXES lib64 x64 amd64 x86_64-linux-gnu aarch64-linux-gnu lib)
# else ()
#     set(LIB_PATH_SUFFIXES x86)
# endif ()

if (NEED_PCAP_LIBRARY)^M
    find_library(PCAP_LIBRARY^M
            NAMES^M
                pcap pcap_static libpcap libpcap_static^M
            HINTS^M
                ENV PCAP_DIR^M
            PATH_SUFFIXES^M
                ${LIB_PATH_SUFFIXES}^M
            PATHS^M
                ${PCAP_PATH}^M
            )^M
    message(STATUS "LIB_PATH_SUFFIXES ${LIB_PATH_SUFFIXES}")^M
    message(STATUS "PCAP_LIBRARY is ${PCAP_LIBRARY}")^M
endif()^M

# if (WIN32 AND PCAP_LIBRARY)
#     ## Only worry about the packet library on Windows.
#     find_library(PACKET_LIBRARY
#         NAMES
#             packet Packet
#         HINTS
#             ENV PCAP_DIR
#         PATH_SUFFIXES
#             ${LIB_PATH_SUFFIXES}
#         PATHS
#             ${PCAP_PATH}
#     )
# else (WIN32 AND PCAP_LIBRARY)
#     set(PACKET_LIBRARY)
# endif (WIN32 AND PCAP_LIBRARY)
# ## message(STATUS "PACKET_LIBRARY is ${PACKET_LIBRARY}")

if (NEED_PCAP_LIBRARY)^M
    set(PCAP_LIBRARIES ${PCAP_LIBRARY} ${PACKET_LIBRARY})^M
endif()^M
set(PCAP_INCLUDE_DIRS ${PCAP_INCLUDE_DIR})
unset(PCAP_LIBRARY)
unset(PCAP_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
if (NEED_PCAP_LIBRARY)^M
find_package_handle_standard_args(^M
    PCAP^M
    REQUIRED_VARS^M
        PCAP_LIBRARIES^M
        PCAP_INCLUDE_DIRS^M
)^M
else()^M
find_package_handle_standard_args(
    PCAP
    REQUIRED_VARS
        PCAP_INCLUDE_DIRS
)
endif()^M
