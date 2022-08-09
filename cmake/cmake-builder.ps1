## cmake-builder.ps1
##
## CMake-based configure and build script for simh using MSVC and MinGW-W64.
##
## - The default builds SIMH for Visual Studio 2022 in the Release configuration.
## - The "-flavor" command line parameter switches between the various MSVC and
##   MinGW builds.
## - The "-config" command line parameter switches between Debug and Release
## - For all options, "-help" is available.
##
## The thumbnail overview: Create a subdirectory, run CMake to configure the
## build environment, build dependency libraries and then reconfigure and build
## the simulators.
##
## This script produces a "Batteries Included" set of simulators, which implies that
## Npcap is installed.
##
## Author: B. Scott Michel
## "scooter me fecit"

param (
    [switch] $clean       = $false,
    [switch] $help        = $false,
    [string] $flavor      = "vs2022",
    [string] $config      = "Release",
    [switch] $nonetwork   = $false,
    [switch] $notest      = $false,
    [switch] $noinstall   = $false,
    [switch] $parallel    = $false,
    [switch] $generate    = $false,
    [switch] $regenerate  = $false,
    [switch] $testonly    = $false,
    [switch] $installOnly = $false
    ## [switch] $allInOne    = $false
)

$scriptName = $(Split-Path -Leaf $PSCommandPath)

function Show-Help
{
    @"
Configure and build simh's dependencies and simulators using the Microsoft
Visual Studio C compiler or MinGW-W64-based gcc compiler.

cmake/build--vs* subdirectories: MSVC build products and artifacts
cmake/build-mingw subdirectory:  MinGW-W64 products and artifacts
cmake/build-ninja subdirectory:  Ninja builder products and artifacts

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

-flavor (2022|vs2022)  Generate build environment for Visual Studio 2022 (default)
-flavor (2019|vs2019)  Generate build environment for Visual Studio 2019
-flavor (2017|vs2017)  Generate build environment for Visual Studio 2017
-flavor (2015|vs2015)  Generate build environment for Visual Studio 2015
-flavor (2013|vs2013)  Generate build environment for Visual Studio 2013
-flavor (2012|vs2012)  Generate build environment for Visual Studio 2012
-flavor mingw          Generate build environment for MinGW GCC/mingw32-make
-flavor ninja          Generate build environment for MinGW GCC/ninja

-config Release        Build dependencies and simulators for Release (optimized) (default)
-config Debug          Build dependencies and simulators for Debug

-help                  Output this help.
"@

    exit 0
}


## CMake generator info:
class GeneratorInfo
{
    [string]  $Generator
    [string[]]$ArchArgs

    GeneratorInfo([string]$gen, [string[]]$arch)
    {
        $this.Generator = $gen
        $this.ArchArgs  = $arch
    }
}

## Yes, I made a mistake by using years for VS instead of something more
## sensible.
$cmakeCanonicalFlavors = @{
    "2022" = "vs2022";
    "2019" = "vs2019";
    "2017" = "vs2017";
    "2015" = "vs2015";
    "2013" = "vs2013";
    "2012" = "vs2012"
}


$cmakeGenMap = @{
    "vs2022" = [GeneratorInfo]::new("Visual Studio 17 2022", @("-A", "Win32"));
    "vs2019" = [GeneratorInfo]::new("Visual Studio 16 2019", @("-A", "Win32"));
    "vs2017" = [GeneratorInfo]::new("Visual Studio 15 2017", @());
    "vs2015" = [GeneratorInfo]::new("Visual Studio 14 2015", @());
    "vs2013" = [GeneratorInfo]::new("Visual Studio 12 2013", @());
    "vs2012" = [GeneratorInfo]::new("Visual Studio 11 2012", @());
    "mingw"  = [GeneratorInfo]::new("MinGW Makefiles", @());
    "ninja"  = [GeneratorInfo]::new("Ninja", @())
}


function Get-CanonicalFlavor([string]$flavor)
{
    $canonicalFlavor = $cmakeCanonicalFlavors[${flavor}]
    if ($null -eq $canonicalFlavor) {
        $canonicalFlavor = $flavor
    }

    return $canonicalFlavor
}

function Get-GeneratorInfo([string]$flavor)
{
    $canonicalFlavor = Get-CanonicalFlavor $flavor
    return $cmakeGenMap[$canonicalFlavor]
}


## Output help early and exit.
if ($help)
{
    Show-Help
}

### CTest params:
## timeout is 180 seconds
$ctestParallel = "4"
$ctestTimeout  = "300"

## Sanity checking: Check that utilities we expect exist...
## CMake: Save the location of the command because we'll invoke it later. Same
## with CTest
$cmakeCmd = $(Get-Command -Name cmake.exe -ErrorAction Ignore).Path
$ctestCmd = $(Get-Command -Name ctest.exe -ErrorAction Ignore).Path
if ($cmakeCmd.Length -gt 0)
{
    Write-Output "** ${scriptName}: cmake is '${cmakeCmd}'"
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

if (!$testOnly)
{
    ## bison/winbison and flex/winflex: Just make sure they exist.
    if (!$nonetwork) {
      $haveBison = $false
      $haveFlex  = $false
      foreach ($i in @("bison", "winbison")) {
        if ($(Get-Command $i -ErrorAction Ignore).Path.Length -gt 0) { $haveBison = $true }
      }

      foreach ($i in @("flex",  "winflex")) {
        if ($(Get-Command $i -ErrorAction Ignore).Path.Length -gt 0) { $haveFlex  = $true }
      }

      if (!$haveBison)
      {
          @"
      !! ${scriptName} error:

      Did not find 'bison' or 'winbison'. Please ensure you have installed bison or
      winbison and that your PATH environment variables references the directory in
      which it was installed.
"@
      }

      if (!$haveFlex)
      {
          @"
      !! ${scriptName} error:

      Did not find 'flex' or 'winflex'. Please ensure you have installed flex or
      winflex and that your PATH environment variables references the directory in
      which it was installed.
"@
      }

      if (!$haveBison -or !$haveFlex)
      {
        exit 1
      }
    }

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

    Note: 'mingw32-make' is part of the MinGW-W64 software ecosystem. You may need
    to open a MSYS or MinGW64 terminal window and use 'pacman' to install it.

    Alternatively, if you use the Scoop package manager, 'scoop install gcc'
    will install this as part of the current GCC compiler instalation.
"@
            exit 1
        }
    }

    if (!$nonetwork -and !(Test-Path -Path "C:\Windows\System32\Npcap"))
    {
        ## Note: Windows does redirection under the covers to make the 32-bit
        ## version of Npcap appear in the C:\Windows\System32 directory. But
        ## it should be sufficient to detect the 64-bit runtime because both
        ## get installed.
        @"
    !! ${scriptName} error:

    Did not find the Npcap packet capture runtime. Please install it for simulator
    network support:

        https://nmap.org/npcap/

    Or invoke this script with the "-nonetwork" flag to disable networking support.

"@
    }
}

## Validate the requested configuration.
if (!@("Release", "Debug").Contains($config))
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
    "** ${scriptName}: Removed ${git_usrbin} from PATH (Git MinGW problem)"
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
    "** ${scriptName}: Removed cmake\dependencies 'bin' directories from PATH."
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
    "** ${scriptName}: SIMH top-level source directory is ${simhTopDir}"
}

$flavor = $(Get-CanonicalFlavor $flavor)
$buildDir  = "${simhTopDir}\cmake\build-${flavor}"
$genInfo = $(Get-GeneratorInfo $flavor)
if ($null -eq $genInfo)
{
    Write-Output ""
    Write-Output "!! ${scriptName}: Unrecognized build flavor '${flavor}'."
    Write-Output ""
    Show-Help
}

if ($regenerate)
{
  $generate = $true;
}

if ($testOnly)
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
  $scriptPhases = @( "generate", "build", "test", "install")
  if ($notest)
  {
      $scriptPhases = $scriptPhases | Where-Object { $_ -ne 'test' }
  }
  if ($noinstall)
  {
      $scriptPhases = $scriptPhases | Where-Object { $_ -ne 'install' }
  }
}

if (($scriptPhases -contains "generate") -or ($scriptPhases -contains "build"))
{
    ## Clean out the build subdirectory
    if ((Test-Path -Path ${buildDir}) -and $clean)
    {
        "** ${scriptName}: Removing ${buildDir}"
        Remove-Item -recurse -force -Path ${buildDir} -ErrorAction Continue | Out-Null
    }

    if (!(Test-Path -Path ${buildDir}))
    {
        "** ${scriptName}: Creating ${buildDir} subdirectory"
        New-Item -Path ${buildDir} -ItemType Directory | Out-Null
    }
    else
    {
        "** ${scriptName}: ${buildDir} exists."
    }

    ## Need to regenerate?
    if ($regenerate)
    {
      Remove-Item          -Force -Path ${buildDir}/CMakeCache.txt -ErrorAction Continue | Out-Null
      Remove-Item -Recurse -Force -Path ${buildDir}/CMakeFiles     -ErrorAction Continue | Out-Null
    }
   
    ## Where we do the heaving lifting:
    $generateArgs = @("-G", $genInfo.Generator, "-DCMAKE_BUILD_TYPE=${config}",
        "-Wno-dev", "--no-warn-unused-cli"
    )
    $generateArgs = $generateArgs + $genInfo.ArchArgs
    if ($nonetwork)
    {
        $generateArgs += @("-DWITH_NETWORK:Bool=Off")
    }
    ## if ($allInOne)
    ## {
    ##     $generateArgs += @("-DALL_IN_ONE:Bool=TRUE")
    ## }
    $generateArgs += ${simhTopDir}

    $buildArgs     =  @("--build", ".", "--config", "${config}")
    if ($parallel)
    {
      $buildArgs += "--parallel"
    }

    $buildSpecificArgs = @()
    if ($flavor -eq "mingw" -and $parallel)
    {
      ## Limit the number of parallel jobs mingw32-make can spawn. Otherwise
      ## it'll overwhelm the machine.
      $buildSpecificArgs += @("-j",  "8")
    }

    ## CTest arguments:
    $testArgs = @("-C", $config, "--timeout", $ctestTimeout, "-T", "test",
                    "--output-on-failure")
    if ($parallel)
    {
        $testArgs += @("--parallel", $ctestParallel)
    }

    $exitval = 0

    $env:PATH = $modPath
    Push-Location ${buildDir}

    try
    {
        "** ${scriptName}: Configuring and generating"

        & ${cmakeCmd} ${generateArgs} 2>&1
        $lec = $LastExitCode
        if ($lec -gt 0) {
            "==== Last exit code ${lec}"
            "** ${scriptName}: Configuration errors. Exiting."
            exit 1
        }
        if ($scriptPhases -contains "build")
        {
            "** ${scriptName}: Building simulators."
            & ${cmakeCmd} ${buildArgs} -- ${buildSpecificArgs}
            $lec = $LastExitCode
            if ($lec -gt 0) {
                "==== Last exit code ${lec}"
                "** ${scriptName}: Build errors. Exiting."
                exit 1
            }
        }
        else
        {
            "** ${scriptName}: Generated build environment in $(Get-Location)"
            exit 0
        }
    }
    catch
    {
        "** ${scriptName}: Caught error, exiting."
        Format-List * -force -InputObject $_
        $exitval = 1
    }
    finally
    {
        Pop-Location
        $env:PATH = $origPath
    }
}

if ($scriptPhases -contains "test")
{
    try {
        ## Let's test our results...
        ##
        ## If cmake failed, ctest will also fail. That's OK.
        ##
        ## Note: We're in the build directory already, so we can prepend $(Get-Location) for the
        ## full path to the build directory, then normalize it.
        Push-Location ${buildDir}
        $env:PATH = $modPath

        $currentPath = $env:PATH
        $depTopDir = $(& $cmakeCmd -L -N ${buildDir} | Select-String "SIMH_DEP_TOPDIR")
        if ($depTopDir) {
            ## RHS of the cached variable's value.
            $depTopDir = $depTopDir.Line.Split('=')[1]
            $env:PATH =  "${depTopdir}\bin;${env:PATH}"
            & $ctestCmd ${testArgs}
            if ($LastExitCode -gt 0) {
                "** ${scriptName}: Tests failed. Exiting."
                exit 1
            }
        }
        $env:PATH = $currentPath
    }
    catch
    {
        "** ${scriptName}: Caught error, exiting."
        Format-List * -force -InputObject $_
        $exitval = 1
    }
    finally
    {
        Pop-Location
        $env:PATH = $origPath
    }
}

if ($scriptPhases -contains "install")
{
    try
    {
        $installPrefix = $(& $cmakeCmd -L -N ${buildDir} | Select-String "CMAKE_INSTALL_PREFIX")
        $installPrefix = $installPrefix.Line.Split('=')[1]
        $installPath = $installPrefix

        "** ${scriptName}: Install directory ${installPath}"
        if (!(Test-Path -Path ${installPath}))
        {
            "** ${scriptName}: Creating ${installPath}"
            New-Item -${installPath} -ItemType Directory -ErrorAction SilentlyContinue
        }

        "** ${scriptName}: Installing simulators."
        & ${cmakeCmd} --install ${buildDir} --config ${config}
        if ($LastExitCode -gt 0) {
            "==== Last exit code ${lec}"
            "** ${scriptName}: Install errors. Exiting."
            exit 1
        }
    }
    finally
    {
        Pop-Location
        $env:PATH = $origPath
    }
}

exit $exitval
