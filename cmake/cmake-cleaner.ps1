## 


param (
    ## The build environment's flavor, e.g., "vs2022" for Visual Studio 2022.
    [Parameter(Mandatory=$true)]
    [string] $flavor         = "",

    ## The target build configuration. Valid values: "Release", "Debug" and
    ## "RelWithDebInfo"
    [Parameter(Mandatory=$true)]
    [string] $config         = ""
)

$scriptName = $(Split-Path -Leaf $PSCommandPath)

write-host ${flavor}
if ([String]::IsNullOrEmpty($flavor)) {
  Write-Error "${scriptName}: Build flavor (vs2022, vs2019, ...) not set. Exiting."
  exit 1
}

if ([String]::IsNullOrEmpty($config)) {
  Write-Error "${scriptName}: Build configuration (Debug, RelWithDebInfo, Release) not set. Exiting."
  exit 1
}

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

if (Test-Path -Path ${simhTopDir}\cmake\build-${flavor}) {
  Write-Host "** ${scriptName}: Removing ${simhTopDir}\cmake\build-${flavor}"
  Remove-Item -recurse -force "${simhTopDir}\cmake\build-${flavor}"
}

if (Test-Path -Path ${simhTopDir}\BIN\Win32\${config}) {
  Write-Host "** ${scriptName}: Removing ${simhTopDir}\BIN\Win32\${config}"
  Remove-Item -recurse -force "${simhTopDir}\BIN\Win32\${config}"
}

if (Test-Path -Path ${env:LOCALAPPDATA}\vcpkg\archives) {
  Write-Host "** ${scriptName}: Removing vcpkg archive ${env:LOCALAPPDATA}\vcpkg\archives"
  Remove-Item -recurse -force "${env:LOCALAPPDATA}\vcpkg\archives"
}

Write-Host "** ${scriptName}: Done."

exit 0
