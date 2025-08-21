# Author: B. Scott Michel (scooter.phd@gmail.com)
# "scooter me fecit"

<#
.SYNOPSIS
Configure and build SIMH's dependencies and simulators using the Microsoft Visual
Studio C compiler or MinGW-W64-based gcc compiler.

.DESCRIPTION
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

.EXAMPLE
PS> cmake-builder.ps1 -flavor vs2022 -config Release

Generate/configure, build, test and install the SIMH simulator suite using
the Visual Studio 2022 toolchain in the Release (optimized) compile
configuration.

.EXAMPLE
PS> cmake-builder.ps1 vs2022 Release

Another way to generate/configure, build, test and install the SIMH simulator
suite using the Visual Studio 2022 toolchain in the Release (optimized)
compile configuration.

.EXAMPLE
PS> cmake-builder.ps1 vs2019 Debug -notest -noinstall

Generate/configure and build the SIMH simulator suite with the Visual Studio
2019 toolchain in the Debug compile configuration. Does not execute tests and
does not install the simulators under the BIN subdirectory in the top of the
source tree.

.EXAMPLE

PS> cmake-builder.ps1 -flavor vs2019 -config Release -installonly

Install the simulators under the BIN subdirectory in the top of the source
tree. Does not generate/configure, but will build to ensure that compile
targets (simulator executables) are up-to-date.
#>

param (
    ## String arguments are positional, so if the user invokes this script
    ## as "cmake-builder.ps1 vs2022 Debug", it's the same as saying
    ## "cmake-builder.ps1 -flavor vs2022 -config Debug"


    ## The build environment's "flavor" that determines which CMake generator is used
    ## to create all of the build machinery to compile the SIMH simulator suite
    ## and the target compiler.
    ## 
    ## Supported flavors:
    ## ------------------
    ## vs2022          Visual Studio 2022 (default)
    ## vs2022-xp       Visual Studio 2022 XP compat
    ## vs2022-x64      Visual Studio 2022 64-bit
    ## vs2019          Visual Studio 2019
    ## vs2019-xp       Visual Studio 2019 XP compat
    ## vs2019-x64      Visual Studio 2019 64-bit
    ## vs2017          Visual Studio 2017
    ## vs2017-xp       Visual Studio 2017 XP compat
    ## vs2017-x64      Visual Studio 2017 64-bit
    ## vs2015          Visual Studio 2015
    ## mingw-make      MinGW GCC/mingw32-make
    ## mingw-ninja     MinGW GCC/ninja
    [Parameter(Mandatory=$false)]
    [string] $flavor         = "vs2022",

    ## The target build configuration. Valid values: "Release", "Debug" and
    ## "RelWithDebInfo"
    [Parameter(Mandatory=$false)]
    [string] $config         = "Release",

    ## Supply a suffix for CPack package names via -DSIMH_PACKAGE_SUFFIX
    [Parameter(Mandatory=$false)]
    [string] $cpack_suffix = "",

    ## (optional) Build a specific simulator or simulators. Separate multiple
    ## targets with a comma, ## e.g. "--target pdp8,pdp11,vax750,altairz80,3b2"
    [Parameter(Mandatory=$false)]
    [string[]] $target     = "",

    ## The rest are flag arguments

    ## Clean (remove) the CMake build directory before configuring
    [Parameter(Mandatory=$false)]
    [switch] $clean          = $false,

    ## Get help.
    [Parameter(Mandatory=$false)]
    [switch] $help           = $false,

    ## Compile the SIMH simulator suite without network support.
    [Parameter(Mandatory=$false)]
    [switch] $nonetwork      = $false,

    ## Compile the SIMH simulator suite without video support.
    [Parameter(Mandatory=$false)]
    [switch] $novideo        = $false,

    ## Compile the SIMH simulator without AIO support.
    [Parameter(Mandatory=$false)]
    [switch] $noaio = $false,

    ## Compile the SIMH simulator without AIO instrinsics ("lock-free" AIO),
    ## using lock-based AIO via thread mutexes instead.
    [Parameter(Mandatory=$false)]
    [switch] $noaiointrinsics = $false,

    ## Disable the build's tests.
    [Parameter(Mandatory=$false)]
    [switch] $notest         = $false,

    ## Do not install the simulator suite in the source directory's BIN
    ## subdirectory.
    [Parameter(Mandatory=$false)]
    [switch] $noinstall      = $false,

    ## Enable parallel builds.
    [Parameter(Mandatory=$false)]
    [switch] $parallel       = $false,

    ## Configure and generate the build environment. Don't compile, test or install.
    [Parameter(Mandatory=$false)]
    [switch] $generate       = $false,

    ## Only run the tests.
    [Parameter(Mandatory=$false)]
    [switch] $testonly       = $false,

    ## Only install the SIMH simulator suite in the source directory's BIN
    ## subdirectory.
    [Parameter(Mandatory=$false)]
    [switch] $installOnly    = $false,

    ## Turn on Windows API deprecation warnings. NOTE: These warnings are OFF by
    ## default.
    [Parameter(Mandatory=$false)]
    [switch] $windeprecation = $false,

    ## Enable Link-Time Optimization (LTO).
    [Parameter(Mandatory=$false)]
    [switch] $lto            = $false,

    ## Turn on maximal compiler warnings for Debug builds (e.g. "-Wall" or "/W3")
    [Parameter(Mandatory=$false)]
    [switch] $debugWall      = $false,

    ## Enable the cppcheck static code analysis rules
    [Parameter(Mandatory=$false)]
    [switch] $cppcheck       = $false
)

$scriptName = $(Split-Path -Leaf $PSCommandPath)
$scriptCmd  = ${PSCommandPath}

function Show-Help
{
    Get-Help -full ${scriptCmd}
    exit 0
}


## CMake generator info:
class GeneratorInfo
{
    [string]  $Generator
    [bool]    $SingleConfig
    [bool]    $UCRT
    [string]  $UCRTVersion
    [string[]]$ArchArgs

    GeneratorInfo([string]$gen, $configFlag, $ucrtFlag, $ucrtVer, [string[]]$arch)
    {
        $this.Generator = $gen
        $this.SingleConfig = $configFlag
        $this.UCRT = $ucrtFlag
        $this.UCRTVersion = $ucrtVer
        $this.ArchArgs  = $arch
    }
}

## Multiple build configurations selected at compile time
$multiConfig = $false
## Single configuration selected at configuration time
$singleConfig = $true

$cmakeGenMap = @{
    "vs2022"      = [GeneratorInfo]::new("Visual Studio 17 2022", $multiConfig,  $false, "",     @("-A", "Win32"));
    "vs2022-xp"   = [GeneratorInfo]::new("Visual Studio 17 2022", $multiConfig,  $false, "",     @("-A", "Win32", "-T", "v141_xp"));
    "vs2022-x64"  = [GeneratorInfo]::new("Visual Studio 17 2022", $multiConfig,  $false, "",     @("-A", "x64", "-T", "host=x64"));
    "vs2019"      = [GeneratorInfo]::new("Visual Studio 16 2019", $multiConfig,  $false, "",     @("-A", "Win32"));
    "vs2019-xp"   = [GeneratorInfo]::new("Visual Studio 16 2019", $multiConfig,  $false, "",     @("-A", "Win32", "-T", "v141_xp"));
    "vs2019-x64"  = [GeneratorInfo]::new("Visual Studio 17 2022", $multiConfig,  $false, "",     @("-A", "x64", "-T", "host=x64"));
    "vs2017"      = [GeneratorInfo]::new("Visual Studio 15 2017", $multiConfig,  $false, "",     @("-A", "Win32"));
    "vs2017-xp"   = [GeneratorInfo]::new("Visual Studio 15 2017", $multiConfig,  $false, "",     @("-A", "Win32", "-T", "v141_xp"));
    "vs2017-x64"  = [GeneratorInfo]::new("Visual Studio 17 2022", $multiConfig,  $false, "",     @("-A", "x64", "-T", "host=x64"));
    "vs2015"      = [GeneratorInfo]::new("Visual Studio 14 2015", $multiConfig,  $false, "",     @());
    "mingw-make"  = [GeneratorInfo]::new("MinGW Makefiles",       $singleConfig, $false, "",     @());
    "mingw-ninja" = [GeneratorInfo]::new("Ninja",                 $singleConfig, $false, "",     @())
}


function Get-GeneratorInfo([string]$flavor)
{
    return $cmakeGenMap[$flavor]
}

function Quote-Args([string[]]$arglist)
{
    return ($arglist | foreach-object { if ($_ -like "* *") { "`"$_`"" } else { $_ } })
}


## Output help early and exit.
if ($help)
{
    Show-Help
}

### CTest params:
## timeout is 180 seconds
$ctestTimeout  = "300"

## Sanity checking: Check that utilities we expect exist...
## CMake: Save the location of the command because we'll invoke it later. Same
## with CTest
$cmakeCmd = $(Get-Command -Name cmake.exe -ErrorAction Ignore).Path
$ctestCmd = $(Get-Command -Name ctest.exe -ErrorAction Ignore).Path
if ($cmakeCmd.Length -gt 0)
{
    Write-Host "** ${scriptName}: cmake is '${cmakeCmd}'"
    Write-Host "** $(& ${cmakeCmd} --version)"
}
else {
    @"
!! ${scriptName} error:

The 'cmake' command was not found. Please ensure that you have installed CMake
and that your PATH environment variable references the directory in which it
was installed.
"@

    exit 1
}

if (!$testonly)
{
    ## Check for GCC and mingw32-make if user wants the mingw flavor build.
    if ($flavor -eq "mingw" -or $flavor -eq "ninja")
    {
        if ($(Get-Command gcc -ErrorAction Ignore).Path.Length -eq 0) {
            @"
    !! ${scriptName} error:

    Did not find 'gcc', the GNU C/C++ compiler toolchain. Please ensure you have
    installed gcc and that your PATH environment variables references the directory
    in which it was installed.
"@
            exit 1
        }

        if ($(Get-Command mingw32-make -ErrorAction Ignore).Path.Length -eq 0) {
            @"
    !! ${scriptName} error:

    Did not find 'mingw32-make'. Please ensure you have installed mingw32-make and
    that your PATH environment variables references the directory in which it was
    installed.

    See the .travis/deps.sh functions mingw64() and ucrt64() for the pacman packages
    that should be installed.
"@
            exit 1
        }
    }
}

## Validate the requested configuration.
if (!@("Release", "Debug", "RelWithDebInfo").Contains($config))
{
    @"
${scriptName}: Invalid configuration: "${config}".

"@
    Show-Help
}

## Look for Git's /usr/bin subdirectory: CMake (and other utilities) have issues
## with the /bin/sh installed there (Git's version of MinGW.)

$tmp_path = $env:PATH
$git_usrbin = "${env:ProgramFiles}\Git\usr\bin"
$tmp_path = ($tmp_path.Split(';') | Where-Object { $_ -ne "${git_usrbin}"}) -join ';'
if ($tmp_path -ne ${env:PATH})
{
    Write-Host "** ${scriptName}: Removed ${git_usrbin} from PATH (Git MinGW problem)"
    $env:PATH = $tmp_path
}

## Also make sure that none of the other cmake-* directories are in the user's PATH
## because CMake's find_package does traverse PATH looking for potential candidates
## for dependency libraries.

$origPath = $env:PATH
$modPath  = $origPath

if (Test-Path -Path cmake\dependencies) {
  $bdirs = $(Get-ChildItem -Attribute Directory cmake\dependencies\*).ForEach({ $_.FullName + "\bin" })
  $modPath  = (${env:Path}.Split(';') | Where-Object { $bdirs -notcontains $_ }) -join ';'
  if ($modPath -ne $origPath) {
    Write-Host "** ${scriptName}: Removed cmake\dependencies 'bin' directories from PATH."
  }
}

## Setup:
$simhTopDir = $(Split-Path -Parent $(Resolve-Path -Path $PSCommandPath).Path)
While (!([String]::IsNullOrEmpty($simhTopDir) -or (Test-Path -Path ${simhTopDir}\CMakeLists.txt))) {
    $simhTopDir = $(Split-Path -Parent $simhTopDir)
}
if ([String]::IsNullOrEmpty($simhTopDir)) {
    @"
!! ${scriptName}: Cannot locate SIMH top-level source directory from
the script's path name. You should really not see this message.
"@

    exit 1
} else {
    Write-Host "** ${scriptName}: SIMH top-level source directory is ${simhTopDir}"
}

$buildDir  = "${simhTopDir}\cmake\build-${flavor}"
$genInfo = $(Get-GeneratorInfo $flavor)
if ($null -eq $genInfo)
{
    Write-Host ""
    Write-Host "!! ${scriptName}: Unrecognized build flavor '${flavor}'."
    Write-Host ""
    Show-Help
}

if ($testonly)
{
    $scriptPhases = @("test")
}
elseif ($generate)
{
    $scriptPhases = @("generate")
}
elseif ($installOnly)
{
    $scriptPhases = @("install")
}
else
{
  $scriptPhases = @( "generate", "build", "test")
  if ($notest)
  {
      $scriptPhases = $scriptPhases | Where-Object { $_ -ne 'test' }
  }
  if ($noinstall -or ![String]::IsNullOrEmpty($target))
  {
      $scriptPhases = $scriptPhases | Where-Object { $_ -ne 'install' }
  }
}

if (($scriptPhases -contains "generate") -or ($scriptPhases -contains "build"))
{
    ## Clean out the build subdirectory
    if ((Test-Path -Path ${buildDir}) -and $clean)
    {
        Write-Host "** ${scriptName}: Removing ${buildDir}"
        Remove-Item -recurse -force -Path ${buildDir} -ErrorAction SilentlyContinue | Out-Null
    }

    if (!(Test-Path -Path ${buildDir}))
    {
        Write-Host "** ${scriptName}: Creating ${buildDir} subdirectory"
        New-Item -Path ${buildDir} -ItemType Directory | Out-Null
    }
    else
    {
        Write-Host "** ${scriptName}: ${buildDir} exists."
    }

    ## Unconditionally remove the CMake cache.
    Remove-Item          -Force -Path ${buildDir}/CMakeCache.txt -ErrorAction SilentlyContinue | Out-Null
    Remove-Item -Recurse -Force -Path ${buildDir}/CMakeFiles     -ErrorAction SilentlyContinue | Out-Null
   
    ## Where we do the heaving lifting:
    $generateArgs = @("-G", $genInfo.Generator)
    if ($genInfo.SingleConfig) {
        ## Single configuration set at compile time:
        $generateArgs += @("-DCMAKE_BUILD_TYPE=${config}")
    }
    if ($genInfo.UCRT) {
        ## Universal Windows Platform
        $generateArgs += @("-DCMAKE_SYSTEM_NAME=WindowsStore", "-DCMAKE_SYSTEM_VERSION=$($genInfo.UCRTVersion)")
    }
    $generateArgs += $genInfo.ArchArgs + @("-Wno-dev", "--no-warn-unused-cli")
    if ($nonetwork)
    {
        $generateArgs += @("-DWITH_NETWORK:Bool=Off")
    }
    if ($novideo)
    {
      $generateArgs += @("-DWITH_VIDEO:Bool=Off")
    }
    if ($noaio)
    {
      $generateArgs += @("-DWITH_ASYNC:Bool=Off")
    }
    if ($noaiointrinsics)
    {
      $generateArgs += @("-DDONT_USE_AIO_INTRINSICS:Bool=On")
    }
    if ($lto)
    {
        $generateArgs += @("-DRELEASE_LTO:Bool=On")
    }
    if ($debugWall)
    {
        $generateArgs += @("-DDEBUG_WALL:Bool=On")
    }
    if ($cppcheck)
    {
        $generateArgs += @("-DENABLE_CPPCHECK:Bool=On")
    }
    if (![String]::IsNullOrEmpty($cpack_suffix))
    {
        $generateArgs += @("-DSIMH_PACKAGE_SUFFIX:Bool=${cpack_suffix}")
    }

    $buildArgs     =  @("--build", "${buildDir}", "--config", "${config}")
    if ($parallel)
    {
      $buildArgs += "--parallel"
    }
    if ($verbose)
    {
      $buildArgs += "--verbose"
    }
    if ($windeprecation)
    {
        $buildArgs += "-DWINAPI_DEPRECATION:Bool=TRUE"
    }
    if (![String]::IsNullOrEmpty($target)) {
        foreach ($targ in $target) {
          $buildArgs += @("--target", "$targ")
        }
    }
    
    $buildSpecificArgs = @()
    if ($flavor -eq "mingw" -and $parallel)
    {
      ## Limit the number of parallel jobs mingw32-make can spawn. Otherwise
      ## it'll overwhelm the machine.
      $buildSpecificArgs += @("-j",  "8")
    }
}

$exitval = 0

foreach ($phase in $scriptPhases) {
    $savedPATH = $env:PATH
    $argList = @()
    $phaseCommand = "Write-Output"

    switch -exact ($phase)
    {
        "generate" {
            $generateArgs += @("-S", ${simhTopDir})
            $generateArgs += @("-B", ${buildDir})

            Write-Host "** ${scriptName}: Configuring and generating"

            $phaseCommand = ${cmakeCmd}
            $argList = $generateArgs
        }

        "build" {
            Write-Host "** ${scriptName}: Building simulators."

            $phaseCommand = ${cmakeCmd}
            $argList = $buildArgs + $buildSpecificArgs
        }

        "test" {
            Write-Host "** ${scriptName}: Testing simulators."

            ## CTest arguments:
            $testArgs = @("-C", $config, "--timeout", $ctestTimeout, "-T", "test",
                          "--output-on-failure")

            ## Output gets confusing (and tests can time out when executing in parallel)
            ## if ($parallel)
            ## {
            ##     $testArgs += @("--parallel", $ctestParallel)
            ## }

            if ($verbose)
            {
                $testArgs += @("--verbose")
            }

            if (![String]::IsNullOrEmpty($target)) {
                $tests = "simh-(" + ($target -join "|") + ")`$"
                $testArgs += @("-R", $tests)
            }
         
            $phaseCommand = ${ctestCmd}
            $argList = $testArgs

            $env:PATH = $modPath

            $depTopDir = $(& $cmakeCmd -L -N ${buildDir} | Select-String "SIMH_DEP_TOPDIR")
            if ($depTopDir) {
                ## RHS of the cached variable's value.
                $depTopDir = $depTopDir.Line.Split('=')[1]
                $env:PATH =  "${depTopdir}\bin;${env:PATH}"
            }
        }

        "install" {
            Write-Host "** ${scriptName}: Installing simulators."

            $installPrefix = $(& $cmakeCmd -L -N ${buildDir} | Select-String "CMAKE_INSTALL_PREFIX")
            $installPrefix = $installPrefix.Line.Split('=')[1]
            $installPath = $installPrefix

            Write-Host "** ${scriptName}: Install directory ${installPath}"
            if (!(Test-Path -Path ${installPath}))
            {
                Write-Host "** ${scriptName}: Creating ${installPath}"
                New-Item -${installPath} -ItemType Directory -ErrorAction SilentlyContinue
            }

            $phaseCommand = ${cmakeCmd}
            $argList = @( "--install", "${buildDir}", "--config", "${config}")
        }
    }

    try {
        Push-Location ${buildDir}
        Write-Host "** ${phaseCommand} $(Quote-Args ${argList})"
        & $phaseCommand @arglist
        if ($LastExitCode -gt 0) {
            $printPhase = (Get-Culture).TextInfo.ToTitleCase($phase)
            Write-Error $("${printPhase} phase exited with non-zero status: " + $LastExitCode)
            exit 1
        }
    }
    catch {
        Write-Host "Error running '${phaseCommand} ${argList}' command: $($_.Exception.Message)" -ForegroundColor Red
        throw $_
    }
    finally {
        Pop-Location
    }

    $env:PATH = $savedPATH
}

exit $exitval
