# Open SIMH machine simulator

- [Open SIMH machine simulator](#open-simh-machine-simulator)
  - [What is Open SIMH?](#what-is-open-simh)
    - [PLEASE NOTE](#please-note)
  - [Getting started: Build Open SIMH](#getting-started-build-open-simh)
    - [Linux, macOS, MinGW64](#linux-macos-mingw64)
    - [Windows](#windows)
  - [But I only want to build one particular simulator...](#but-i-only-want-to-build-one-particular-simulator)



## What is Open SIMH?

SIMH is a framework and collection of computer system simulators, created by Bob Supnik, originally at Digital Equipment Corporation, and extended by contributions of many other people.

Open SIMH is now an open source project, licensed under an MIT open source license (see [LICENSE.txt](LICENSE.txt) for the specific wording).  The project gatekeepers are the members of the [SIMH Steering Group](SIMH-SG.md).  We welcome and encourage contributions from all.  Contributions will be covered by the project license.

The Open SIMH code base is derived from a code base maintained by Mark Pizzolato as of 12 May 2022.  From that point onward there is no connection between that source and the Open SIMH code base.  A detailed listing of features as of that point may be found in [SIMH-V4-status](SIMH-V4-status.md).

### PLEASE NOTE

**Do not** contribute material taken from `github.com/simh/simh` unless you are the author of the material in question.

<a href="https://scan.coverity.com/projects/open-simh-simh">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/29458/badge.svg"/>
</a>

## Getting started: Build Open SIMH

### Linux, macOS, MinGW64

__Step 1__: Dependency libraries, such as SDL2 and PCRE/PCRE2, are installed via the `.travis/deps.sh` script. `.travis/deps.sh` takes a single argument: the plaform
for which Open SIMH is being built. Platform names include `osx` ([HomeBrew](homebrew)), `macports` ([MacPorts](macports)), `linux` (Debian-based Linux _apt_
installer), `arch-linux` (Arch Linux _pacman_ installer), `mingw32` (32-bit MinGW64), `mingw64` (MinGW64 GCC), `ucrt64` (MinGW Universal C Runtime target)
and `clang64` (MinGW Clang).

For example, to install dependencies on a Linux platform (Debian, Ubuntu, ...) that uses the _apt_ package manager:
```
$ sudo sh .travis/deps.sh linux
```

The `.travis/deps.sh` script also installs [Git](gitscm), the [CMake meta-build system](cmake) and the [Ninja build system](ninja), if not already
installed on your system.

__Step 2__: Once the library dependencies are installed, you're ready to build the simulators:

```shell
## Get the simulator source code
$ git clone https://github.com/bscottm/open-simh

## Change into the open-simh subdirectory, ensure that you're on the
## mainpath (not master) branch:
$ cd open-simh
$ git switch mainpath

## Configure
$ cmake --preset ninja-release

## Build
$ cmake --build --preset ninja-release
```

You will find the simulators under the `BIN` subdirectory.

### Windows

__Step 1__: Install Visual Studio 2022 if you haven't already.

__Step 2__: Install [Git](gitscm) and [CMake meta-build system](cmake). These can be installed via package managers such as [Scoop](scoop) or
[Chocolatey](chocolatey)[^1] or directly via their respective installer packages downloaded from their web sites.

[^1]: If you don't have a Windows software package manager installed and need to make a choice between Scoop and Chocolatey, Scoop is preferable over Chocolatey.
Scoop doesn't require elevated privileges and installs software under your Windows home directory. Scoop is also easy to remove: just delete the entirety of the
`scoop` directory in your Windows home directory.

To install Git and CMake via Scoop from a PowerShell prompt:
```powershell
## If you need to install Scoop, use the following two lines (otherwise,
## you can ignore them.)
PS > Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
PS > Invoke-RestMethod -Uri https://get.scoop.sh | Invoke-Expression

## Install cmake and Git
PS > scoop install cmake git
```

__Step 3__: Build the simulators. CMake clones the [vcpkg](vcpkg) C/C++ dependency manager as a submodule and bootstraps it. You will see [vcpkg](vcpkg) build the
dependency libraries during CMake's _configure_ phase[^2].

[^2]: Dependency libraries are always built from source via vcpkg. This ensures that the library matches the Microsoft compiler, which avoids new or
incompatible library metadata issues between compiler sub- and patch-versions.

This example uses the PowerShell prompt; the commands are the same if you use the traditional `CMD` command interpreter. The `##` lines are comments.
```powershell
## Clone the simulator code
PS > git clone https://github.com/bscottm/open-simh

## Change into the open-simh subdirectory, ensure that you're on the
## mainpath (not master) branch:
PS > cd open-simh
PS > git switch mainpath

## Configure
PS > cmake --preset win32-vs2022-release

## Build
PS > cmake --build --preset win32-vs2022-release
```

You will find the simulators under the `BIN\Win32\Release` subdirectory.

## But I only want to build one particular simulator...

- `cmake --preset ninja-release --target pdp11` builds the PDP-11 simulator on Linux and macOS platforms.

- `cmake --preset win32-vs2022-release --target pdp11` builds the PDP-11 simulator on Windows.

There is no straighforward way to ask CMake to list all of the available simulator targets. Looking at the `CMakeLists.txt` and searching for `add_simulator` is one
way to find which simulators are built under a particular subdirectory.

You can also direct CMake to build multiple targets with multiple `--target` flags on the command line. For example:
```shell
$ cmake --preset ninja-release --target pdp11 --target pdp10-kl --target imlac
```


[chocolatey]: https://chocolatey.org/
[cmake]: https://cmake.org/download/
[gitscm]: https://git-scm.com/
[homebrew]: https://brew.sh/
[macports]: https://www.macports.org/
[ninja]: https://ninja-build.org/
[scoop]: https://scoop.sh/
[vcpkg]: https://vcpkg.io/