# Build `simh` using CMake

<!-- TOC -->

- [Build `simh` using CMake](#build-simh-using-cmake)
    - [Why CMake?](#why-cmake)
    - [Quickstart For The Impatient](#quickstart-for-the-impatient)
        - [Linux/WSL](#linuxwsl)
        - [Windows](#windows)
        - [Running Simulators](#running-simulators)
    - [Details](#details)
        - [Tools and Runtime Library Dependencies](#tools-and-runtime-library-dependencies)
        - [CMake Configuration Options](#cmake-configuration-options)
        - [Directory Structure/Layout](#directory-structurelayout)
        - [Regenerating the simulator `CMakeLists.txt` Files](#regenerating-the-simulator-cmakeliststxt-files)
        - [Step-by-Step](#step-by-step)
            - [Linux/WSL/Ubuntu Step-by-Step](#linuxwslubuntu-step-by-step)
            - [Visual Studio Step-by-Step](#visual-studio-step-by-step)
            - [MinGW/GCC Step-by-Step](#mingwgcc-step-by-step)
    - [Motivation](#motivation)

<!-- /TOC -->


## Why CMake?

[CMake][cmake] is a cross-platform "meta" build system that provides similar functionality to
GNU _autotools_ within a more integrated and platform-agnostic framework. Two of the motivating
factors for providing a [CMake][cmake]-based alternative build system were library dependency
management (i.e., SDL2, SDL2_ttf, et. al.) and support for other build tools. A sample of
[CMake][cmake]'s supported build tools/environments includes:

  - Unix (GNU) Makefiles
  - [MinGW Makefiles][mingw64]
  - [Ninja][ninja] build tool
  - MS Visual Studio solutions (2015, 2017, 2019)
  - IDE build wrappers, such as [Sublime Text](https://www.sublimetext.com)
    and [CodeBlocks](http://www.codeblocks.org)

[CMake][cmake] (aka `simh-cmake`) is not intended to supplant, supercede or replace the
existing `simh` build infrastructure. If you like the existing `makefile` "poor man's
configure" approach, there's nothing to stop you from using it. [CMake][cmake] is a parallel
build system to the `simh` `makefile` and is just another way to build `simh`'s simulators. If
you look under the hood, you'll find a Python script that generates [CMake][cmake]'s
`CMakeLists.txt` files automagically from the `simh` `makefile`.


## Quickstart For The Impatient

The quickstart consists of:

  - Clone the `simh` repository, if you haven't done that already
  - Install runtime dependency development libraries (Linux)
  - Run the appropriate `cmake-builder` script for your platform

There are two scripts, `cmake/cmake-builder.ps1` (Windows PowerShell) and
`cmake/cmake-builder.sh` (_bash_), that automate the entire `simh-cmake` build sequence. This
sequence consists of four phases: _configure/generate_, _build_, _test_ and _install_. If the
build succeeds (and you see a lot of `-- Installing ` lines at the end of the output), you
should have a complete set of simulators under the top-level `simh/BIN` subdirectory.

On Windows (and potentially on Linux if you haven't installed the dependency development
libraries), `simh-cmake` will download, compile and locally install the runtime dependencies.
There is no support (yet) for Microsoft's [vcpkg][vcpkg]; it is planned at a future date. The
runtime dependency library build, followed by the simulator build, is called a [CMake][cmake]
"superbuild". The superbuild should only execute once; `simh-cmake` will regenerate the build
tool's files once the runtime dependency libraries are successfully detected and found.

### Linux/WSL

The quickstart shell steps below were tested on Ubuntu 18, both natively and under Windows
Services for Linux (WSL). Package names, such as `libsdl2-ttf`, may vary between Linux
distributions. For other ideas or to see how SIMH is built on [Gitlab][gitlab], refer to the
`.gitlab-ci.yml` configuration file.

The default [CMake][cmake] generator is "Unix Makefiles", i.e., [CMake][cmake] generates
everything for GNU `make`. [Ninja][ninja] is an alternate supported builder, which can be used
by passing the `--flavor=ninja` option to the `cmake/cmake-builder.sh` script.

```shell
## clone the SIMH repository (skip if you did this already)
$ git clone  https://github.com/simh/simh.git simh
$ cd simh

## Install development dependency libraries (skip if these are)
## already installed. It doesn't hurt to make sure that they are
## installed.)
$ sudo apt-get update -qq -y && \
  sudo apt-get install -qq -y cmake libpcre2-8-0 libpcre2-dev libsdl2-ttf-dev zlib1g-dev && \
  sudo apt-get install -qq -y libpcap-dev libvdeplug-dev

## Install the Ninja builder if desired, otherwise, skip this step.
$ sudo apt-get install -qq -y ninja-build

## Invoke the automation script for Unix Makefiles. Lots of output ensues.
$ cmake/cmake-builder.sh

## For Ninja (lighter weight and highly parallel compared to GNU make):
$ cmake/cmake-builder.sh --flavor=ninja

## Help from the script:
$ cmake/cmake-builder.sh --help
Configure and build simh simulators on Linux and *nix-like platforms.

Subdirectories:
cmake/build-unix:  Makefile-based build simulators
cmake/build-ninja: Ninja build-based simulators

Options:
--------
--clean (-x)      Remove the build subdirectory
--generate (-g)   Generate the build environment, don't compile/build
--parallel (-p)   Enable build parallelism (parallel builds)
--nonetwork       Build simulators without network support
--notest          Do not execute 'ctest' test cases
--noinstall       Do not install SIMH simulators.
--testonly        Do not build, execute the 'ctest' test cases
--installonly     Do not build, install the SIMH simulators
--allInOne        Use 'all-in-one' project structure (vs. individual)

--flavor (-f)     Specifies the build flavor: 'unix' or 'ninja'
--config (-c)     Specifies the build configuraiton: 'Release' or 'Debug'

--help (-h)       Print this help.
```


### Windows

These quickstart steps were tested using Visual Studio 2019 and using [MinGW][mingw64]/GCC. These
have not been tested with the [CLang][clang] compiler.

Building on Windows is slightly more complicated than building on Linux because development
package management is not as well standardized as compared to Linux distributions. The
following quickstart steps use the [Scoop][scoop] package manager to install required
development tools.

Runtime dependencies, such as `SDL2`, `PNG`, `zlib`, are automagically handled by the
`simh-cmake` build process, also known as "the superbuild." The superbuild should only occur
once to compile and locally install the runtime dependency libraries, header files and DLLs.
Once the runtime dependency libraries are installed, the superbuild regenerates the build
environment to use the locally installed dependencies.

To make things even more complicated, there are a variety of [CMake][cmake] genenerators,
depending on which compiler you have installed and whether you are using [MinGW][mingw64] or
Visual Studio. If you are using [MinGW][mingw64], __do not use `pkg-config` to install `SDL2`
and `SDL2-ttf`__ -- let `simh-cmake` compile and locally install those two dependencies. There
is an active ticket with Kitware to resolve this issue.

__Emulated Ethernet via Packet Capture (PCAP):__ If you want emulated Ethernet support in
Windows-based simulators, you __have__ to install the [WinPCAP][winpcap] package. __Note__:
The follow-on project, [NPCAP](https://nmap.org/npcap/), does not support packet reinjection
whereas [WinPCAP][winpcap] does. [NPCAP](https://nmap.org/npcap/) cannot be used (at the moment)
with `SIMH`. While there are no open CERT or CVE advisories and [WinPCAP][winpcap] still
operates under Windows(tm) 10, [WinPCAP][winpcap] may not be a viable choice in certain
configuration-controlled environments.

```shell
# If you have Visual Studio installed, skip the following two development
# tool installation steps.
#
# Install development tools using Scoop (Chocolatey is another option, package
# names are different, YMMV).
PS> scoop install 7zip cmake gcc winflexbison git msys2
# If you want/need the Ninja build system (an alternative to mingw32-make):
PS> scoop install ninja

# Clone the SIMH repository, if needed.
PS> git clone  https://github.com/simh/simh.git simh
PS> cd simh

# Choose one of the following build steps, depending on which development
# environment you use/installed:

# Visual Studio 2019 build, Release configuration. Simulators and runtime
# dependencies will install into the simh/BIN/Win32/Release directory.
PS> cmake\cmake-builder.ps1 -flavor vs2019

# MinGW/GCC build, Release build type. Simulators and runtime
# dependencies will install into the simh/BIN directory.
PS> cmake\cmake-builder.ps1 -flavor mingw

# Ninja/GCC build, Release build type. Simulators and runtime
# dependencies will install into the simh/BIN directory.
PS> cmake\cmake-builder.ps1 -flavor ninja

## Help output from the script:
PS> cmake\cmake-builder.ps1 -help
Configure and build simh's dependencies and simulators using the Microsoft
Visual Studio C compiler or MinGW-W64-based gcc compiler.

cmake/build-vs* subdirectories: MSVC build products and artifacts
cmake/build-mingw subdirectory: MinGW-W64 products and artifacts
cmake/build-ninja subdirectory: Ninja builder products and artifacts

Arguments:
-clean                 Remove and recreate the build subdirectory before
                       configuring and building
-generate              Generate build environment, do not compile/build.
                       (Useful for generating MSVC solutions, then compile/-
                       build within the Visual Studio IDE.)
-parallel              Enable build parallelism (parallel target builds)
-nonetwork             Build simulators without network support.
-notest                Do not run 'ctest' test cases.
-noinstall             Do not install simulator executables
-installOnly           Only execute the simulator executable installation
                       phase.
-allInOne              Use the simh_makefile.cmake "all-in-one" project
                       configuration vs. individual CMakeList.txt projects.
                       (default: off)

-flavor (2019|vs2019)  Generate build environment for Visual Studio 2019 (default)
-flavor (2017|vs2017)  Generate build environment for Visual Studio 2017
-flavor (2015|vs2015)  Generate build environment for Visual Studio 2015
-flavor (2013|vs2013)  Generate build environment for Visual Studio 2013
-flavor (2012|vs2012)  Generate build environment for Visual Studio 2012
-flavor mingw          Generate build environment for MinGW GCC/mingw32-make
-flavor ninja          Generate build environment for MinGW GCC/ninja

-config Release        Build dependencies and simulators for Release (optimized) (default)
-config Debug          Build dependencies and simulators for Debug

-help                  Output this help.
```

### Running Simulators

Assuming that the `cmake-builder.ps1` or `cmake-builder.sh` script successfully completed and installed
the simulators, running a simulator is very straightforward:

```shell
## Linux and MinGW/GCC (anyone except VS...)
$ BIN/vax

MicroVAX 3900 simulator V4.0-0 Current        git commit id: ad9fce56
sim>

## Visual Studio:
PS> BIN\Win32\Release\vax

MicroVAX 3900 simulator V4.0-0 Current        git commit id: ad9fce56
sim>
```


## Details

### Tools and Runtime Library Dependencies

The table below lists the development tools and packages needed to build the
`simh` simulators, with corresponding `apt`, `rpm` and `Scoop` package names,
where available. Blank names indicate that the package is not offered via the
respective package manager.

| Prerequisite             | Category   | `apt` package   | `rpm` package  | [Scoop][scoop] package | Notes  |
| ------------------------ | ---------- | --------------- | -------------- | ---------------------- | :----: |
| [CMake][cmake]           | Dev. tool  | cmake           | cmake          | cmake                  | (1)    |
| [Git][gitscm]            | Dev. tool  | git             | git            | git                    | (1, 2) |
| [bison][bison]           | Dev. tool  | bison           | bison          | winflexbison           | (2, 3) |
| [flex][flex]             | Dev. tool  | flex            | flex           | winflexbison           | (2, 3) |
| [WinPCAP][winpcap]       | Runtime    |                 |                |                        | (4)    |
| [zlib][zlib]             | Dependency | zlib1g-dev      | zlib-devel     |                        | (5)    |
| [pcre2][pcre2]           | Dependency | libpcre2-dev    | pcre2-devel    |                        | (5)    |
| [libpng][libpng]         | Dependency | libpng-dev      | libpng-devel   |                        | (5)    |
| [FreeType][FreeType]     | Dependency | libfreetype-dev | freetype-devel |                        | (5)    |
| [libpcap][libpcap]       | Dependency | libpcap-dev     | libpcap-devel  |                        | (5)    |
| [SDL2][SDL2]             | Dependency | libsdl2-dev     | SDL2-devel     |                        | (5)    |
| [SDL_ttf][SDL2_ttf]      | Dependency | libsdl2-ttf-dev |                |                        | (5)    |
| [pthreads4w][pthreads4w] | Dependency |                 |                |                        | (6)    |

_Notes_:

(1) Required development tool.
(2) Tool might already be installed on your system.
(3) Tool might already be installed in Linux and Unix systems; [winflexbison][winflexbison] is
    package that installs both the [bison][bison] and [flex][flex] tools for Windows developers.
    [bison][bison] and [flex][flex] are _only_ required to compile the [libpcap][libpcap] packet
    capture library. If you do not need emulated native Ethernet networking, [bison][bison] and
    [flex][flex] are optional.
(4) [WinPCAP][winpcap] is a Windows packet capture device driver. It is a runtime requirement used
    by simulators that emulate native Ethernet networking on Windows.
(5) If the package name is blank or you do not have the package installed, `simh-cmake` will
    download and compile the library dependency from source. [Scoop][scoop] does not provide these
    development library dependencies, so all dependencies will be built from source on Windows.
    Similarly, `SDL_ttf` support may not be available for RPM package-based systems (RedHat,
    CentOS, ArchLinux, ...), but will be compiled from source.
(6) [pthreads4w][pthreads4w] provides the POSIX `pthreads` API using a native Windows
    implementation. This dependency is built only when using the _Visual Studio_ compiler.
    [Mingw-w64][mingw64] provides a `pthreads` library as a part of the `gcc` compiler toolchain.


### CMake Configuration Options

The default `simh-cmake` configuration is _"Batteries Included"_: all options are enabled as
noted below. They generally mirror those in the `makefile`:

* `WITH_NETWORK`: Enable (=1)/disable (=0) simulator networking support. (def: enabled)
* `WITH_PCAP`: Enable (=1)/disable (=0) libpcap (packet capture) support. (def: enabled)
* `WITH_SLIRP`: Enable (=1)/disable (=0) SLIRP network support. (def: enabled)
* `WITH_VDE`: Enable (=1)/disable (=0) VDE2/VDE4 network support. (def: enabled, Linux only)
* `WITH_TAP`: Enable (=1)/disable (=0) TAP/TUN device network support. (def: enabled, Linux only)
* `WITH_VIDEO`: Enable (=1)/disable (=0) simulator display and graphics support (def: enabled)
* `PANDA_LIGHTS`: Enable (=1)/disable (=0) KA-10/KI-11 simulator's Panda display. (def: disabled)
* `DONT_USE_ROMS`: Enable (=1)/disable (=0) building hardcoded support ROMs. (def: disabled)
* `ENABLE_CPPCHECK`: Enable (=1)/disable (=0) [cppcheck][cppcheck] static code checking rules.
* `ALL_IN_ONE`: Use the 'all-in-one' project definitions (vs. individual CMakeLists.txt) (def: disabled)

Specify configuration options during the `simh-cmake` configuration/generation step (see the
[step-by-step](#step-by-step) section). For example, if your build environment is on a
Ubuntu/Linux/WSL system using the Ninja build system:

```` shell
# Do configuration/generation in the cmake/build-ninja subdirectory
$ cd cmake/build-ninja
# Clean out prior CMake configuration
$ rm -rf CMakeCache.txt CMakefiles
# Don't want networking enabled:
$ cmake -G Ninja -DWITH_NETWORK=Off -DCMAKE_BUILD_TYPE=Release ../..
````

A Windows Visual Studio 2019 environment would reconfigure by executing the following:

```` shell
# Do configuration in the cmake\build-vs2019 subdirectory
PS> cd cmake\build-vs2019
# Clear out prior CMake configuration
PS> remove-item -force CMakeCache.txt
PS> remove-item -force -recurse CMakeFiles
# Dont'want video enabled:
PS> cmake -G "Visual Studio 16 2019" -A Win32 -DWITH_VIDEO=Off ..\..
````

__ENABLE_CPPCHECK__: This is a developer option that enables additional rules that runs the
[cppcheck](cppcheck) static code analysis tool over individual simulator code. The
[cppcheck](cppcheck) rules have to be executed manually and have the naming convention
`<simulator>-cppcheck`:

    ```` shell
    # Run cppcheck on the PDP-8 simulator's source code:
    $ ninja pdp8-cppcheck
    ````

__ALL_IN_ONE project definitions__: This option is discouraged, and should never be used by
Visual Studio developers. The "all-in-one" project definitions are a variation on how
`simh-cmake` configures the individual simulators. For Visual Studio developers and IDE
enthusiasts for whom individual simulator projects are important, this option will make all
simulator suprojects disappear and likely make your IDE unusable or unsuitable to compile SIMH.
If your normal development environment is Emacs or Vi/ViM, IDE subprojects aren't important and
this option is available to you. It doesn't improve compile or build times.


### Directory Structure/Layout

`simh-cmake` is relatively self-contained within the `cmake` subdirectory. The exceptions are
the `simh` top-level directory `CMakeLists.txt` file and the individual simulator
`CMakeLists.txt` files. The individual simulator `CMakeLists.txt` files are automagically
generated from the `simh` `makefile` by the `cmake/generate.py` script -- see [this
section](#regenerating-the-cmakeliststxt-files) for more information.

The directory structure and layout is:

```
simh
| - CMakeLists.txt         ## Top-level, "master" project defs for SIMH and
|                          ## simulators
|
+ 3b2                      ## 3b2 simulator
| - CMakeLists.txt         ## Individual subproject CMake defs for 3b2
| - ...                    ## 3b2 sources
+ ALTAIR                   ## Altair simulator
| - CMakeLists.txt         ## Individual subproject CMake defs for Altair
| - ...                    ## Altair sources
+ AltairZ80
| - CMakeLists.txt         ## Individual subproject CMake defs for AltairZ80
| - ...                    ## AltairZ80 sources
+ BIN                      ## SIMH's preferred installation location
| + Win32/Release          ## Visual Studio Release configuration installation loc.
| + Win32/Debug            ## Visual Studio Debug configuration installation loc.
...
+ cmake
| + build-vs2019           ## Subdirectory for VS 2019 build artifacts
| + build-mingw            ## Subdirectory for MinGW build artifacts
| + build-ninja            ## Subdirectory for Ninja build artifacts
| + build-unix             ## Subdirectory for GNU make build artifacts
| + dependencies           ## Local installation subdirectory hierarchy for
|                          ## runtime dependencies
| - *.cmake                ## CMake packages and modules
| - generate.py            ## CMakeLists.txt generator script
```

Git will ignore all of the `cmake/build-*` and `cmake/dependencies` subdirectories. These
subdirectories encapsulate all of the build artifacts and products associated with a particular
compiler/build environment. It is actually safe to delete these subdirectories if you want to
start over from scratch. Moreover, you can also develop and target multiple compilers, since
their [CMake][cmake] outputs are compartmentalized.

### Regenerating the simulator `CMakeLists.txt` Files

`simh-cmake` is a parallel build system to the `makefile`. The `makefile` is the authoritative
source for simulators that can be built (the `all :` rule) as well as how those simulators
compile and options needed to compile them. Consequently, the individual simulator
`CMakeLists.txt` project definitions are automagically generated from the `makefile` using the
`cmake/generate.py` Python3 script.

__Simulator developers:__ If you add a new simulator to SIMH or change a simulator's source
file list, compile options, etc., you will have to regenerate the `CMakeLists.txt` files. It's
relatively painless and Git will detect which have changed to minimize the amount of repository
churn:

```shell
$ cd cmake
$ python3 -m generate
```

That's it. Basically, if you change `makefile`, regenerate.

There is nothing that prevent you from manually editing a `CMakeLists.txt` file while doing
development (see the individual `CMakeLists.txt` files and `cmake/add_simulator.cmake` for
information on how to define your own simulator, as well as available options.) Just keep in
mind that the `makefile` is authoritative and all of your changes will be wiped out by the next
`CMakeList.txt` regeneration.

### Step-by-Step

This section steps you through how to work with `simh-cmake` without the `cmake-builder.ps1` or
`cmake-builder.sh` scripts. It also covers some of the interesting and frequently used
[CMake][cmake] configuration options.

The `cmake-builder.ps1` and `cmake-builder.sh` scripts execute the following four steps:

1. Create a subdirectory in which you will build the simulators.
 
     `simh-cmake` **will not allow you** to configure, build or compile `simh` in the source tree.
     An informal convention for build subdirectories is `cmake/build-<something>`, such as
     `cmake-unix` for Unix Makefile builds and `cmake-ninja` for [Ninja][ninja]-based builds. Git
     (`.gitignore`) ignores all subdirectories that start with `cmake/build-*`.)

2. Configure and generate for your environment

    You should only have to execute this step once. [CMake][cmake] will inject the necessary
    dependencies into your build environment (`Makefile`, `build.ninja`, VS solution or project
    file) to reconfigure if you change a `CMakeLists.txt` file. 
    
    If you change any of the `cmake/*.cmake` files that detect OS features and runtime
    dependencies, or need to change configuration or options, **make sure to delete** the
    `CMakeCache.txt` cache and the `CMakeFiles` subdirectory first:

    ````shell
    # Assuming that you are building using Ninja and need to reconfigure:
    $ cd cmake/build-ninja
    $ rm -rf CMakeCache.txt CMakeFiles
    ````

    Configuration settings worth tweaking include:

    __CMAKE_BUILD_TYPE__: Commonly used values are `Release` and `Debug`. Some generators support
    `MinSizeRel`, but don't count on it -- use `Release` or `Debug`. If not otherwise specified,
    `simh-cmake` defaults to `Release`. This controls the compiler flags used in environments that
    do not support multiple build configurations (everything that isn't Visual Studio.) This has
    to be specified at configuration/generation.

    __CMAKE_INSTALL_PREFIX__: Directory where simulators will be installed. Defaults to
    `${CMAKE_SOURCE_DIR}/BIN`.

3. Compile dependencies and simulators

    Usually this is straightforward, i.e., `make`, `mingw32-make` or `ninja`. For Visual Studio,
    you're better off with invoking `cmake` to run the build. Note that invoking `cmake` to run
    the build works for all build environments and tools.

4. Run tests

    This is an optional, but highly recommended step. Runs all of the VAX tests, and the DEC PDP-8
    and AT&T 3b2 simulator tests.

5. Install

    If the build and tests succeed, then install the simulators. For Windows developers, the
    install step copies over runtime dependency library DLLs, which save you from editing/changing
    your `PATH` environment variable.

#### Linux/WSL/Ubuntu Step-by-Step

Putting this all together for a Ubuntu/WSL development environment using the [Ninja][ninja]
builder. __NOTE__: These steps assume that you have installed the prerequisite development
libraries. See [the Linux/WSL quickstart](#linuxwsl) for the `apt-get` lines related to these
prerequisites.

``` shell
# Configure/generate.
#
# Make a build directory and generate Ninja build rules. We are going to ask
# Ninja to build a Release (optimized) set of simulators (CMAKE_BUIlD_TYPE=Release).
$ mkdir -p cmake/build-ninja
$ cd cmake/build-ninja
$ cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
-- The C compiler identification is GNU 9.2.0
-- The CXX compiler identification is GNU 9.2.0
-- Check for working C compiler: /usr/bin/cc
-- Check for working C compiler: /usr/bin/cc -- works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting C compile features
-- Detecting C compile features - done
-- Check for working CXX compiler: /usr/local/bin/c++
-- Check for working CXX compiler: /usr/local/bin/c++ -- works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- CMAKE_BUILD_TYPE is Release
-- Found PkgConfig: /usr/bin/pkg-config (found version "0.29")
... And a lots more output ...

# Now build. You could also just execute "ninja" here too.
$ cmake --build .
... Lots of output building the simulators ...

# Run the tests
$ ctest
Test project /nowhere/special/simh/cmake/build-ninja
      Start  1: test-3b2
 1/23 Test  #1: test-3b2 .........................   Passed   51.16 sec
      Start  2: test-pdp8
 2/23 Test  #2: test-pdp8 ........................   Passed   16.24 sec
      Start  3: test-infoserver100
 3/23 Test  #3: test-infoserver100 ...............   Passed    1.05 sec
      Start  4: test-infoserver1000
... Output from the remaining 20 tests ...
100% tests passed, 0 tests failed out of 23
Total Test time (real) = 248.27 sec

# And now install to simh/BIN.
$ cmake --install .
```

#### Visual Studio Step-by-Step

First make sure that you have installed the development tools needed to build the runtime
dependency libraries before executing the step-by-step instructions below. Refer to the `scoop`
script lines in [the Windows quickstart](#windows) for more information.

These step-by-step instructions assume that you are using Visual Studio 2019. If you use an
earlier version of Visual Studio, choose a [CMake][cmake] generator as noted in the instructions.

``` shell
# Make a VS 2019 build directory. The actual name of the directory is not important --
# "cmake/build-vs2019" is merely a convention, vice "cmake/build-vs2015"
# for VS 2015.
PS> New-Item -Path cmake\build-vs2019 -ItemType Directory
PS> cd cmake\build-vs2019

# Configure and generate the VS solution:
# VS 2019 for 32-bit x86:
PS> cmake -G "Visual Studio 16 2019" -A Win32 ..\..
# VS 2017:
# PS> cmake -G "Visual Studio 15 2017" ..\..
# VS 2015:
# PS> cmake -G "Visual Studio 14 2015" ..\..

# Once cmake finishes configurating and generating, there will be a "simh.sln" VS
# solution file in the cmake\build-vs2019 directory:
PS> dir simh.sln


    Directory: ...\cmake\build-vs2019


Mode                LastWriteTime         Length Name
----                -------------         ------ ----
-a----        2/13/2020  11:03 PM         107996 simh.sln


# Now build... You could also open the solution file in VS and build from
# there. Also, you can
$ cmake --build . --config Release
### Lots of output ###

# Run the tests
$ ctest -C Release
Test project /nowhere/special/simh/cmake/build-ninja
      Start  1: test-3b2
 1/23 Test  #1: test-3b2 .........................   Passed   51.16 sec
      Start  2: test-pdp8
 2/23 Test  #2: test-pdp8 ........................   Passed   16.24 sec
      Start  3: test-infoserver100
 ... Output from the remaining 21 tests ...
100% tests passed, 0 tests failed out of 23
Total Test time (real) = 248.27 sec

# And now install to simh/BIN.
$ cmake --install . --config Release
```

#### MinGW/GCC Step-by-Step

The MinGW/GCC steps are similar to the [Unix/WSL/Unix steps](#linuxwslubuntu-step-by-step), using
PowerShell instead of the standard Unix _bash_ shell. You can follow the
[Unix/WSL/Unix steps](#linuxwslubuntu-step-by-step) from inside the MinGW `mintty` console with
_bash_ (assuming you have installed `cmake`, `bison`, `flex` and the other required development
tools.)

As previously noted in the [Windows quickstart](#windows), do not use `pacman` to install `SDL2`
and `SDL2_ttf`. Let `simh-cmake` build them as runtime dependencies.

``` shell
# Configure/generate.
#
# Make a build directory and generate a MinGW Makefile. We are going to ask
# mingw32-make to build a Release (optimized) set of simulators (CMAKE_BUIlD_TYPE=Release).
$ mkdir -p cmake/build-mingw
$ cd cmake/build-mingw
$ cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
... Configuration output ...

# Now build. You could also just execute "mingw32-make" here too.
$ cmake --build .
... Lots of output building the simulators ...

# Run the tests
$ ctest
Test project /nowhere/special/simh/cmake/build-mingw
      Start  1: test-3b2
 1/23 Test  #1: test-3b2 .........................   Passed   51.16 sec
      Start  2: test-pdp8
 2/23 Test  #2: test-pdp8 ........................   Passed   16.24 sec
      Start  3: test-infoserver100
 3/23 Test  #3: test-infoserver100 ...............   Passed    1.05 sec
      Start  4: test-infoserver1000
... Output from the remaining 20 tests ...
100% tests passed, 0 tests failed out of 23
Total Test time (real) = 248.27 sec

# And now install to simh/BIN.
$ cmake --install .
```

## Motivation

**Note**: This is personal opinion. There are many personal opinions like this,
but this one is mine. (With apologies to the USMC "Rifle Creed.")

`simh` is a difficult package to build from scratch, especially if you're on a Windows
platform. There's a separate directory tree you have to check out that has to sit parallel to
the main `simh` codebase (aka `sim-master`.) It's an approach that I've used in the past and
it's hard to maintain (viz. a recent commit log entry that says that `git` forks should _not_
fork the `windows-build` subdirectory.) That's not a particularly clean or intuitive way of
building software. It's also prone to errors and doesn't lend itself to upgrading dependency
libraries.

[cmake]: https://cmake.org
[cppcheck]: http://cppcheck.sourceforge.net/
[ninja]: https://ninja-build.org/
[scoop]: https://scoop.sh/
[gitscm]: https://git-scm.com/
[bison]: https://www.gnu.org/software/bison/
[flex]: https://github.com/westes/flex
[winpcap]: https://www.winpcap.org/
[zlib]: https://www.zlib.net
[pcre2]: https://pcre.org
[libpng]: http://www.libpng.org/pub/png/libpng.html
[FreeType]: https://www.freetype.org/
[libpcap]: https://www.tcpdump.org/
[SDL2]: https://www.libsdl.org/
[SDL2_ttf]: https://www.libsdl.org/projects/SDL_ttf/
[mingw64]: https://mingw-w64.org/
[winflexbison]: https://github.com/lexxmark/winflexbison
[pthreads4w]: https://github.com/jwinarske/pthreads4w
[chocolatey]: https://chocolatey.org/
[vcpkg]: https://github.com/Microsoft/vcpkg
[gitlab]: https://gitlab.com/scooter-phd/simh-cmake
[clang]: https://clang.llvm.org/
