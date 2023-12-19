# Build `simh` using CMake


- [Build `simh` using CMake](#build-simh-using-cmake)
  - [Why CMake?](#why-cmake)
  - [Before You Begin Building...](#before-you-begin-building)
    - [Toolchains and Tools](#toolchains-and-tools)
      - [Ninja: "failed recompaction: Permission denied"](#ninja-file-recompaction-permission-denied)
      - [Windows XP-compatible/Server 2003 binaries](#windows-xp-compatibleserver-2003-binaries)
    - [Feature Libraries](#feature-libraries)
      - [Linux, macOS and MinGW-w64](#linux-macos-and-mingw-w64)
      - [Windows: "Legacy" superbuild or `vcpkg`](#windows-legacy-superbuild-or-vcpkg)
    - [CMake Directory Structure](#cmake-directory-structure)
  - [Building the simulators](#building-the-simulators)
    - [CMake Builder Scripts](#cmake-builder-scripts)
    - [CMake Command Line](#cmake-command-line)
      - [Create a build directory](#create-a-build-directory)
      - [Linux/Unix/Unix-like/macOS/MinGW-w64 walkthrough](#linuxunixunix-likemacosmingw-w64-walkthrough)
      - [Windows PowerShell walkthrough](#windows-powershell-walkthrough)
      - [Changing the Compiler](#changing-the-compiler)
      - [Configuration Options](#configuration-options)
    - [`RELEASE_LTO` static code checks](#release_lto-static-code-checks)
    - [`cppcheck` static code checks](#cppcheck-static-code-checks)
    - [Visual Studio-Only Development](#visual-studio-only-development)
      - [Quirks](#quirks)
      - [VS 2022 and 2019 Walkthrough](#vs-2022-and-2019-walkthrough)
      - [XP-compatible Build via the VS2019 and VS2022 IDEs](#xp-compatible-build-via-the-vs2019-and-vs2022-ides)
  - [`CMake` Generators](#cmake-generators)
  - [Simulator Developer Notes](#simulator-developer-notes)
    - [How to compile a simulator: `add_simulator`](#how-to-compile-a-simulator-add_simulator)
    - [Simulator "core" libraries](#simulator-core-libraries)
  - [`add_simulator` Reference](#add_simulator-reference)
    - [Adding a new simulator](#adding-a-new-simulator)
    - [Regenerating `CMakeLists.txt` from the `makefile`](#regenerating-cmakeliststxt-from-the-makefile)


## Why CMake?

[CMake][cmake] is a cross-platform meta-build system that provides similar
functionality to GNU _autotools_ within a more integrated and platform-agnostic
framework. A sample of the supported build environments include:

  - Unix Makefiles
  - [MinGW Makefiles][mingw64]
  - [Ninja][ninja]
  - macOS Xcode
  - MS Visual Studio solutions (2015, 2017, 2019, 2022)
  - IDE build wrappers ([Sublime Text][sublime] and [CodeBlocks][codeblocks])

Making the Windows build process less complex by automatically downloading,
building and installing dependency feature libraries, and consistent cross
platform support were the initial motivations behind a [CMake][cmake]-based
build infrastructure. Since then, that motivation expanded to supporting a wider
variety of platforms and compiler combinations, streamlining the overall compile
process, enhanced IDE integration and SIMH packaging.

## Before You Begin Building...

### Toolchains and Tools

Before you begin building the simulators, you need the following:

- A C/C++ compiler toolchain.

  - _GNU C Compiler (gcc)_: `gcc` is the default compiler for Linux and
    Unix/Unix-like platforms. It can also be used for [Mingw-w64][mingw64]-based
    builds on Windows.

  - _Microsoft Visual C/C++_: Visual Studio 2022, 2019, 2017 and 2015 are
    supported. The [appveyor CI/CD][appveyor] pipeline builds using these four
    Microsoft toolchains in _Release_ and _Debug_ configurations.

    - _VS 2022_: The Community Edition can be downloaded from the
      [Microsoft Visual Studio Community][vs_community] page.

    - _VS 2019 Community Edition, VS 2017 and 2015_:
      [Microsoft's older Visual Studio Releases page][older_vs_community]
      hosts the installers for these previous releases.

  - _CLang/LLVM_: `clang` is the default compiler on MacOS. To use `clang` on
    Linux/Unix-like operating systems, [CMake][cmake] needs to be invoked
    manually (see [here](#cmake-command-line).)

  Success reports for additional platforms not listed are happily accepted along
  with patches to the [CMake][cmake] infrastructure to ensure future support.

- [CMake][cmake] version 3.14 or newer.

  - Linux: Install `cmake` using your distribution's package manager (e.g. `apt`,
    `pacman`, `rpm`, ...)

      apt: `sudo apt install cmake cmake-data`

      pacman: `sudo pacman install cmake`

  - macOS: Install `cmake` using your preferred external package management
    system, such as [Homebrew][homebrew] or [MacPorts][macports].

      Homebrew: `brew install cmake extra-cmake-modules`

      MacPorts: `sudo port install cmake`

  - Windows:

    - _Visual Studio IDE, Developer command or PowerShell console windows_: No
      additional software installation needed. Microsoft provides `cmake` that
      can be invoked from the command prompt or from within the VS IDE.
      Microsoft has bundled various version of `cmake` into Visual Studio since
      VS 2015.

    - Otherwise, install `cmake` using your preferred Windows software package
      manager, such as [Chocolatey][chocolatey] or [Scoop][scoop]. You can also
      [download and install the `cmake` binary distribution][cmake_downloads]
      directly.

- The [Git source control system][gitscm].

  - Linux: Install `git` using your distribution's package manager (e.g. `apt`,
    `pacman`, `rpm`, ...)

      apt: `sudo apt install git`

      pacman: `sudo pacman install git`

  - macOS: Install `git` using your preferred external package management
    system, such as [Homebrew][homebrew] or [MacPorts][macports]

      Homebrew: `brew install git`

      MacPorts: `sudo port install git`

  - Windows:

    [Git][gitscm] is needed to apply patches to [dependency feature
    libraries](#feature-libraries) when building those libraries.

    - _Visual Studio IDE_: [Git][gitscm] is available via the Visual Studio IDE.
      If you do all of your simulator development from within the VS IDE, no
      additional software needs to be installed.

    - _Visual Studio Developer command or PowerShell console windows_: Unlike
      `cmake`, `git`'s location is not added to `PATH` or `$env:PATH`. Use the
      VS IDE for `git`-related tasks (add, commit, branch, push, pull, etc.)

    -  Otherwise, install `git` using your preferred Windows software package
       manager, such as [Chocolatey][chocolatey] or [Scoop][scoop]. You can also
       [download and install the `git` client][gitscm_downloads] directly.

- GNU Make:

  - Required for Linux and macOS. Consult your appropriate package manager to
    install `make` if it is not already installed.

  - MinGW-w64 uses a version of GNU Make named `mingw32-make`. See the MinGW-W64
    notes below under "Feature Libraries."

- Ninja:

  [Ninja][ninja] is an optional, but useful/faster parallel build alternative to
  Unix Makefiles and Visual Studio's `msbuild`.


#### Ninja: "failed recompaction: Permission denied"

This is a long-standing issue with [Ninja][ninja] on Windows when `ninja` is
recursively invoked. You are very likely to encounter this error message when
you build dependency feature libraries from a [builder script](#cmake-builder-scripts),
[`cmake` on the command line](#cmake-command-line)  and
[from inside the Visual Studio IDE](#visual-studio-only-development).

This error message will halt the build process. It is easy to work around by
restarting the build process.

#### Windows XP-compatible/Server 2003 binaries

Microsoft has deprecated XP tool support and it WILL eventually disappear in a
future Visual Studio release. Windows XP itself is well beyond End of Lifetime
support, with extended support having ended on April 8, 2014.

You should only install this esoterica if you absolutely have a driving need to
do so. The Continuous Integration/Continuous Deployment (CI/CD) pipelines will
produce XP-compatible binaries. If you need XP-compatible binaries, use the
CI/CD artifacts.

If you absolutely, positively need to build XP-compatible binaries, the steps
for VS 2022 and VS 2019 are essentially the same. If you use the [CMake command line](#cmake-command-line),
you will have to ensure that you add `-A Win32 -T v141_xp` to `cmake`'s configuration arguments
to select a 32-bit target architecture and the `v141_xp` toolkit.

_VS2022_: Install the `v141_xp` tools

Start the Visual Studio Installer, whether this is a new VS2022 install or
modifying an existing installation.

- New install
  - In the "Workloads" pane, check "Desktop development with C++" workload's
    checkbox, if not already checked.
  - Click on the item labeled "Individual components"
  - In the "Individual components" pane:
    - Enter "XP" in the "Search components (Ctrl-Q)" field.
    - Locate the "Compilers, build tools and runtimes" heading
    - Select "C++ for Windows XP Support for VS 2017 (v141) tools [Deprecated]" checkbox.
  - Continue to customize your VS 2022 installation as needed.
  - Click on "Install" in the lower right hand corner

- Modifying an existing VS2022 installation
  - Click on the Visual Studio 2022 `Modify` button.
  - In the "Modifying --" window, click on "Individual Components"
  - Click on the item labeled "Individual components"
  - In the "Individual components" pane:
    - Enter "XP" in the "Search components (Ctrl-Q)" field.
    - Locate the "Compilers, build tools and runtimes" heading
    - Select "C++ for Windows XP Support for VS 2017 (v141) tools [Deprecated]" checkbox.
  - Continue to customize your VS 2022 installation as needed.
  - Click on the "Modify" button in the lower right corner of the Window.

VS 2022 can build XP-compatible binaries via the [`cmake-builder` scripts](#cmake-builder-scripts),
the [command line](#cmake-command-line) or [via the IDE](#xp-compatible-build-via-the-vs2019-and-vs2022-ides).

_VS2019_: Install the `v141_xp` tools.

Start the Visual Studio Installer, whether this is a new VS2019 install or
modifying an existing installation.

- New installation: Follow the VS 2022 "New install" instructions. The steps are
  the same.

- Modifying an existing VS 2019 installation:
  - Click on the Visual Studio 2019 `Modify` button.
  - Follow the remaining VS 2022 modification steps, starting with the "In the
    'Modifying --' window, ..." step.
  - Instead of "Continue to customize your VS 2022 installation as needed", continue
    to customize your VS 2019 installation as needed.

VS 2019 can build XP-compatible binaries via the [`cmake-builder` scripts](#cmake-builder-scripts),
the [command line](#cmake-command-line) or [via the IDE](#xp-compatible-build-via-the-vs2019-and-vs2022-ides).

_VS2017_: There are two requirements that need to be satisfied:

- `CMake` version 3.14 or higher. The `CMake` distributed with VS 2017 is
  version 3.12.

  - You will need to install a newer version of `CMake` (see above
    for download links.)
  - You will also need to ensure that the directory to the updated `cmake.exe` is on
    the front of your `PATH` (`cmd`) or `env:PATH` (`PowerShell`). If you are
    unsure what this means, do not proceed further.

- Ensure that the `v141_xp` toolkit is installed. Installation instructions
  can be found [by following this link to the Stackoverflow question's solution.][v141_xp].

VS 2017 can build XP-compatible binaries via the [`cmake-builder` scripts](#cmake-builder-scripts) or
the [command line](#cmake-command-line).

### Feature Libraries

All SIMH features are enabled by default. `CMake` only disables features
when the underlying support headers and libraries are not detected or the
feature is specifically disabled at `cmake` configuration time.

Available features are PCAP, TUN/TAP and VDE networking, SDL2 graphics, SDL2
TrueType font rendering, and simulator window snapshot support. Note that some
of these features are platform specific. For example, TUN/TAP and VDE networking
are not available on Windows platforms, whereas `cmake` tries to detect the
TUN/TAP header file on Linux and macOS, and VDE is an optionally installed
package on Linux and macOS.

#### Linux, macOS and MinGW-w64

[Github Actions][open_simh_actions] and [appveyor][appveyor] CI/CD pipelines
execute the `.travis/deps.sh` script to install these feature libraries on Linux
and macOS. `.travis/deps.sh` can also install the requisite toolchains and
feature libraries for and MinGW-64 Win64 native and Universal C Runtime (UCRT)
binaries.

  - Linux apt-based distributions (e.g., Debian, Ubuntu):

    ```bash
    $ sudo sh .travis/deps.sh linux
    ```

  - macOS Homebrew:

    ```bash
    $ sudo sh .travis/deps.sh osx
    ```

  - macOS MacPorts:

    ```bash
    $ sudo sh .travis/deps.sh macports
    ```

  - MinGW-w64 Win64 console:

    ```bash
    $ echo $MSYSTEM
    MINGW64
    $ .travis/deps.sh mingw64
    ```

  - MinGW-w64 UCRT console:

    ```bash
    $ echo $MSYSTEM
    UCRT64
    $ .travis/deps.sh ucrt64
    ```

#### Windows: "Legacy" superbuild or `vcpkg`

The SIMH CMake infrastructure has two distinct feature library dependency
strategies: the _"legacy"_ superbuild and `vcpkg`. The principal differences
between the two strategies are:

  1. _"legacy"_ can produce Windows XP-compatible executables.

  2. `vcpkg` has robust compiler support for MS Visual Studio compilers. Using
     GCC or Clang with vcpkg is a work-in-progress.

  3. `vcpkg` has a larger open source ecosystem and better long term support
     outlook[^1].

  4. _"legacy"_ installs the minimal dependency features necessary to avoid
     becoming its own "ports" system (which is what `vcpkg` provides.) For
     example, _"legacy"_ does not install `bzip2` as a `libpng` subdependency,
     which limits the compression methods available to `libpng` when capturing
     screenshots.

  5. `vcpkg` installs more subdependencies, potentially increasing
     functionality. Continuing `libpng` as the example, `vcpkg` will install
     `bzip2` as a subdependency, which adds compression methods to `libpng` when
     capturing simulator screenshots. `vcpkg` also installs the Harfbuzz text
     shaper as a Freetype subdependency.

  6. _"legacy"_ compiles the dependency libraries as part of the overall
     compile/build process.

  7. `vcpkg` compiles and installs dependencies during the CMake configuration
     step, which makes the configuration process longer.

Setup and Usage:

- _"legacy"_ superbuild

    This is the default dependency feature library build strategy. It will
    download, compile and install the minimal feature libraries necessary to
    support the SIMH simulators: `zlib`, `libpng`, `pcre` (version 1, not
    PCRE2), `freetype`, `SDL2` and `SDL_ttf`. The CMake configuration process
    generates a _superbuild_ that installs the dependencies under the
    `cmake/dependencies` subdirectory tree. Once the dependency feature
    libraries finish building successfully, the _superbuild_ invokes CMake to
    reconfigure SIMH to use these newly installed dependencies.

- [`vcpkg`](https://vcpkg.io)

  Simply set the `VCPKG_ROOT` environment variable to use the `vcpkg` strategy.
  `vcpkg` operates in [Manifest mode][vcpkg_manifest]; refer to the `vcpkg.json`
  manifest file.

  The default platform triplets for  the Visual Studio compilers are
  `x86-windows-static` and `x64-windows-static`, depending on the architecture
  flag passed to CMake.

  The `x64-mingw-dynamic` triplet is known to work from within a MinGW-w64
  console/terminal window using the GCC compiler.

  If you haven't git-cloned `vcpkg`, `git clone vcpkg` somewhere outside of the
  SIMH source tree. For example, you could choose to clone `vcpkg` in the
  directory above `open-simh`:

        ```powershell
        PS C:\...\open-simh> pwd
        C:\...\open-simh
        PS C:\...\open-simh> cd ..
        PS C:\...> git clone https://github.com/Microsoft/vcpkg.git
        PS C:\...> cd vcpkg
        PS C:\...\vcpkg> .\vcpkg\bootstrap-vcpkg.bat
        PS C:\...\vcpkg> cd ..\open-simh
        PS C:\...\open-simh>
        ```
  Then set the `VCPKG_ROOT` environment variable to the `vcpkg` installation directory.

[^1]: `vcpkg` does not support the `v141_xp` toolkit required to compile Windows
XP binaries. Windows XP is a target platform that SIMH can hopefully deprecate
in the future. For the time being, Windows XP is a target platform that is part
of the CI/CD pipelines and requires the _"legacy"_ superbuild strategy.


### CMake Directory Structure

The directory structure below is a guide to where to find things and where to
look for things, such as simulator executables.

```
simh                      # Top-level SIMH source directory
+-- CMakeLists.txt        # Top-level CMake configuration file
+-- BIN                   # Simulator executables (note 1)
|   +-- Debug
|   +-- Release
|   +-- Win32
|       +-- Debug
|       +-- Release
+-- cmake                 # CMake modules and build subdirectories
|   +-- build-vs2022      # Build directory for VS-2022 (note 2)
|   +-- build-vs2019      # Build directory for VS-2019 (note 2)
|   +-- build-vs2017      # Build directory for VS-2017 (note 2)
|   +-- build-vs2017-xp   # Build directory for VS-2017 v141_xp toolkit (note 2)
|   +-- build-vs2015      # Build directory for VS-2015 (note 2)
|   +-- build-unix        # Build directory for Unix Makefiles (note 2)
|   +-- build-ninja       # Build directory for Ninja builder (note 2)
|   +-- dependencies      # Install subdirectory for Windows dependency libraries
|   |   +-- Windows-10-MSVC-19.34
|   |   |   |...          # Feature library subdirectory for Windows legacy
|   |   |   |...          # dependency superbuilds (note 3)
|...
+-- out                   # Visual Studio build directories (note 4)
|   +-- build
|   +-- install
+-- 3b2
|   +-- CMakeLists.txt    # 3b2 simulator CMake configuration file
+-- alpha
|   +-- CMakeLists.txt    # alpha simulator CMake configuration file
|...
+-- VAX
|   +--  CMakeLists.txt   # VAX simulators family CMake configuration file
```

Notes:
1. The `BIN` directory is where CMake directs the underlying build system to
   place SIMH simulator executables. The `BIN` directory's structure varies,
   depending on the underlying build system.
   - Single configuration builders (`make`, `ninja`): Simulators executables
     will appear directly underneath the `BIN` directory.
   - Multi-configuration builders (`msbuild`, `xcodebuild`): The simulator
     executables will appear underneath individual configuration subdirectories
     ("Debug" and "Release").
   - The Windows platform has its `Win32` subdirectory.
2. The `cmake-builder.ps1` and `cmake-builder.sh` scripts create the
   `cmake/build-*` subdirectories as needed.
3. `Windows-10-MSVC-19.34` is an example feature library subdirectory created as
   the installation area for VS 2022-built dependencies on Windows. Linux,
   macOS and MinGW-w64 do not build feature libraries because those platforms install
   feature libraries via their respective package managers.
4. The `out` subdirectory is the default build subdirectory hierarchy when
   directly building the simulator suite using the Visual Studio IDE.


## Building the simulators

The basic workflow for building the `simh` simulator suite is: configure, build,
test and install. There are two principal ways of executing this workflow: via
the `cmake/cmake-builder.sh` or `cmake/cmake-builder.ps1` scripts or manually
invoking `cmake` directly. For Visual Studio users, the simulators can be build
entirely from within the VS IDE.

### CMake Builder Scripts

Building the simulator suite via the build scripts is simply a matter of
following the appropriate script below. If you are a build-from-source SIMH
user, this is all you need to do. The [Github Actions][open_simh_actions] and
the [appveyor] CI/CD pipelines execute these scripts.

- Linux/Unix-lib/macOS/MinGW-w64:

    ```bash
    # Clone the open-simh repository, if not already done:
    $ git clone https://github.com/open-simh/simh.git
    $ cd simh

    # Install feature dependency libraries (use "osx", for HomeBrew or "macports" for MacPorts)
    # on macOS with HomeBrew.)
    $ sh .travis/deps.sh linux

    # Configure cmake to generate Unix Makefiles, compile the simulators
    # using the 'Release' configuration inside the cmake/build-unix build
    # directory:
    $ cmake/cmake-builder.sh
    ```

- Windows PowerShell:

    ```powershell
    # Clone the open-simh repository, if not already done:
    PS C:\...\open-simh> git clone https://github.com/open-simh/simh.git
    PS C:\...\open-simh> cd simh

    # VS 2022 default build in Release configuration.
    #
    # Build directory is cmake\build-vs2022. Will also perform a superbuild
    # the first time to build the dependency feature libraries for VS 2022:
    PS C:\...\open-simh> cmake\cmake-builder.ps1
    ```

When the scripts complete, the simulators will be copied in the top-level SIMH
`BIN` directory. Alternatively, you can navigate to the desired simulator
binary's subdirectory and execute the simulator from inside the `cmake/build-*`
subdirectories. Refer to the [directory structure](#cmake-directory-structure)
notes, above.

Both builder scripts provide options to change the build tool, configuration and
options. You should add the `--clean` flag to clean the build subdirectory
before rebuilding with a different configuration or when you change options
e.g., switching from a `Release` to a `Debug` build or building without network
or video support.

- Linux/Unix-lib/macOS/MinGW-w64:

    ```bash
    # Rebuild with the Debug configuration. Clean the Release configuration
    # out of the build directory before rebuilding:
    $ cmake/cmake-builder.sh --config Debug --clean

    # Use Ninja instead of Unix Makefiles in the Release configuration.
    # Build directory is cmake/build-ninja:
    $ cmake/cmake-builder.sh --flavor ninja

    # List the supported command line flags:
    $ cmake/cmake-builder.sh --help
    ** cmake version 3.18.4

    CMake suite maintained and supported by Kitware (kitware.com/cmake).
    Configure and build simh simulators on Linux and *nix-like platforms.

    Compile/Build options:
    ----------------------
    --clean (-x)      Remove the build subdirectory
    --generate (-g)   Generate the build environment, don't compile/build
    --parallel (-p)   Enable build parallelism (parallel builds)
    --notest          Do not execute 'ctest' test cases
    --noinstall       Do not install SIMH simulators.
    --testonly        Do not build, execute the 'ctest' test cases
    --installonly     Do not build, install the SIMH simulators

    --flavor (-f)     [Required] Specifies the build flavor. Valid flavors are:
                        unix
                        ninja
                        xcode
                        xcode-universal
                        msys
                        msys2
                        mingw
                        ucrt
    --config (-c)     Specifies the build configuration: 'Release' or 'Debug'

    --target          Build a specific simulator or simulators. Separate multiple
                      targets with a comma, e.g. "--target pdp8,pdp11,vax750,altairz80,3b2"
    --lto             Enable Link Time Optimization (LTO) in Release builds
    --debugWall       Enable maximal warnings in Debug builds
    --cppcheck        Enable cppcheck static code analysis rules

    --cpack_suffix    Specify CPack's packaging suffix, e.g., "ubuntu-22.04"
                      to produce the "simh-4.1.0-ubuntu-22.04.deb" Debian
                      package.

    --verbose         Turn on verbose build output

    SIMH feature control options:
    -----------------------------
    --nonetwork       Build simulators without network support
    --novideo         Build simulators without video support
    --no-aio-intrinsics
                      Do not use compiler/platform intrinsics to implement AIO
                      functions (aka "lock-free" AIO), reverts to lock-based AIO
                      if threading libraries are detected.

    Other options:
    --------------
    --help (-h)       Print this help.
    ```


- Windows PowerShell:

    ```powershell
    # Use VS 2017 toolchain, Debug configuration. Build directory is
    # cmake\build-vs2017. Will also perform a superbuild the first time
    # to build the dependency feature libraries for VS 2017:
    PS C:\...\open-simh> cmake\cmake-builder.ps1 -flavor vs2017 -config Debug

    # List the supported command line flags:
    PS C:\...\open-simh> Get-Help -deatailed cmake\cmake-builder.ps1

    NAME
        C:\...\play\open-simh\cmake\cmake-builder.ps1

    SYNOPSIS
        Configure and build SIMH's dependencies and simulators using the Microsoft Visual
        Studio C compiler or MinGW-W64-based gcc compiler.


    SYNTAX
        C:\...\play\open-simh\cmake\cmake-builder.ps1 [[-flavor] <String>] [[-config] <String>] [[-cpack_suffix] <String>] [[-target]
        <String[]>] [-clean] [-help] [-nonetwork] [-novideo] [-noaioinstrinsics] [-notest] [-noinstall] [-parallel] [-generate] [-testonly]
        [-installOnly] [-windeprecation] [-lto] [-debugWall] [-cppcheck] [<CommonParameters>]


    DESCRIPTION
        This script executes the three (3) phases of building the entire suite of SIMH
        simulators using the CMake meta-build tool. The phases are:

        1. Configure and generate the build environment selected by '-flavor' option.
        2. Build missing runtime dependencies and the simulator suite with the compiler
           configuration selected by the '-config' option. The "Release" configuration
           generates optimized executables; the "Debug" configuration generates
           development executables with debugger information.
        3. Test the simulators

        There is an install phase that can be invoked separately as part of the SIMH
        packaging process.

        The test and install phases can be enabled or disabled by the appropriate command line
        flag (e.g., '-noInstall', '-noTest', '-testOnly', '-installOnly'.)

        Build environment and artifact locations:
        -----------------------------------------
        cmake/build-vs*          MSVC build products and artifacts
        cmake/build-mingw        MinGW-W64 products and artifacts
        cmake/build-ninja        Ninja builder products and artifacts


    PARAMETERS
        -flavor <String>
            The build environment's "flavor" that determines which CMake generator is used
            to create all of the build machinery to compile the SIMH simulator suite
            and the target compiler.

            Supported flavors:
            ------------------
            vs2022          Visual Studio 2022 (default)
            vs2022-xp       Visual Studio 2022 XP compat
            vs2019          Visual Studio 2019
            vs2019-xp       Visual Studio 2019 XP compat
            vs2017          Visual Studio 2017
            vs2017-xp       Visual Studio 2017 XP compat
            vs2015          Visual Studio 2015
            mingw-make      MinGW GCC/mingw32-make
            mingw-ninja     MinGW GCC/ninja

        -config <String>
            The target build configuration. Valid values: "Release" and "Debug"

    [...truncated for brevity...]
    ```


### CMake Command Line

This section is targeted to simulator developers and build-from-source users who
want or need fine-grained control over the configure/build/test workflow.

The thumbnail view of this process for both Linux/macOS/MinGW-w64 and Windows
resembles:

1. Create a subdirectory where `cmake` will do its work (the build subdirectory.)

2. `cd` into your `cmake` build subdirectory.

3. Configure:
      ```sh
      cmake -G "<generator>" -DOPTION=VALUE -DOPTION=VALUE ... -S ${simh_source_directory}
      ```

4. Build:
      ```sh
      cmake --build . [build options...]
      ```

5. Test
      ```sh
      ctest [ctest options]
      ```

#### Create a build directory

Create a build directory that isn't the same as the top-level SIMH source directory:

```sh
# Clone the open-simh repository, if not already done:
$ git clone https://github.com/open-simh/simh.git
$ cd simh

# Create a subdirectory where you will build the simulators with
# CMake. .gitignore ignores all directories starting with "build-"
# under the cmake subdirectory (but you don't have to follow this
# convention)
$ mkdir cmake/build-mybuild
$ cd cmake/build-mybuild
```

The `CMakeLists.txt` driver will not allow you to configure in the SIMH
top-level source directory. This is a __feature__, not a bug. Separating the
build from the source directory makes it a lot easier to clean up after builds
(just recursively remove the build directory). If you try to configure in the
same directory as the source, you __will__ be reminded to create a separate
build directory._


#### Linux/Unix/Unix-like/macOS/MinGW-w64 walkthrough

The following walkthrough shows how to manually configure, build, test and
install the simulators from a Linux and macOS terminal window, and a MinGW-w64
shell console. This walkthrough has all features enabled (video, networking);
[configuration options](#configuration-options) are discussed separately.

```bash
# Should be in the cmake/build-mybuild subdirectory
$ pwd
/.../simh/cmake/build-mybuild

# Configure: Generate Unix Makefiles, all options enabled in the
# Release build configuration.
$ cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -S ../..

# Build all simulators
$ cmake --build .

# Run the tests with a 5 minute timeout per test (most tests only require 2
# minutes, SEL32 is a notable exception)
$ ctest --build-config Release --output-on-failure --timeout 300
```

Examples of other things you can do from the command line:

```sh
# To build a specific simulator, such as b5500, specify the
# target:
$ cmake --build . --target b5500

# Since cmake generated a Makefile and we're in the same directory as
# the Makefile, invoke make directly. This what the previous command line
# does.
$ make b5500

# Need to reconfigure for a Debug configuration (you just built a Release
# configuration above..)?
$ rm -rf CMakeCache.txt CMakeFiles
$ cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -S ../.. -B .

# Then build via cmake or make...
```

To build with Ninja as the CMake generator (or [any other available generator](#cmake-generators)),
from the `simh` top-level source directory:

```bash
$ mkdir cmake/build-ninja
$ cd cmake/build-ninja
$ cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -S ../.. -B .
$ cmake --build .

# Build a specific simulator
$ cmake --build . --target 3b2

# Build the 3b2 simulator using ninja if we're in the same subdirectory
# as build.ninja:
$ ninja 3b2

# Run the tests
$ ctest --build-config Release --output-on-failure --timeout 300
```

#### Windows PowerShell walkthrough

Windows follows the same pattern as the Linux/macOS/MinGW-w64 walkthrough with
minor changes. The most notable change is the `cmake` build system generator's
name. You will need to choose the generator that corresponds to your installed
Visual Studio version. You also have to specify the build configuration at
compile time when using Visual Studio and `msbuild`.

```powershell
# Create a Visual Studio build directory
PS> mkdir cmake/build-vstudio
PS> cd cmake/build-vstudio

# Choose one of the following that corresponds to your installed Visual Studio:
PS> cmake -G "Visual Studio 17 2022" -A Win32 -S ../..
PS> cmake -G "Visual Studio 16 2019" -A Win32 -S ../..
PS> cmake -G "Visual Studio 15 2017" -A Win32 -S ../..
PS> cmake -G "Visual Studio 14 2015" -A Win32 -S ../..
# If you insist on building XP-compatible binaries, use this configuration
# command line -- you must have the v141_xp toolkit installed.
PS> cmake -G "Visual Studio 17 2022" -A Win32 -T v141_xp -S ../..
PS> cmake -G "Visual Studio 16 2019" -A Win32 -T v141_xp -S ../..
PS> cmake -G "Visual Studio 15 2017" -A Win32 -T v141_xp -S ../..

# Build
PS> cmake --build . --config Release

# Test
PS> ctest --build-config Release --output-on-failure --timeout 300
```

The `cmake` Visual Studio generators create the solution file, which you
can open from within Visual Studio. In the above example, look for the `.sln`
file underneath the `cmake/build-vstudio` subdirectory.

If you have [Ninja][ninja] installed, you can the following as your `cmake`
configure command from inside a Visual Studio developer PowerShell console:

```powershell
# Configure.
#
# Note that Ninja is a single configuration build system, so you have to
# specify CMAKE_BUILD_TYPE at configuration time:
PS> cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -A Win32 -S ../..

# Build.
PS> ninja

# Test step is the same as above.
```

#### Changing the Compiler

The C and C++ compilers can be manually configured via the `CMAKE_C_COMPILER`
and `CMAKE_CXX_COMPILER` options on the command line. The following example uses
the [Ninja][ninja] build system and the `Clang` compilers:

```sh
## From inside your build directory:
$ cmake -G Ninja CMAKE_C_COMPILER=clang CMAKE_CXX_COMPILER=clang ...
```

#### Configuration Options

You can enable or disable SIMH simulator suite options on the `cmake`
configuration command line by specifying `-DOPTION=VALUE`. The configuration
options generally mirror those in the original `simh` `makefile`.

_NOTE:_ `CMake` __aggressively__ caches option values. If you change an option,
you should also remove the `CMakeCache.txt` file and recursively remove the
`CMakeFiles` subdirectory within your build directory. See the examples
following the table.

| Option               | Default            | Description |
| -------------------- | ------------------ | ----------- |
| `CMAKE_BUILD_TYPE`   |                    | [`CMake`-defined variable][cmake_build_type] that controls the build's configuration, typical values are _"Release"_ and _"Debug"_. This only needs to be set for single configuration tools, such as `make` and `ninja`. It does not have to be set for multi-configuration tools such as `msbuild` |
| `NO_DEP_BUILD`       | platform-specific  | Build dependency libraries on Windows (def: enabled), disabled on Linux/macOS, MinGW-w64. |
| `BUILD_SHARED_DEPS`  | platform-specific  | Build dependencies as shared libraries/DLLs on Windows. Does nothing on Linux/macOS. Disabled by default on Windows to ensure that the simulators link against static libraries. |
| `WITH_ASYNC`         | enabled            | Asynchronous I/O and threading support. |
| `WITH_REGEX`         | enabled            | PCRE regular expression support. |
| `WITH_NETWORK`       | enabled            | Simulator networking support. `WITH_PCAP`, `WITH_SLIRP`, `WITH_VDE` and `WITH_TAP` only have meaning if `WITH_NETWORK` is enabled. |
| `WITH_PCAP`          | enabled            | libpcap (packet capture) support. |
| `WITH_SLIRP`         | enabled            | SLIRP UDP network support. |
| `WITH_VDE`           | enabled            | VDE2/VDE4 network support. |
| `WITH_TAP`           | enabled            | TAP/TUN device network support. |
| `WITH_VIDEO`         | enabled            | Simulator display and graphics support |
| `PANDA_LIGHTS`       | disabled           | KA-10/KI-11 simulator's Panda display. |
| `DONT_USE_ROMS`      | disabled           | Do not build support ROM header files (i.e., embed the simulator's boot ROMs in the simulator executable.) |
| `ENABLE_CPPCHECK`    | disabled           | `cppcheck` static code analysis support. |
| `WINAPI_DEPRECATION` | disabled           | Show (enable) or mute (disable) WinAPI deprecation warnings. |
| `WARNINGS_FATAL`     | disabled           | Compiler warnings are fatal errors, e.g. set "-Werror" on `gcc`, "/WX" for MSVC |
| `RELEASE_LTO`        | disabled           | Use Link-Time Optimization in Release builds, where supported. Normally disabled; the CI/CD builds turn this on to catch additional warnings emitted with higher optimization and LTO. |
| `DEBUG_WALL`         | disabled           | Turn on maximal warnings for Debug builds, e.g., `-Wall` for GCC/Clang and `/W4` for MSVC. |

The following table summarizes "enabled" and "disabled" option values on the command line:

| Option   | `CMake` value |
| -------- | ------------- |
| enabled  | "On", "1" or "True" |
| disabled | "Off", "0" or "False |

- Linux/macOS/MinGW-w64 example:

    ```bash
    # Remove the CMakeCache.txt file and CMakeFiles subdirectory if you are
    # reconfiguring:
    $ rm -rf CMakeCache.txt CMakeFiles/

    # Then (re)configure:
    $ cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DWITH_NETWORK=Off -DENABLE_CPPCHECK=Off -S ../..

    # Alteratively, "0" and "Off" are equivalent, as are "1" and "On".
    $ cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DWITH_NETWORK=0 -DENABLE_CPPCHECK=0 -S ../..
    ```

- Windows Powershell example (VS 2022, turn off networking and enable WinAPI deprecation warnings)

    ```powershell
    # Remove the CMakeCache.txt file and CMakeFiles subdirectory if you are
    # reconfiguring:
    PS> remove-item -recurse -force CMakeCache.txt CMakeFiles/

    # Then (re)configure:
    PS> cmake -G "Visual Studio 17 2022" -A Win32 -DWITH_NETWORK=Off -DWINAPI_DEPRECATION=True -S ../..
    ```


### `RELEASE_LTO` static code checks

_GCC_ and _Clang_ _Release_ builds use the `-O2` optimization level unless the
`RELEASE_LTO` configuration option is set to `True` (or `On` or `1`), e.g.:

```sh
$ cmake -G Ninja -DRELEASE_LTO:Bool=True ...
```

Or using the builder scripts:

```sh
$ cmake-builder.sh --flavor unix --lto ...
```
```powershell
PS> cake-builder.ps1 -flavor mingw-unix -lto ...
```

Setting `RELEASE_LTO` to `True` does two things:

  1. It changes the optimization level to `-O3` and turns on link-time
     optimization (`-flto`).
  2. It will also turn all compiler warnings into errors (`-Werror`).

The net effect of turning on `-O3`, `-flto` and `-Werror` is additional static
code checking and any LTO-emitted warnings are fatal compilation errors.

`RELEASE_LTO` is the default optimization for the appveyor and Github Actions
 CI/CD builds. Any warnings will cause these CI/CD builds to fail and must be
 corrected before the code is accepted in the SIMH main branch.


### `cppcheck` static code checks

[Cppcheck][cppcheck] is a robust, open source static code checking tool that
provides more in-depth code style analysis to complement the `RELEASE_LTO`
approach. If the `ENABLE_CPPCHECK` option is `True` or `On` and the `cppcheck`
executable is detected on your `PATH`, you will be able to use the `cppcheck`
rule to run the checker over ALL of SIMH's source:

```sh
## Execute from inside your build directory.
## (note: the command is the same for Windows)
$ cmake --build . --target cppcheck
```

This will execute ALL of the static code analysis across ALL simulators and ALL
SIMH core libraries. This will result in VOLUMINOUS output.

Add the simulator or [core library](#simulator-core-libraries) name to
`_cppcheck` to cppcheck an individual simulator or core library, such as
`pdp8_cppcheck` or `simhcore_cppcheck`.

The `cppcheck` rule is not currently executed by a CI/CD build. It is an
optional build rule intended to improve overall code quality over the long term.

### Visual Studio-Only Development

Microsoft packages `cmake` and `git` in several versions of Visual Studio, which
can be accessed from the IDE and build the SIMH simulator suite solely from
within the IDE. The walkthrough provides directions for VS 2022 and VS 2019.


#### Quirks

- Visual Studio uses the `ninja` build tool by default -- not `msbuild`. You can
  change this setting via the "CMake Settings" pane.

- The Visual Studio CMake build places the intermediate build products and
  artifacts in an `out\build` subdirectory relative to the `simh` top level
  source directory. Refer to the [directory structure](#cmake-directory-structure)
  section.

-  Visual Studio installs the simulator executables under the
   `out\install\<Configuration>\bin`, where `<Configuration>` is the current
   Visual Studio configuration. This differs from where the CMake build would
   normally install executables in a top-level source directory named `BIN`.

- The initial configuration created by Visual Studio is `x64-Debug`. You will
  need to add configurations, such as `x64-Release` or `x86-Debug`, to suit
  your needs. The walkthrough will show you how to add the `x64-Release`
  configuration.


#### VS 2022 and 2019 Walkthrough

1. Start VS 2022 or VS 2019 and choose "Clone a repository".

     - The repository location is [https://github.com/open-simh/simh.git](https://github.com/open-simh/simh.git)

     - Path: The directory here you want the repository to be cloned. This is the
       source directory.

2. When Visual Studio finishes cloning the repository:

     - Choose `Open>CMake...` from the `File` menu. Open the `CMakeLists.txt` file
       in the source directory where `git` just checked out SIMH's source code.

     - The _Output_ pane should switch to "CMake" and display CMake's
       configuration output. It should look similar to the following:

       ```
       1> CMake generation started for default configuration: 'x64-Debug'.
       1> Command line: "C:\WINDOWS\system32\cmd.exe" /c [... long command line ...]
       1> Working directory: C:\...\open-simh-github\out\build\x64-Debug
       1> [CMake] -- The C compiler identification is MSVC 19.33.31630.0
       1> [CMake] -- The CXX compiler identification is MSVC 19.33.31630.0
       1> [CMake] -- Detecting C compiler ABI info
       1> [CMake] -- Detecting C compiler ABI info - done
       1> [CMake] -- Check for working C compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.33.31629/bin/Hostx64/x64/cl.exe - skipped
       1> [CMake] -- Detecting C compile features
       1> [CMake] -- Detecting C compile features - done
       1> [CMake] -- Detecting CXX compiler ABI info
       1> [CMake] -- Detecting CXX compiler ABI info - done
       1> [CMake] -- Check for working CXX compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.33.31629/bin/Hostx64/x64/cl.exe - skipped
       1> [CMake] -- Detecting CXX compile features
       1> [CMake] -- Detecting CXX compile features - done
       1> [CMake] -- CMAKE_BUILD_TYPE is Debug
       1> [CMake] -- CMAKE_MODULE_PATH: C:/.../open-simh-github/cmake
       1> [CMake] -- Setting NO_DEP_BUILD to FALSE, will BUILD missing dependencies
       1> [CMake] -- Creating dependency library directory hierarchy

       [... snipped for brevity ...]

       1> [CMake] --
       1> [CMake] -- Configuring done
       1> [CMake] -- Generating done
       1> [CMake] -- Build files have been written to: C:/Users/bsm21317/play/open-simh-gitlab/out/build/x64-Debug
       1> Extracted CMake variables.
       1> Extracted source files and headers.
       1> Extracted code model.
       1> Extracted toolchain configurations.
       1> Extracted includes paths.
       1> CMake generation finished.
       ```

3. Wait for `CMake` configuration to complete (`CMake generation finished` in the log output.)

4. If you want or need to add additional build configurations to the default
   `x64-Debug`, click on the dropdown arrow next to `x64-Debug` on the IDE's
   ribbon. Select the `Manage Configurations...` dropdown item.

     - Click the green "`+`" sign on the left side of the `CMakeSettings.json`
       ("CMake Settings") window.

     - Select the configuration that you want to add from the list of supported
       configurations. For the purposes of this example, choose `x64-Release` to
       add the Win64 Release configuration.

       Other suitable configurations include `x86-Debug` and `x86-Release`.

     - Save `CMakeSettings.json` (`File>Save` or `Ctrl-S`)

     - Visual Studio will reconfigure `cmake` using the current VS configuration.
       In this example, it will still be set to `x64-Debug`. Wait for
       reconfiguration to finish.

     - When reconfiguration finishes, choose `x64-Release` from the configuration
       dropdown. Visual Studio will (again) reconfigure, this time using the
       `x64-Release` configuration. And wait for reconfiguration to finish (again.)

5. Select `Build All` from the `Build` menu, or equivalently chord `Ctrl-Shift-B`
   on the keyboard, to start the dependency feature library superbuild.

     - When all dependency feature libraries have been built, the build process
       __will__ unexpectedly terminate with a _"failed recompaction: Permission
       denied"_ error (see [this `ninja` note](#ninja-file-recompaction-permission-denied).)

       Choose `Delete Cache and Reconfigure` from the `Project` menu. This will
       cause CMake to reconfigure the project and detect the dependency feature
       libraries.

       When reconfiguration is complete, choose `Build All` from the `Build` menu,
       or equivalently chord `Ctrl-Shift-B` on the keyboard and restart building
       the simulator suite.

      Choose `Run CTests for simh` from the `Test` menu to run the simulator suite
      tests.

6. To install the simulators, choose `Install simh` from the `Build` menu. Note
   that the VS IDE will install the simulators in `out\build\<configuration>\install\bin`,
   and not the `BIN` directory off the top level SIMH source directory.


#### XP-compatible Build via the VS2019 and VS2022 IDEs

- Ensure that you previously installed [the `v141_xp` tools](#windows-xp-compatibleserver-2003-binaries).
- Follow steps 1-3 [above](#vs-2022-and-2019-walkthrough)
- Add the `x86-Debug` configuration, similar to step 4, with the following
  modifications:

  - Click the green "`+`" sign on the left side of the `CMakeSettings.json`
    ("CMake Settings") window.

  - Select the `x86-Debug` configuration.

  - Add `-T v141_xp` in the "CMake command arguments" text box.

  - Click on the "Show advanced settings" link.

  - Change the "CMake generator" to "Visual Studio 16 2019" for VS 2019 or
    "Visual Studio 17 2022" for VS 2022 from the "CMake generator" dropdown.

  - Save `CMakeSettings.json` (`File>Save` or `Ctrl-S`)

  - Scroll the "CMake Settings" window up until the "Save and generate CMake
    cache to load variables" link is visible. Click on this link and allow
    Visual Studio to reconfigure `CMake`.

- Repeat the above steps for the `x86-Release` configuration.

- Build, test and install as in steps 5 and 6 [above](#vs-2022-and-2019-walkthrough)


## `CMake` Generators

The available list of [CMake][cmake] build system generators is always available via:

```shell
$ cmake --help
[...]

Generators

The following generators are available on this platform (* marks default):
* Visual Studio 17 2022        = Generates Visual Studio 2022 project files.
                                 Use -A option to specify architecture.
* Visual Studio 16 2019        = Generates Visual Studio 2019 project files.
                                 Use -A option to specify architecture.
[...]
```
## Simulator Developer Notes


### How to compile a simulator: `add_simulator`

Each simulator subdirectory contains a `CMakeLists.txt` file (see [the directory
structure](#cmake-directory-structure).) Each of these simulator
`CMakeLists.txt` contain one or more [`add_simulator`](cmake/add_simulator.cmake)
function calls. For example, the AT&T 3b2 simulator invokes `add_simulator` as:

```cmake
add_simulator(3b2
    SOURCES
        3b2_cpu.c
        3b2_sys.c
        3b2_rev2_sys.c
        3b2_rev2_mmu.c
        3b2_mau.c
        3b2_rev2_csr.c
        3b2_timer.c
        3b2_stddev.c
        3b2_mem.c
        3b2_iu.c
        3b2_if.c
        3b2_id.c
        3b2_dmac.c
        3b2_io.c
        3b2_ports.c
        3b2_ctc.c
        3b2_ni.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        REV2
    FEATURE_FULL64
    LABEL 3B2
    TEST 3b2)
```

`add_simulator` is relatively self explanatory:

- The first argument is the simulator's executable name: `3b2`. This generates
  an executable named `3b2` on Unix platforms or `3b2.exe` on Windows.

- Argument list keywords: `SOURCES`, `INCLUDES`, `DEFINES`, `LABEL` and `TEST`.

    - `SOURCES`: The source files that comprise the simulator. The file names
      are relative to the simulator's source directory. In the `3b2`'s case,
      this is relative to the `3B2/` subdirectory where `3B2/CMakeLists.txt` is
      located.

      [CMake][cmake] sets the variable `CMAKE_CURRENT_SOURCE_DIR` to the same
      directory from which `CMakeLists.txt` is being read.

    - `INCLUDES`: Additional include/header file directories needed by the
      simulator, i.e., subdirectories that follow the compiler's `-I` flag).
      These subdirectories are relative to the top level `simh` directory.

      It's a good idea to add `${CMAKE_CURRENT_SOURCE_DIR}` to the list of
      simulator includes if the simulator uses header files in its own
      subdirectory.

    - `DEFINES`: Preprocessor defines needed by the simulator, i.e., values that
      follow the compiler's `-D` flags.

    - `LABEL`: The simulator's `ctest` test label to which `add_simulator` will
      prepend `simh-`. `ctest` labels group simulators in the same subdirectory,
      so it's possible to run an entire simulator group's tests:

      ```sh
      # Run all of the 3b2 simulator tests (3b2 and 3b2-700):
      $ ctest -L simh-3B2
      ```

      `add_simulator` names individual simulator tests by concatenating `simh-`
      and the executable's name, e.g., `simh-3b2` for the 3b2 simulator and
      `simh-3b2-700` for the 3b2/700 simulator. To execute the individual 3b2
      simulator's test:

      ```sh
      $ ctest -R simh-3b2
      ```

    - `TEST`: Some simulators have test scripts that follow the naming
      convention `[sim]_test.ini` -- the argument to the `TEST` parameter is the
      `[sim]` portion of the test script's name.

- Option keywords: These determine which of [simulator core libraries](#simulator-core-libraries) is
  linked with the simulator.

  - `FEATURE_INT64`: 64-bit integers, 32-bit pointers
  - `FEATURE_FULL64`: 64-bit integers, 64-bit pointers
  - `FEATURE_VIDEO`: Simulator video support.
  - `FEATURE_DISPLAY`: Video display support.
  - `USES_AIO`: Asynchronous I/O support (primarily useful for simulator
    network devices.)

- `PKG_FAMILY` option: This option adds the simulator to a package "family" or
  simulator packaging group, e.g., "DEC PDP simulators". The default package
  family is `default_family` if not specified.

- `BUILDROMS` option keyword: If the simulator has a boot ROM header file that
  is maintained or generated by `BuildROMS`, add this keyword to the
  `add_simulator` function call.


### Simulator "core" libraries

The `CMake` build infrastructure avoids repeatedly compiling the simulator
"core" source code. Instead, a simulator "links" with one of six (6) static
libraries that represents the combination of required features: 32/64 bit
support and video:

| Library           | Video | Integer size | Address size | `add_simulator` flags |
| :---------------- | :---: | -----------: | -----------: | :-------------------- |
| simhcore.a        | N     | 32           | 32           |                       |
| simhi64.a         | N     | 64           | 32           | `FEATURE_INT64`       |
| simhz64.a         | N     | 64           | 64           | `FEATURE_FULL64`      |
| simhcore\_video.a | Y     | 32           | 32           | `FEATURE_VIDEO`       |
| simhi64\_video.a  | Y     | 64           | 32           | `FEATURE_INT64`, `FEATURE_VIDEO` |
| simhz64\_video.a  | Y     | 64           | 64           | `FEATURE_FULL64`, `FEATURE_VIDEO` |

In addition to these six libraries, there are six asynchronous I/O (AIO)
variants that are built and linked into a simulator when the `USES_AIO` feature
flag is present in `add_simulator()`'s arguments:

| Library variant        | Description |
| :--------------------- | :---------: |
| simhcore\_aio.a        | simhcore.a with AIO support.        |
| simhi64\_aio.a         | simhi64.a with AIO support.         |
| simhz64\_aio.a         | simhz64.a with AIO support.         |
| simhcore\_video\_aio.a | simhcore\_video.a with AIO support. |
| simhi64\_video\_aio.a  | simhi64\_video.a with AIO support.  |
| simhz64\_video\_aio.a  | simhz64\_video.a with AIO support.  |

The `EXCLUDE_FROM_ALL` property is set on each of theses libraries in CMake to
avoid building the entire matrix. Practically speaking, 10 out of the 12 total
libraries actually build for the entire simulator suite.

Internally, these core libraries are [`CMake` interface libraries][cmake_interface_library] -- when they
are added to a simulator's executable via `target_link_libraries`, the simulator
inherits the public compile and linker flags from the interface library. Thus, each
core library provides a consistent set of preprocessor definitions, header file
directories, linker options, compiler flags appropriate to the desired simulator
support features for network, video and regular expressions.

## `add_simulator` Reference

```cmake
add_simulator(simulator_name
    SOURCES
        ## Source files here. Do not include the top-level
        ## 'sim*.c' or 'scp.c' -- those are part of the
        ## simulator core libraries.
    INCLUDES
        ## Places where the compiler should look for additional
        ## header files. Always a good idea to include
        ## CMAKE_CURRENT_SOURCE_DIR since it's the same as the
        ## simulator subdirectory
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        ## Additional preprocessor definitions, if needed. If
        ## not needed, leave it out.

    ## If neither FEATURE_INT64 or FEATURE_FULL64 is used, the
    ## simulator uses 32-bit integers and addresses.
    ##
    ## If you define both FEATURE_INT64 and FEATURE_FULL64,
    ## FEATURE_INT64 wins.

    ## 64-bit integers, 32-bit addresses
    FEATURE_INT64
    ## 64-bit integers, 64-bit addresses
    FEATURE_FULL64

    ## Simulator needs video support
    FEATURE_VIDEO

    ## Simulator needs display support (-DUSE_DISPLAY). Use
    ## in conjunction with FEATURE_VIDEO
    FEATURE_DISPLAY

    ## Simulator uses asynchronous I/O, i.e., calls AIO_CHECK_EVENT
    ## in its sim_instr() instruction simulation loop:
    USES_AIO

    ## Arguments to append after "RegisterSanityCheck". These arguments
    ## appear between "RegisterSanityCheck" and the test script, if
    ## given, e.g.:
    ##
    ##    mysimulator RegisterSanityCheck -r -t path/to/mysim_test.ini
    TEST_ARGS "-r"

    ## Packaging "family" (group) to which the simulator belongs,
    ## for packagers that support grouping (Windows: NSIS .exe,
    ## WIX .msi; macOS)
    PKG_FAMILY decpdp_family

    ## CTest label for grouping related simulators (3b2, VAXen,
    ## PDP-10)
    LABEL 3B2

    ## [sim] prefix for the simulator's "tests/[sim]_diag.ini" script
    TEST 3b2)
```

`add_simulator` defines a `CMake` executable target, which can be referenced
via the `simulator_name`. This can be useful if you need to add specific linker
flags, such as increasing the default thread stack size. The IBM 650 simulator
has example code that increases the thread stack size on Windows:

```cmake
add_simulator(i650
    SOURCES
        i650_cpu.c
        i650_cdr.c
        i650_cdp.c
        i650_dsk.c
        i650_mt.c
        i650_sys.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
    FEATURE_INT64
    LABEL I650
    TEST i650)

if (WIN32)
    if (MSVC)
        set(I650_STACK_FLAG "/STACK:8388608")
    else ()
        set(I650_STACK_FLAG "-Wl,--stack,8388608")
    endif ()
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.13")
        target_link_options(i650 PUBLIC "${I650_STACK_FLAG}")
    else ()
        set_property(TARGET i650 LINK_FLAGS " ${I650_STACK_FLAG}")
    endif ()
endif()
```

### Adding a new simulator

1. Create a new subdirectory in the top-level SIMH source directory.

    ```sh
    $ mkdir MySimulator
    $ cd MySimulator
    ```

2. Add your source to the `MySimulator` directory.

3. Create the `CMakeLists.txt` file that invokes `add_simulator`.

4. Include your new simulator in the `cmake/simh-simulators.cmake` file.

    ```cmake
    ## ...
    add_subdirectory(VAX)
    add_subdirectory(alpha)
    add_subdirectory(imlac)
    add_subdirectory(sigma)
    add_subdirectory(tt2500)

    ## add_subdirectory tells CMake to include MySimulator's CMakeLists.txt.
    ##
    ## This also adds MySimulator to Visual Studio's projects, and possibly informs
    ## other IDEs about the subdirectory.
    add_subdirectory(MySimulator)
    ```

5. Reconfigure your build. Refer to the [CMake command line](#cmake-command-line)
   section.

6. Build and develop.

### Regenerating `CMakeLists.txt` from the `makefile`

An alternate development path for new simulators is updating the SIMH `makefile`
in the top-level source directory, then regenerating the simulator suite's
`CMakeLists.txt` via the `cmake/generate.py` Python3 script. This works on both
Linux and Windows.

_Note:_ The `cmake/generate.py` script is not automatically run by `cmake` when
the `makefile` is newer than the top-level `CMakeLists.txt`. If you have done a
`git pull` and you get undefined symbols when a simulator is linked, the
solution is `cmake/generate.py` to update the affected simulator
`CMakeLists.txt` file.

```sh
## You have to be in the cmake subdirectory to run the generate.py script
$ cd cmake
$ python -m generate --help
usage: generate.py [-h] [--debug [DEBUG]] [--srcdir SRCDIR] [--orphans]

SIMH simulator CMakeLists.txt generator.

options:
  -h, --help       show this help message and exit
  --debug [DEBUG]  Debug level (0-3, 0 == off)
  --srcdir SRCDIR  makefile source directory.
  --orphans        Check for packaging orphans

# [simh_source] is the absolute path to your top-level SIMH source directory
$ python -m generate --orphans
generate.py: Looking for makefile, starting in [simh-source]/open-simh/cmake
generate.py: Looking for makefile, trying [simh-source]/open-simh
generate.py: Processing [simh-source]/open-simh/makefile
generate.py: all target pdp1
generate.py: all target pdp4
generate.py: all target pdp7
generate.py: all target pdp8
generate.py: all target pdp9
generate.py: all target pdp15
generate.py: all target pdp11
generate.py: all target pdp10
generate.py: all target vax
generate.py: all target microvax3900
generate.py: all target microvax1
generate.py: all target rtvax1000
generate.py: all target microvax2
generate.py: all target vax730
generate.py: all target vax750
generate.py: all target vax780
generate.py: all target vax8200
generate.py: all target vax8600
generate.py: all target besm6
generate.py: all target microvax2000
generate.py: all target infoserver100
generate.py: all target infoserver150vxt
generate.py: all target microvax3100
generate.py: all target microvax3100e
generate.py: all target vaxstation3100m30
generate.py: all target vaxstation3100m38
generate.py: all target vaxstation3100m76
generate.py: all target vaxstation4000m60
generate.py: all target microvax3100m80
generate.py: all target vaxstation4000vlc
generate.py: all target infoserver1000
generate.py: all target nd100
generate.py: all target nova
generate.py: all target eclipse
generate.py: all target hp2100
generate.py: all target hp3000
generate.py: all target i1401
generate.py: all target i1620
generate.py: all target s3
generate.py: all target altair
generate.py: all target altairz80
generate.py: all target gri
generate.py: all target i7094
generate.py: all target ibm1130
generate.py: all target id16
generate.py: all target id32
generate.py: all target sds
generate.py: all target lgp
generate.py: all target h316
generate.py: all target cdc1700
generate.py: all target swtp6800mp-a
generate.py: all target swtp6800mp-a2
generate.py: all target tx-0
generate.py: all target ssem
generate.py: all target b5500
generate.py: all target intel-mds
generate.py: all target scelbi
generate.py: all target 3b2
generate.py: all target 3b2-700
generate.py: all target i701
generate.py: all target i704
generate.py: all target i7010
generate.py: all target i7070
generate.py: all target i7080
generate.py: all target i7090
generate.py: all target sigma
generate.py: all target uc15
generate.py: all target pdp10-ka
    undefined make macro: KA10_DPY
    undefined make macro: KA10_LDFLAGS
generate.py: all target pdp10-ki
    undefined make macro: KI10_DPY
generate.py: all target pdp10-kl
generate.py: all target pdp10-ks
generate.py: all target pdp6
    undefined make macro: PDP6_DPY
    undefined make macro: PDP6_LDFLAGS
generate.py: all target i650
generate.py: all target imlac
generate.py: all target tt2500
generate.py: all target sel32
generate.py: exp target alpha
generate.py: exp target pdq3
generate.py: exp target sage
generate.py: Expecting to emit 78 simulators.
==== writing to [simh-source]/open-simh/3B2/CMakeLists.txt
==== writing to [simh-source]/open-simh/ALTAIR/CMakeLists.txt
==== writing to [simh-source]/open-simh/AltairZ80/CMakeLists.txt
==== writing to [simh-source]/open-simh/B5500/CMakeLists.txt
==== writing to [simh-source]/open-simh/BESM6/CMakeLists.txt
==== writing to [simh-source]/open-simh/CDC1700/CMakeLists.txt
==== writing to [simh-source]/open-simh/GRI/CMakeLists.txt
==== writing to [simh-source]/open-simh/H316/CMakeLists.txt
==== writing to [simh-source]/open-simh/HP2100/CMakeLists.txt
==== writing to [simh-source]/open-simh/HP3000/CMakeLists.txt
==== writing to [simh-source]/open-simh/I1401/CMakeLists.txt
==== writing to [simh-source]/open-simh/I1620/CMakeLists.txt
==== writing to [simh-source]/open-simh/I650/CMakeLists.txt
==== writing to [simh-source]/open-simh/I7000/CMakeLists.txt
==== writing to [simh-source]/open-simh/I7094/CMakeLists.txt
==== writing to [simh-source]/open-simh/Ibm1130/CMakeLists.txt
==== writing to [simh-source]/open-simh/Intel-Systems/Intel-MDS/CMakeLists.txt
==== writing to [simh-source]/open-simh/Intel-Systems/scelbi/CMakeLists.txt
==== writing to [simh-source]/open-simh/Interdata/CMakeLists.txt
==== writing to [simh-source]/open-simh/LGP/CMakeLists.txt
==== writing to [simh-source]/open-simh/ND100/CMakeLists.txt
==== writing to [simh-source]/open-simh/NOVA/CMakeLists.txt
==== writing to [simh-source]/open-simh/PDP1/CMakeLists.txt
==== writing to [simh-source]/open-simh/PDP10/CMakeLists.txt
==== writing to [simh-source]/open-simh/PDP11/CMakeLists.txt
==== writing to [simh-source]/open-simh/PDP18B/CMakeLists.txt
==== writing to [simh-source]/open-simh/PDP8/CMakeLists.txt
==== writing to [simh-source]/open-simh/PDQ-3/CMakeLists.txt
==== writing to [simh-source]/open-simh/S3/CMakeLists.txt
==== writing to [simh-source]/open-simh/SAGE/CMakeLists.txt
==== writing to [simh-source]/open-simh/SDS/CMakeLists.txt
==== writing to [simh-source]/open-simh/SEL32/CMakeLists.txt
==== writing to [simh-source]/open-simh/SSEM/CMakeLists.txt
==== writing to [simh-source]/open-simh/TX-0/CMakeLists.txt
==== writing to [simh-source]/open-simh/VAX/CMakeLists.txt
==== writing to [simh-source]/open-simh/alpha/CMakeLists.txt
==== writing to [simh-source]/open-simh/imlac/CMakeLists.txt
==== writing to [simh-source]/open-simh/sigma/CMakeLists.txt
==== writing to [simh-source]/open-simh/swtp6800/swtp6800/CMakeLists.txt
==== writing to [simh-source]/open-simh/tt2500/CMakeLists.txt
==== writing [simh-source]/open-simh/cmake/simh-simulators.cmake
==== writing [simh-source]/open-simh/cmake/simh-packaging.cmake
```


[appveyor]: https://www.appveyor.com/
[bison]: https://www.gnu.org/software/bison/
[chocolatey]: https://chocolatey.org/
[cmake_build_type]: https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html
[cmake_downloads]: https://cmake.org/download/
[cmake_interface_library]: https://cmake.org/cmake/help/latest/command/add_library.html#id4
[cmake]: https://cmake.org
[codeblocks]: http://www.codeblocks.org
[coreutils]: https://www.gnu.org/software/coreutils/coreutils.html
[cppcheck]: http://cppcheck.sourceforge.net/
[flex]: https://github.com/westes/flex
[FreeType]: https://www.freetype.org/
[gitscm_downloads]: https://git-scm.com/downloads
[gitscm]: https://git-scm.com/
[homebrew]: https://brew.sh/
[libpcap]: https://www.tcpdump.org/
[libpng]: http://www.libpng.org/pub/png/libpng.html
[macports]: https://www.macports.org/
[mingw64]: https://mingw-w64.org/
[ninja]: https://ninja-build.org/
[npcap]: https://nmap.org/npcap/
[older_vs_community]: https://visualstudio.microsoft.com/vs/older-downloads/
[open_simh_actions]: https://github.com/open-simh/simh/actions
[pcre2]: https://pcre.org
[pthreads4w]: https://github.com/jwinarske/pthreads4w
[scoop]: https://scoop.sh/
[SDL2_ttf]: https://www.libsdl.org/projects/SDL_ttf/
[SDL2]: https://www.libsdl.org/
[sublime]: https://www.sublimetext.com
[util-linux]: https://git.kernel.org/pub/scm/utils/util-linux/util-linux.git/
[v141_xp]: https://stackoverflow.com/questions/49516896/how-to-install-build-tools-for-v141-xp-for-vc-2017
[vcpkg_manifest]: https://learn.microsoft.com/en-us/vcpkg/users/manifests
[vs_community]: https://visualstudio.microsoft.com/vs/community/
[winflexbison]: https://github.com/lexxmark/winflexbison
[winsdk_download]: https://developer.microsoft.com/en-us/windows/downloads/sdk-archive/
[zlib]: https://www.zlib.net
