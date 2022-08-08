#!/bin/bash

##-- Bash functions --
showHelp()
{
    [ x"$1" != x ] && { echo "${scriptName}: $1"; echo ""; }
    cat <<EOF
Configure and build simh simulators on Linux and *nix-like platforms.

Subdirectories:
cmake/build-unix:  Makefile-based build simulators
cmake/build-ninja: Ninja build-based simulators

Options:
--------
--clean (-x)      Remove the build subdirectory
--generate (-g)   Generate the build environment, don't compile/build
--regenerate (-r) Regenerate the build environment from scratch.
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
EOF

    exit 1
}

scriptName=$0
buildArgs=
buildPostArgs=""
buildClean=
buildFlavor="Unix Makefiles"
buildSubdir=build-unix
buildConfig=Release
testArgs=
notest=no
buildParallel=no
generateOnly=
regenerateFlag=
testOnly=
noinstall=
installOnly=
allInOne=

## This script really needs GNU getopt. Really.
getopt -T > /dev/null
if [ $? -ne 4 ] ; then
    echo "${scriptName}: GNU getopt needed for this script to function properly."
    exit 1
fi

## Check if CMake supports parallel
cmake=$(which cmake) || {
    echo "${scriptName}: Could not find 'cmake'. Please install and re-execute this script."
    exit 1
}

ctest=$(which ctest) || {
    echo "${scriptName}: Could not find 'ctest'. Please check your 'cmake' installation."
    exit 1
}

canParallel=no
(${cmake} --build /tmp --help 2>&1 | grep parallel > /dev/null) && {
    canParallel=yes
}

canTestParallel=no
# (${ctest} --help 2>&1 | grep parallel > /dev/null) && {
#     canTestParallel=yes
# }

longopts=clean,help,flavor:,config:,nonetwork,notest,parallel,generate,testonly,regenerate
longopts=${longopts},noinstall,installonly,allInOne

ARGS=$(getopt --longoptions $longopts --options xhf:cpg -- "$@")
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
                ninja)
                    buildFlavor=Ninja
                    buildSubdir=build-ninja
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
            buildArgs="${buildArgs} -DWITH_NETWORK:Bool=Off -DWITH_PCAP:Bool=Off -DWITH_SLIRP:Bool=Off"
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
        -p | --parallel)
            buildParallel=yes
            shift
            ;;
        -g | --generate)
            generateOnly=yes
            shift
            ;;
        -r | --regenerate)
            generateOnly=yes
	    regenerateFlag=yes
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
        --allInOne)
            allInOne=yes
            buildClean=yes
            shift
            ;;
        --)
            ## End of options. we'll ignore.
            shift
            break
            ;;
    esac
done

## Determine the SIMH top-level source directory:
simhTopDir=$(dirname $(realpath $0))
while [ "x${simhTopDir}" != x -a ! -f "${simhTopDir}/CMakeLists.txt" ]; do
  simhTopDir=$(dirname "${simhTopDir}")
done

if [[ "x${simhTopDir}" = x ]]; then
  echo "${scriptName}: Can't determine SIMH top-level source directory."
  echo "Did this really happen?"
  exit 1
else
  buildSubdir=$(realpath "${simhTopDir}/cmake/${buildSubdir}")
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
    phases="${phases} install"
fi

for ph in ${phases}; do
    case $ph in
    generate)
	[ x$regenerateFlag = xyes ] && {
	    echo "${scriptName}: Removing CMakeCache.txt and CMakeFiles"
	    rm -rf ${buildSubdir}/CMakeCache.txt ${buildSubdir}/CMakefiles
	}
        ( cd "${buildSubdir}" \
          && ${cmake} -G "${buildFlavor}" -DCMAKE_BUILD_TYPE="${buildConfig}" "${simhTopDir}" ) || {
          echo "*** ${scriptName}: Errors detected during environment generation. Exiting."
          exit 1
        }
        ;;
    build)
        ${cmake} --build "${buildSubdir}" ${buildArgs} -- ${buildPostArgs} || {
            echo "*** ${scriptName}: Build errors detected. Exiting."
            exit 1
        }
        ;;
    test)
        (cd "${buildSubdir}" && ${ctest} ${testArgs}) || {
            echo "*** ${scriptName}: Errors detected during testing. Exiting."
            exit 1
        }
        ;;
    install)
        ${cmake} --build "${buildSubdir}" --target install
        ;;
    esac
done
