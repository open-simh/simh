cmake_minimum_required(VERSION 3.7.0)

project(simh)

# Do not pollute source directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/BIN)

# Options for initial configuration only
set(BUILD_ALL OFF CACHE BOOL "Initial default: build all simulators")

# Normal options that can be changed
set(SIMH_ENABLE_ASYNC ON CACHE BOOL "Enable asynchronous I/O support")
set(SIMH_ENABLE_GRAPHICS ON CACHE BOOL "Initial default: enable graphics support")
set(SIMH_ENABLE_NETWORK ON CACHE BOOL "Initial default: enable network support")
set(SIMH_EXECUTABLE_PREFIX "" CACHE STRING "Prefix for simulator executables")

# Use folders for IDE generators
set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)

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
if(UNIX)
  # Find pkg-config dependencies
  find_package(PkgConfig)
  pkg_check_modules(PCRE libpcre)
  pkg_check_modules(PCREPOSIX libpcreposix)
  if(SIMH_ENABLE_GRAPHICS)
    pkg_check_modules(SDL1 sdl)
    pkg_check_modules(SDL2 sdl2)
  endif()
  if(SIMH_ENABLE_NETWORK)
    pkg_check_modules(VDEPLUG vdeplug)
  endif()
else()
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

set(SIMH_DIR         ${CMAKE_SOURCE_DIR})
set(SIMH_DEFINITIONS _GNU_SOURCE HAVE_GLOB)
set(SIMH_INCLUDES    ${SIMH_DIR})
set(SIMH_LIBRARIES   ${LIBM_LIBRARIES} ${THREAD_LIBRARIES})
if(WIN32)
  # Don't warn about POSIX functions being deprecated.
  # Don't warn about unsafe C functions.
  list(APPEND SIMH_DEFINITIONS _CRT_NONSTDC_NO_WARNINGS _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS)
  list(APPEND SIMH_LIBRARIES   PThreads4W::PThreads4W Winmm.lib ws2_32.lib)
endif()
# The sim_* sources need to be built with each simulator as they have features
# enabled by conditional compilation.  To improve build times this should be
# factored out into the compile-time variants so that the built code can be
# shared between targets.
set(SIMH_SOURCES
  ${SIMH_DIR}/scp.h
  ${SIMH_DIR}/scp.c
  ${SIMH_DIR}/sim_console.h
  ${SIMH_DIR}/sim_console.c
  ${SIMH_DIR}/sim_fio.h
  ${SIMH_DIR}/sim_fio.c
  ${SIMH_DIR}/sim_timer.h
  ${SIMH_DIR}/sim_timer.c
  ${SIMH_DIR}/sim_tmxr.h
  ${SIMH_DIR}/sim_tmxr.c
  ${SIMH_DIR}/sim_tape.h
  ${SIMH_DIR}/sim_tape.c
  ${SIMH_DIR}/sim_disk.h
  ${SIMH_DIR}/sim_disk.c
  ${SIMH_DIR}/sim_serial.h
  ${SIMH_DIR}/sim_serial.c
  ${SIMH_DIR}/sim_video.h
  ${SIMH_DIR}/sim_video.c
  ${SIMH_DIR}/sim_imd.h
  ${SIMH_DIR}/sim_imd.c
  ${SIMH_DIR}/sim_card.h
  ${SIMH_DIR}/sim_card.c
  ${SIMH_DIR}/scp_help.h
  ${SIMH_DIR}/sim_defs.h
  ${SIMH_DIR}/sim_rev.h
  ${SIMH_DIR}/sim_ether.h
  ${SIMH_DIR}/sim_ether.c
  ${SIMH_DIR}/sim_sock.h
  ${SIMH_DIR}/sim_sock.c
)
set(SIMH_FRONTPANEL_SOURCES
  ${SIMH_DIR}/sim_frontpanel.h
  ${SIMH_DIR}/sim_frontpanel.c
)

# Asynchronous I/O
if(SIMH_ENABLE_ASYNC)
  list(APPEND SIMH_DEFINITIONS SIM_ASYNCH_IO)
endif()

# Dynamic linker
if(LIBDL_LIBRARIES)
  string(SUBSTRING ${CMAKE_SHARED_LIBRARY_SUFFIX} 1 -1 LIBDL_SUFFIX)
  list(APPEND SIMH_DEFINITIONS HAVE_DLOPEN=${LIBDL_SUFFIX})
  list(APPEND SIMH_LIBRARIES   ${LIBDL_LIBRARIES})
endif()

# Shared memory support
if(LIBRT_LIBRARIES)
  list(APPEND SIMH_DEFINITIONS HAVE_SHM_OPEN)
  list(APPEND SIMH_LIBRARIES   ${LIBRT_LIBRARIES})
endif()

# Regular expression support
if(PCRE_FOUND AND PCREPOSIX_FOUND)
  list(APPEND SIMH_DEFINITIONS HAVE_PCRE_H HAVE_PCREPOSIX_H)
  list(APPEND SIM_INCLUDES     ${PCRE_INCLUDE_DIRS} ${PCREPOSIX_INCLUDE_DIRS})
  list(APPEND SIMH_LIBRARIES   ${PCRE_LIBRARIES} ${PCREPOSIX_LIBRARIES})
else()
  list(APPEND SIMH_DEFINITIONS HAVE_REGEX_H)
endif()

# Curses support
if(CURSES_FOUND)
  list(APPEND SIMH_DEFINITIONS HAVE_NCURSES)
  list(APPEND SIM_INCLUDES     ${CURSES_INCLUDE_DIRS})
  list(APPEND SIMH_LIBRARIES   ${CURSES_LIBRARIES})
endif()

# Graphics support
if(SIMH_ENABLE_GRAPHICS)
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
    list(APPEND GRAPHICS_DEFINITIONS HAVE_LIBSDL)
	set(GRAPHICS_CFLAGS    ${SDL2_CFLAGS})
	set(GRAPHICS_INCLUDES  ${SDL2_INCLUDE_DIRS})
	set(GRAPHICS_LIBRARIES ${SDL2_LIBRARIES})
  elseif(SDL1_FOUND)
    list(APPEND GRAPHICS_DEFINITIONS HAVE_LIBSDL)
	set(GRAPHICS_CFLAGS    ${SDL1_CFLAGS})
	set(GRAPHICS_INCLUDES  ${SDL1_INCLUDE_DIRS})
	set(GRAPHICS_LIBRARIES ${SDL1_LIBRARIES})
  endif()
endif()

# Network support
if(SIMH_ENABLE_NETWORK)
  set(NETWORK_DEFINITIONS USE_NETWORK)
  if(VDEPLUG_FOUND)
	list(APPEND NETWORK_DEFINITIONS HAVE_VDE_NETWORK)
	list(APPEND NETWORK_INCLUDES    ${VDEPLUG_INCLUDE_DIRS})
	list(APPEND NETWORK_LIBRARIES   ${VDEPLUG_LIBRARIES})
  endif()
endif()

# simulator SCSI support source files
set(SIMH_SCSI ${SIMH_DIR}/sim_scsi.h ${SIMH_DIR}/sim_scsi.c)

# Utility to generate headers for inclusion of binary ROMs in simulators.
add_executable(BuildROMs sim_BuildROMs.c)
set_property(TARGET BuildROMs PROPERTY FOLDER "Support")
if(WIN32)
  # Suppress warnings about unsafe standard library functions.
  target_compile_definitions(BuildROMs PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

# Convenience function for defining simulators
#
# Example:
#   add_simulator(NAME foo SCSI DEFINITIONS USE_FOO SOURCES foo.h foo.c)
#
# Required arguments: NAME, SOURCES
#
# Keyword arguments:
#   DISABLED        Do not create a target for this simulator.
#   DISPLAY         Build this simulator with display support.
#   EXPERIMENTAL    Do not build this simulator as part of 'all'.
#   NETWORK         Build this simulator with network support.
#   ROMS            This simulator references boot ROMs.
#   SCSI            Build this simulator with SCSI support.
#   UNSUPPORTED     This simulator is unsupported or incomplete; it will
#                   not be built as part of 'all'.
#
# Single value arguments:
#   NAME            (Required) Specifies the name of the simulator executable.
#
# Multiple value arguments:
#   DEFINITIONS     Specify additional defines when building source files.
#   INCLUDES        Specify additional include directories to search.
#   SOURCES         (Required) Specify source files to build.
#   LIBRARIES       Specify additional libraries to link against.
#   ALIASES         Specify names of simulators that are just copies of this simulator.
#
function(add_simulator2)
  cmake_parse_arguments(PARSE_ARGV 0 SIMULATOR "DISABLED;DISPLAY;EXPERIMENTAL;NETWORK;ROMS;SCSI;UNSUPPORTED" "NAME" "ALIASES;DEFINITIONS;INCLUDES;SOURCES;LIBRARIES")
  if(NOT SIMULATOR_NAME)
    message(FATAL_ERROR "No required NAME argument to add_simulator.")
  endif()
  if(NOT SIMULATOR_SOURCES)
    message(FATAL_ERROR "Simulator ${SIMULATOR_NAME} doesn't specify any source files")
  endif()
  if(SIMULATOR_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unknown arguments '${SIMULATOR_UNPARSED_ARGUMENTS}'")
  endif()
  if(SIMULATOR_DISABLED)
    # Disabled simulators have code that doesn't compile without modification.
    message(STATUS "Simulator ${SIMULATOR_NAME} is disabled.")
    return()
  endif()

  if(SIMULATOR_EXPERIMENTAL OR SIMULATOR_UNSUPPORTED)
    set(ENABLED FALSE)
  else()
    set(ENABLED TRUE)
  endif()
  string(TOUPPER "SIMH_${SIMULATOR_NAME}" NAMEU)
  string(REPLACE "-" "_" NAMEU ${NAMEU})
  set(${NAMEU}_ENABLE ${ENABLED} CACHE BOOL "Build the ${SIMULATOR_NAME} simulator")

  add_executable(${SIMULATOR_NAME} ${SIMULATOR_SOURCES} ${SIMH_SOURCES})
  target_compile_definitions(${SIMULATOR_NAME} PUBLIC ${SIMULATOR_DEFINITIONS} ${SIMH_DEFINITIONS})
  target_include_directories(${SIMULATOR_NAME} PUBLIC
    ${SIMULATOR_INCLUDES}
    ${SIMH_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    )
  target_link_libraries(${SIMULATOR_NAME} PUBLIC ${SIMULATOR_LIBRARIES} ${SIMH_LIBRARIES})
  install(TARGETS ${SIMULATOR_NAME} RUNTIME DESTINATION bin)
  if(SIMULATOR_ROMS)
    add_dependencies(${SIMULATOR_NAME} BuildROMs)
  endif()
  if(SIMULATOR_SCSI)
    target_sources(${SIMULATOR_NAME} PUBLIC ${SIMH_SCSI})
  endif()
  if(SIMULATOR_EXPERIMENTAL)
    message(STATUS "Simulator ${SIMULATOR_NAME} is experimental.")
    set_property(TARGET ${SIMULATOR_NAME} PROPERTY EXCLUDE_FROM_ALL TRUE)
    set_property(TARGET ${SIMULATOR_NAME} PROPERTY FOLDER "Simulators/Experimental")
  endif()
  if(SIMULATOR_UNSUPPORTED)
    message(STATUS "Simulator ${SIMULATOR_NAME} is unsupported/incomplete.")
    set_property(TARGET ${SIMULATOR_NAME} PROPERTY EXCLUDE_FROM_ALL TRUE)
    set_property(TARGET ${SIMULATOR_NAME} PROPERTY FOLDER "Simulators/Unsupported")
  endif()
  if(NOT (SIMULATOR_EXPERIMENTAL OR SIMULATOR_UNSUPPORTED))
    set_property(TARGET ${SIMULATOR_NAME} PROPERTY FOLDER "Simulators")
  endif()
  if(SIMULATOR_NETWORK)
    set(${NAMEU}_NETWORK ${SIMH_ENABLE_NETWORK} CACHE BOOL "Enable ${SIMULATOR_NAME} network support")
    if(SIMH_ENABLE_NETWORK)
      target_compile_definitions(${SIMULATOR_NAME} PUBLIC ${NETWORK_DEFINITIONS})
      target_include_directories(${SIMULATOR_NAME} PUBLIC ${NETWORK_INCLUDES})
      target_link_libraries(${SIMULATOR_NAME} PUBLIC ${NETWORK_LIBRARIES})
    endif()
  endif()
  # TODO
  if(SIMULATOR_DISPLAY)
    set(${NAMEU}_DISPLAY FALSE CACHE BOOL "Enable ${SIMULATOR_NAME} display support")
  endif()
  foreach(alias ${SIMULATOR_ALIASES})
    add_custom_command(TARGET ${SIMULATOR_NAME}
      POST_BUILD
      COMMENT "Copying ${SIMULATOR_NAME}${CMAKE_EXECUTABLE_SUFFIX} to ${alias}${CMAKE_EXECUTABLE_SUFFIX}"
      COMMAND cmake -E copy_if_different $<TARGET_FILE:${SIMULATOR_NAME}> $<TARGET_FILE_DIR:${SIMULATOR_NAME}>/${alias}${CMAKE_EXECUTABLE_SUFFIX}
    )
  endforeach()
endfunction()

# Function for adding graphics support
function(add_simulator_graphics NAME)
  string(TOUPPER ${NAME} NAMEU)
  if(SIMH_ENABLE_GRAPHICS)
	set(SIMULATOR_${NAMEU}_GRAPHICS ${SIMH_ENABLE_GRAPHICS} CACHE BOOL "Enable ${NAME} graphics support")
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
  if(SIMH_ENABLE_GRAPHICS)
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
  if(SIMH_ENABLE_NETWORK)
	set(SIMULATOR_${NAMEU}_NETWORK ${SIMH_ENABLE_NETWORK} CACHE BOOL "Enable ${NAME} network support")
	if(BUILD_ALL OR SIMULATOR_${NAMEU}_ENABLE)
	  if(SIMULATOR_${NAMEU}_NETWORK)
	    target_link_libraries(${NAME} PUBLIC ${NETWORK_LIBRARIES})
	    target_compile_definitions(${NAME} PUBLIC ${NETWORK_DEFINITIONS})
	    target_include_directories(${NAME} PUBLIC ${NETWORK_INCLUDES})
	  endif()
    endif()
  endif()
endfunction()

# Add the subdirectories for all the machine simulators.
#
# Please keep this alphabetized by case INsensitive directory name.
add_subdirectory(3B2)
add_subdirectory(alpha)
add_subdirectory(ALTAIR)
add_subdirectory(AltairZ80)
add_subdirectory(B5500)
add_subdirectory(BESM6)
add_subdirectory(CDC1700)
add_subdirectory(GRI)
add_subdirectory(H316)
add_subdirectory(HP2100)
add_subdirectory(HP3000)
add_subdirectory(I1401)
add_subdirectory(I1620)
add_subdirectory(I650)
add_subdirectory(I7000)
add_subdirectory(I7094)
add_subdirectory(Ibm1130)
add_subdirectory(imlac)
add_subdirectory(Intel-Systems)
add_subdirectory(Interdata)
add_subdirectory(LGP)
add_subdirectory(NOVA)
add_subdirectory(PDP1)
add_subdirectory(PDP10)
add_subdirectory(PDP11)
add_subdirectory(PDP18B)
add_subdirectory(PDP8)
add_subdirectory(PDQ-3)
add_subdirectory(S3)
add_subdirectory(SAGE)
add_subdirectory(SDS)
add_subdirectory(SEL32)
add_subdirectory(sigma)
add_subdirectory(SSEM)
add_subdirectory(swtp6800)
add_subdirectory(tt2500)
add_subdirectory(TX-0)
add_subdirectory(VAX)
