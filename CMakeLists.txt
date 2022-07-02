
cmake_minimum_required(VERSION 3.7.0)

project(simh)

# Do not pollute source directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/BIN)

# Options for initial configuration only
set(BUILD_ALL OFF CACHE BOOL "Initial default: build all simulators")

# Normal options that can be changed
set(ENABLE_ASYNC ON CACHE BOOL "Enable asynchronous I/O support")
set(ENABLE_GRAPHICS ON CACHE BOOL "Initial default: enable graphics support")
set(ENABLE_NETWORK ON CACHE BOOL "Initial default: enable network support")
set(EXECUTABLE_PREFIX "simx-" CACHE STRING "Prefix for simulator executables")

# Find system libraries
if(UNIX)
  find_library(LIBM_LIBRARIES m)
  find_library(LIBDL_LIBRARIES dl)
  find_library(LIBRT_LIBRARIES rt)
else()
  set(LIBM_LIBRARIES "")
  set(LIBDL_LIBRARIES "")
  set(LIBRT_LIBRARIES "")
endif()
# Find curses
find_package(Curses)
# Find threads
find_package(Threads)
set(THREAD_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
# Find pkg-config dependencies
find_package(PkgConfig)
pkg_check_modules(PCRE libpcre)
pkg_check_modules(PCREPOSIX libpcreposix)
if(ENABLE_GRAPHICS)
  pkg_check_modules(SDL1 sdl)
  pkg_check_modules(SDL2 sdl2)
endif()
if(ENABLE_NETWORK)
  pkg_check_modules(VDEPLUG vdeplug)
endif()
if(WIN32)
  find_package(pthread REQUIRED)
endif()

# GNU toolchain
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
  set(COMPILER_IS_GNU ON)
  set(COMPILER_IS_GNU_LIKE ON)
endif()
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
  set(COMPILER_IS_GNU_LIKE ON)
endif()
if(COMPILER_IS_GNU_LIKE)
  # should be C90, but code uses to many C++ comments...
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
  # enable lots of warnings
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-address")
  # allow setting the CPU architecture
  set(COMPILER_ARCH "" CACHE STRING "Compile using specific CPU architecture")
  if(COMPILER_ARCH)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=${COMPILER_ARCH}")
  endif()
  # tell the optimizer about various pitfalls
  set(COMMON_SAFETY
	"-fno-strict-aliasing -fno-strict-overflow")
  if(COMPILER_IS_GNU)
    set(COMMON_SAFETY "${COMMON_SAFETY} -fno-unsafe-loop-optimizations")
  endif()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_SAFETY}")
  # optimizations for release builds
  set(COMMON_OPTIMIZATIONS "-finline-functions")
  if(COMPILER_IS_GNU)
    set(COMMON_OPTIMIZATIONS "${COMMON_OPTIMIZATIONS} -fgcse-after-reload")
  endif()
  # allow use of link-time optimization
  set(COMPILER_USE_LTO OFF CACHE BOOL "Compile with link-time optimization")
  if(COMPILER_USE_LTO)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
	if(COMPILER_IS_GNU)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-fat-lto-objects")
	endif()
  endif()
  # some additions for relwithdebinfo builds
  if("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_OPTIMIZATIONS}")
  endif()
  # additional optimizations for release builds
  if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_OPTIMIZATIONS}")
	# option to omit the frame pointer
	set(COMPILER_OMIT_FP ON CACHE BOOL "Compile without frame pointers")
	if(COMPILER_OMIT_FP)
	  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fomit-frame-pointer")
	endif()
  endif()
endif()

# Definitions common to all simulators
add_library(sim
  scp_help.h
  sim_console.h
  sim_console.c
  sim_defs.h
  sim_disk.h
  sim_disk.c
  sim_ether.h
  sim_ether.c
  sim_fio.h
  sim_fio.c
  sim_frontpanel.h
  sim_frontpanel.c
  sim_imd.h
  sim_imd.c
  sim_rev.h
  sim_serial.h
  sim_serial.c
  sim_sock.h
  sim_sock.c
  sim_tape.h
  sim_tape.c
  sim_timer.h
  sim_timer.c
  sim_tmxr.h
  sim_tmxr.c
  sim_video.h
  sim_video.c
)
target_compile_definitions(sim PUBLIC
  _GNU_SOURCE
  HAVE_GLOB
)
target_link_libraries(sim PUBLIC
  ${LIBM_LIBRARIES}
  ${THREAD_LIBRARIES}
)
target_include_directories(sim PUBLIC .)
if(WIN32)
  target_compile_definitions(sim PUBLIC
    _CRT_NONSTDC_NO_WARNINGS            # Don't warn about POSIX functions being deprecated.
    _CRT_SECURE_NO_WARNINGS             # Don't warn about unsafe C functions.
  )
  target_link_libraries(sim PUBLIC PThreads4W::PThreads4W Winmm.lib)
endif()
# These sources need to be built with each simulator as they have features
# enabled by conditional compilation.
set(SIM_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/scp.h
  ${CMAKE_CURRENT_SOURCE_DIR}/scp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sim_card.h
  ${CMAKE_CURRENT_SOURCE_DIR}/sim_card.c
)

# simulator SCSI support
add_library(sim_scsi sim_scsi.h sim_scsi.c)
target_link_libraries(sim_scsi PUBLIC sim)

# Asynchronous I/O
if(ENABLE_ASYNC)
  target_compile_definitions(sim PUBLIC SIM_ASYNCH_IO)
endif()

# Dynamic linker
if(LIBDL_LIBRARIES)
  string(SUBSTRING ${CMAKE_SHARED_LIBRARY_SUFFIX} 1 -1 LIBDL_SUFFIX)
  target_compile_definitions(sim PUBLIC HAVE_DLOPEN=${LIBDL_SUFFIX})
  target_link_libraries(sim PUBLIC ${LIBDL_LIBRARIES})
endif()

# Shared memory support
if(LIBRT_LIBRARIES)
  target_compile_definitions(sim PUBLIC HAVE_SHM_OPEN)
  target_link_libraries(sim PUBLIC ${LIBRT_LIBRARIES})
endif()

# Regular expression support
if(PCRE_FOUND AND PCREPOSIX_FOUND)
  target_compile_definitions(sim PUBLIC HAVE_PCRE_H HAVE_PCREPOSIX_H)
  target_include_directories(sim PUBLIC ${PCRE_INCLUDE_DIRS} ${PCREPOSIX_INCLUDE_DIRS})
  target_link_libraries(sim PUBLIC ${PCRE_LIBRARIES} ${PCREPOSIX_LIBRARIES})
else()
  target_compile_definitions(sim PUBLIC HAVE_REGEX_H)
endif()

# Curses support
if(CURSES_FOUND)
  target_compile_definitions(sim PUBLIC HAVE_NCURSES)
  target_include_directories(sim PUBLIC ${CURSES_INCLUDE_DIRS})
  target_link_libraries(sim PUBLIC ${CURSES_LIBRARIES})
endif()

# Graphics support
if(ENABLE_GRAPHICS)
  set(DISPLAY_DEFINITIONS USE_DISPLAY)
  set(DISPLAY_SOURCES
	${CMAKE_SOURCE_DIR}/display/display.c
	${CMAKE_SOURCE_DIR}/display/sim_ws.c
	)
  set(DISPLAY_VT_SOURCES
	${CMAKE_SOURCE_DIR}/display/vt11.c
	)
  set(GRAPHICS_DEFINITIONS USE_SIM_VIDEO)
  if(SDL2_FOUND)
    set(GRAPHICS_DEFINITIONS ${GRAPHICS_DEFINITIONS} HAVE_LIBSDL)
	set(GRAPHICS_CFLAGS ${SDL2_CFLAGS})
	set(GRAPHICS_INCLUDES ${SDL2_INCLUDE_DIRS})
	set(GRAPHICS_LIBRARIES ${SDL2_LIBRARIES})
  elseif(SDL1_FOUND)
    set(GRAPHICS_DEFINITIONS ${GRAPHICS_DEFINITIONS} HAVE_LIBSDL)
	set(GRAPHICS_CFLAGS ${SDL1_CFLAGS})
	set(GRAPHICS_INCLUDES ${SDL1_INCLUDE_DIRS})
	set(GRAPHICS_LIBRARIES ${SDL1_LIBRARIES})
  endif()
endif()

# Network support
if(ENABLE_NETWORK)
  set(NETWORK_DEFINITIONS USE_NETWORK)
  if(VDEPLUG_FOUND)
	set(NETWORK_DEFINITIONS ${NETWORK_DEFINITIONS} HAVE_VDE_NETWORK)
	set(NETWORK_INCLUDES ${NETWORK_INCLUDES} ${VDEPLUG_INCLUDE_DIRS})
	set(NETWORK_LIBRARIES ${NETWORK_LIBRARIES} ${VDEPLUG_LIBRARIES})
  endif()
  target_compile_definitions(sim PUBLIC ${NETWORK_DEFINITIONS})
  if(WIN32)
    # Don't warn about WinSock deprecated functions.
    target_compile_definitions(sim PUBLIC _WINSOCK_DEPRECATED_NO_WARNINGS)
    target_link_libraries(sim PUBLIC ws2_32.lib)
  endif()
endif()

# Utility to generate headers for inclusion of binary ROMs in simulators.
add_executable(BuildROMs sim_BuildROMs.c)
if(WIN32)
  # Suppress warnings about unsafe standard library functions.
  target_compile_definitions(BuildROMs PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

# Convenience function for defining simulators
function(add_simulator NAME)
  string(TOUPPER ${NAME} NAMEU)
  set(SIMULATOR_${NAMEU}_ENABLE ${BUILD_ALL} CACHE BOOL "Build the ${NAME} simulator")
  if(BUILD_ALL OR SIMULATOR_${NAMEU}_ENABLE)
	add_executable(${NAME} ${${NAMEU}_SOURCES} ${SIM_SOURCES})
	target_link_libraries(${NAME} PUBLIC
	  sim
	  ${${NAMEU}_LIBRARIES})
	target_compile_definitions(${NAME} PUBLIC
	  ${${NAMEU}_DEFINITIONS}
	  )
	target_include_directories(${NAME} PUBLIC
	  ${${NAMEU}_INCLUDES}
	  ${CMAKE_SOURCE_DIR}
	  ${CMAKE_CURRENT_SOURCE_DIR}
	  )
    set_target_properties(${NAME} PROPERTIES
      OUTPUT_NAME ${EXECUTABLE_PREFIX}${NAME}
      )
	install(TARGETS ${NAME}
	  RUNTIME DESTINATION bin
	  )
  endif()
endfunction()

# Function for adding graphics support
function(add_simulator_graphics NAME)
  string(TOUPPER ${NAME} NAMEU)
  if(ENABLE_GRAPHICS)
	set(SIMULATOR_${NAMEU}_GRAPHICS ${ENABLE_GRAPHICS} CACHE BOOL "Enable ${NAME} graphics support")
	if(BUILD_ALL OR SIMULATOR_${NAMEU}_ENABLE)
	  if(SIMULATOR_${NAMEU}_GRAPHICS)
		  target_link_libraries(${NAME} PUBLIC ${GRAPHICS_LIBRARIES})
		  target_compile_definitions(${NAME} PUBLIC USE_SIM_VIDEO)
		  target_compile_definitions(${NAME} PUBLIC ${GRAPHICS_DEFINITIONS})
		  target_include_directories(${NAME} PUBLIC ${GRAPHICS_INCLUDES})
	  endif()
    endif()
  endif()
endfunction()

# Function for adding display support
function(add_simulator_display NAME)
  string(TOUPPER ${NAME} NAMEU)
  if(ENABLE_GRAPHICS)
    if(BUILD_ALL OR SIMULATOR_${NAMEU}_ENABLE)
      if(SIMULATOR_${NAMEU}_GRAPHICS)
        target_compile_definitions(${NAME} PUBLIC USE_DISPLAY)
	    target_compile_definitions(${NAME} PUBLIC ${DISPLAY_DEFINITIONS})
      endif()
    endif()
  endif()
endfunction()

# Function for adding network support
function(add_simulator_network NAME)
  string(TOUPPER ${NAME} NAMEU)
  if(ENABLE_NETWORK)
	set(SIMULATOR_${NAMEU}_NETWORK ${ENABLE_NETWORK} CACHE BOOL "Enable ${NAME} network support")
	if(BUILD_ALL OR SIMULATOR_${NAMEU}_ENABLE)
	  if(SIMULATOR_${NAMEU}_NETWORK)
	    target_link_libraries(${NAME} PUBLIC ${NETWORK_LIBRARIES})
	    target_compile_definitions(${NAME} PUBLIC ${NETWORK_DEFINITIONS})
	    target_include_directories(${NAME} PUBLIC ${NETWORK_INCLUDES})
	  endif()
    endif()
  endif()
endfunction()

# Advanced Computer Design
add_subdirectory(PDQ-3)
# AT&T
add_subdirectory(3B2)
# Burroughs Corporation
add_subdirectory(B5500)
# Control Data Corporation (CDC)
add_subdirectory(CDC1700)
# Data General (DG)
add_subdirectory(NOVA)
# Digital Equipment Corporation (DEC)
add_subdirectory(alpha)
add_subdirectory(PDP1)
add_subdirectory(PDP8)
add_subdirectory(PDP10)
add_subdirectory(PDP11)
add_subdirectory(PDP18B)
add_subdirectory(VAX)
# GRI Computer Corporation
add_subdirectory(GRI)
# Hewlett-Packard (HP)
add_subdirectory(HP2100)
add_subdirectory(HP3000)
# Honeywell
add_subdirectory(H316)
# Intel-based computers
add_subdirectory(Intel-Systems)
# Interdata / Perkin-Elmer
add_subdirectory(Interdata)
# International Business Machines (IBM)
add_subdirectory(Ibm1130)
#add_subdirectory(I650)
add_subdirectory(I1401)
add_subdirectory(I1620)
add_subdirectory(I7000)
add_subdirectory(I7094)
add_subdirectory(S3)
# Manchester University
add_subdirectory(SSEM)
# Massachusetts Institute of Technology (MIT)
add_subdirectory(TX-0)
# Micro Instrumentation and Telemetry Systems (MITS)
add_subdirectory(ALTAIR)
add_subdirectory(AltairZ80)
# Royal MacBee
add_subdirectory(LGP)
# Sage Technology
add_subdirectory(SAGE)
# Scientific Data Systems (SDS)
add_subdirectory(SDS)
add_subdirectory(sigma)
# Soviet union
add_subdirectory(BESM6)
# Southwest Technical Products
add_subdirectory(swtp6800)
