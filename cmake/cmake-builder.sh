#!/bin/bash

##-- Bash functions --
showHelp()
{
    [ x"$1" != x ] && { echo "${scriptName}: $1"; echo ""; }
    cat <<EOF
Configure and build simh simulators on Linux and *nix-like platforms.

Compile/Build options:
--clean (-x)      Remove the build subdirectory
--generate (-g)   Generate the build environment, don't compile/build
--cache           '--generate' and show CMake's variable cache
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
--no-aio          Build simulators without AIO (asynchronous I/O). NOTE: This will
                  impact certain devices' functionality, notably networking.
--no-aio-intrinsics
                  Do not use compiler/platform intrinsics to implement AIO
                  functions (aka "lock-free" AIO), reverts to lock-based AIO
                  if threading libraries are detected.

Other options:
--------------
--help (-h)       Print this help.
EOF

    exit 1
}

scriptName=$0
generateArgs=
buildArgs=
buildPostArgs=""
buildClean=
buildFlavor=
buildSubdir=
buildConfig=Release
testArgs=
notest=no
buildParallel=no
generateOnly=
testOnly=
noinstall=
installOnly=
verboseMode=
simTarget=
cpack_suffix=

## CMake supports "-S" flag (implies -B works as well.) Otherwise, it's
## the older invocation command line.
cmakeSFlag=

## This script really needs GNU getopt. Really. And try reallly hard to
## find the version that supports "--long-opt"
##
## MacOS workaround: MacOS has an older getopt installed in /usr/bin, brew
## has an updated version that installs in a custom place.
[[ -d /usr/local/opt/gnu-getopt/bin ]] && PATH="/usr/local/opt/gnu-getopt/bin:$PATH"
[[ -d /opt/homebrew/opt/gnu-getopt/bin ]] && PATH="/opt/homebrew/opt/gnu-getopt/bin:$PATH"
[[ -d /usr/local/opt/coreutils/libexec/gnubin ]] && PATH="/usr/local/opt/coreutils/libexec/gnubin:$PATH"

getopt_prog=
IFS_SAVE="${IFS}"; IFS=":"; for p in ${PATH}; do
    "${p}/getopt" -T > /dev/null 2>&1 
    if [[ $? -eq 4 ]]; then
        getopt_prog="${p}/getopt"
        break
    fi
done
IFS="${IFS_SAVE}"

if [[ "x${getopt_prog}" = "x" ]]; then
    echo "${scriptName}: GNU getopt needed for this script to function properly."
    echo "${scriptName}: Specifically, a 'getopt' that supports the '-T' flag (enhanced getopt)"
    exit 1
fi

## This script also needs GNU coreutils
realpath=$(which realpath) || {
    echo "${scriptName}: Could not find 'realpath'. Please install and re-execute this script."
    echo "${scriptName}: 'realpath' is a component of the GNU coreutils collection."
}
dirname=$(which dirname) || {
    echo "${scriptName}: Could not find 'dirname'. Please install and re-execute this script."
    echo "${scriptName}: 'dirname' is a component of the GNU coreutils collection."
}

## Check if CMake supports parallel
cmake=$(which cmake) || {
    echo "${scriptName}: Could not find 'cmake'. Please install and re-execute this script."
    exit 1
}

ctest=$(which ctest) || {
    echo "${scriptName}: Could not find 'ctest'. Please check your 'cmake' installation."
    exit 1
}

echo "** $(${cmake} --version)"

$(${cmake} -h 2>&1 | grep -- "-S" > /dev/null) && {
  cmakeSFlag=yes
}

canParallel=no
(${cmake} --build /tmp --help 2>&1 | grep parallel > /dev/null) && {
    canParallel=yes
}

canTestParallel=no
# (${ctest} --help 2>&1 | grep parallel > /dev/null) && {
#     canTestParallel=yes
# }

if [[ "x${MSYSTEM}" != x ]]; then
  case "${MSYSTEM}" in
    MSYS|MINGW64)
      buildFlavor="MinGW Makefiles"
      buildSubdir=build-mingw
      ;;
    UCRT64)
      buildFlavor="Ninja"
      buildSubdir=build-ninja
      ;;
  esac
fi

longopts=clean,help,flavor:,config:,nonetwork,novideo,notest,parallel,generate,testonly
longopts=${longopts},noinstall,installonly,verbose,target:,lto,debugWall,cppcheck,cpack_suffix:
longopts=${longopts},cache,no-aio,no-aio-intrinsics

ARGS=$(${getopt_prog} --longoptions $longopts --options xhf:c:pg -- "$@")
if [ $? -ne 0 ] ; then
    showHelp "${scriptName}: Usage error (use -h for help.)"
fi

eval set -- ${ARGS}
while true; do
    case $1 in
        -x | --clean)
            buildClean=yes; shift
            ;;
        -h | --help)
            showHelp
            ;;
        -f | --flavor)
            case "$2" in
                unix)
                    buildFlavor="Unix Makefiles"
                    buildSubdir=build-unix
                    shift 2
                    ;;
                ninja|ucrt64)
                    buildFlavor=Ninja
                    buildSubdir=build-ninja
                    shift 2
                    ;;
                xcode)
                    buildFlavor=Xcode
                    buildSubdir=build-xcode
                    shift 2
                    ;;
                xcode-universal)
                    buildFlavor=Xcode
                    buildSubdir=build-xcode-universal
                    generateArgs="${generateArgs} -DMAC_UNIVERSAL:Bool=On"
                    shift 2
                    ;;
                mingw|mingw64|msys|msys2)
                    buildFlavor="MinGW Makefiles"
                    buildSubdir=build-mingw
                    shift 2
                    ;;
                *)
                    showHelp "Invalid build flavor: $2"
                    ;;
            esac
            ;;
        -c | --config)
            case "$2" in
                Release|Debug)
                    buildConfig=$2
                    shift 2
                    ;;
                *)
                    showHelp "Invalid build configuration: $2"
                    ;;
            esac
            ;;
        --nonetwork)
            generateArgs="${generateArgs} -DWITH_NETWORK:Bool=Off"
            shift
            ;;
        --novideo)
            generateArgs="${generateArgs} -DWITH_VIDEO:Bool=Off"
            shift
            ;;
        --no-aio)
            generateArgs="${generateArgs} -DWITH_ASYNC:Bool=Off"
            shift
            ;;
        --no-aio-intrinsics)
            generateArgs="${generateArgs} -DDONT_USE_AIO_INTRINSICS:Bool=On"
            shift
            ;;
        --notest)
            notest=yes
            shift
            ;;
        --noinstall)
            noinstall=yes
            shift
            ;;
        --lto)
            generateArgs="${generateArgs} -DRELEASE_LTO:Bool=On"
            shift
            ;;
        --debugWall)
            generateArgs="${generateArgs} -DDEBUG_WALL:Bool=On"
            shift
            ;;
        --cppcheck)
            generateArgs="${generateArgs} -DENABLE_CPPCHECK:Bool=On"
            shift
            ;;
        --cpack_suffix)
            generateArgs="${generateArgs} -DSIMH_PACKAGE_SUFFIX=$2"
            shift 2
            ;;
        -p | --parallel)
            buildParallel=yes
            shift
            ;;
        -g | --generate)
            generateOnly=yes
            shift
            ;;
        --cache)
            generateOnly=yes
            generateArgs="${generateArgs} -LA"
            shift
            ;;
        --testonly)
            testOnly=yes
            shift
            ;;
        --installonly)
            installOnly=yes
            shift
            ;;
        --verbose)
            verboseMode="--verbose"
            shift
            ;;
        --target)
            noinstall=yes
            simTarget="${simTarget} $2"
            shift 2
            ;;
        --)
            ## End of options. we'll ignore.
            shift
            break
            ;;
    esac
done

# Sanity check: buildSubdir should be set, unless the '-f' flag wasn't present.
if [ "x${buildSubdir}" = x ]; then
  echo ""
  echo "${scriptName}: Build flavor is NOT SET -- see the \"--flavor\"/\"-f\" flag in the help."
  echo ""
  showHelp
fi

## Determine the SIMH top-level source directory:
simhTopDir=$(${dirname} $(${realpath} $0))
while [ "x${simhTopDir}" != x -a ! -f "${simhTopDir}/CMakeLists.txt" ]; do
  simhTopDir=$(${dirname} "${simhTopDir}")
done

if [[ "x${simhTopDir}" = x ]]; then
  echo "${scriptName}: Can't determine SIMH top-level source directory."
  echo "Did this really happen?"
  exit 1
else
  buildSubdir=$(${realpath} "${simhTopDir}/cmake/")"/${buildSubdir}"
  echo "${scriptName}: SIMH top-evel directory: ${simhTopDir}"
  echo "${scriptName}: Build directory:         ${buildSubdir}"
fi

if [[ x"$buildClean" != x ]]; then
    echo "${scriptName}: Cleaning ${buildSubdir}"
    rm -rf ${buildSubdir}
fi

if [[ ! -d ${buildSubdir} ]]; then
    mkdir ${buildSubdir}
fi

## Setup test arguments (and add parallel later)
testArgs="-C ${buildConfig} --timeout 180 --output-on-failure"

## Parallel only applies to the unix flavor. GNU make will overwhelm your
## machine if the number of jobs isn't capped.
if [[ x"$canParallel" = xyes ]] ; then
    if [ x"$buildParallel" = xyes -a "$buildFlavor" != Ninja ] ; then
        (${cmake} --build . --help 2>&1 | grep parallel 2>&1 > /dev/null) && {
          buildArgs="${buildArgs} --parallel"
	  buildPostArgs="${buildPostArgs} -j 8"
	}

    # Don't execute ctest in parallel...
    # [ x${canTestParallel} = xyes ] && {
    #    testArgs="${testArgs} --parallel 4"
    # }
    fi
else
    buildParallel=
fi

if [[ x"${simTarget}" != x ]]; then
    simTests=""
    for tgt in $(echo ${simTarget} | sed 's/,/ /g'); do
        buildArgs="${buildArgs} --target ${tgt}"
        [[ x"${simTests}" != x ]] && simTests="${simTests}|"
        simTests="${simTests}${tgt}"
    done
    testArgs="${testArgs} -R simh-(${simTests})\$"
fi

buildArgs="${buildArgs} --config ${buildConfig}"

if [[ x$generateOnly = xyes ]]; then
    phases=generate
elif [[ x$testOnly = xyes ]]; then
    phases=test
elif [[ x$installOnly = xyes ]]; then
    phases=install
else
    phases="generate build"
    if [[ x${notest} != xyes ]]; then
        phases="${phases} test"
    fi
fi

for ph in ${phases}; do
    case $ph in
    generate)
        ## Uncondintionally remove the CMake cache.
        echo "${scriptName}: Removing CMakeCache.txt and CMakeFiles"
        rm -rf ${buildSubdir}/CMakeCache.txt ${buildSubdir}/CMakefiles

        if [[ "x${cmakeSFlag}" != x ]]; then
          echo "${cmake} -G "\"${buildFlavor}\"" -DCMAKE_BUILD_TYPE="${buildConfig}" -S "${simhTopDir}" -B ${buildSubdir} ${generateArgs}"
          ${cmake} -G "${buildFlavor}" -DCMAKE_BUILD_TYPE="${buildConfig}" -S "${simhTopDir}" -B "${buildSubdir}" ${generateArgs} || { \
            echo "*** ${scriptName}: Errors detected during environment generation. Exiting."
            exit 1
          }
        else
          echo "${cmake} -G "\"${buildFlavor}\"" -DCMAKE_BUILD_TYPE="${buildConfig}" "${simhTopDir}" ${generateArgs}"
          ( cd "${buildSubdir}"; \
            ${cmake} -G "${buildFlavor}" -DCMAKE_BUILD_TYPE="${buildConfig}" "${simhTopDir}" ${generateArgs}) || { \
              echo "*** ${scriptName}: Errors detected during environment generation. Exiting.";
              exit 1
            }
        fi
        ;;
    build)
        ${cmake} --build "${buildSubdir}" ${buildArgs} ${verboseMode} -- ${buildPostArgs} || {
            echo "*** ${scriptName}: Build errors detected. Exiting."
            exit 1
        }
        ;;
    test)
        (cd "${buildSubdir}" \
            && echo ${ctest} ${testArgs} ${verboseMode} \
            && ${ctest} ${testArgs} ${verboseMode}) || {
            echo "*** ${scriptName}: Errors detected during testing. Exiting."
            exit 1
        }
        ;;
    install)
        ${cmake} --build "${buildSubdir}" --target install --config "${buildConfig}"
        ;;
    package)
        (cd "${buildSubdir}" \
            && ${cpack} -G ZIP -C ${buildConfig} ${verboseMode} \
            && mv *.zip ${simhTopDir}/PACKAGES \
        )
        ;;
    esac
done
