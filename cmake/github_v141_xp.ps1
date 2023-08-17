<#
.SYNOPSIS
Install Visual Studio's 'v141_xp' XP toolkit on Github.

.DESCRIPTION
Update the existing Github Visual Studio installation in-place. This script
will iterate through all VS installations identified by the `vswhere` utility.

This script is Grey Magic. The `vs_installer` installer utility exits almost
immediately if you run it from the command line. `vs_installer`'s subprocesses
are reasonably well known, which allows this script to query the processes and
wait for them to terminate.

GitHub Actions does something strange with stdout and stderr, so you won't get any
indication of failure or output that shows you that something is happening.  

The waiting code was adapted from the Chocolatey VS installer scripts (Wait-VSIstallerProcesses).

Ref: https://github.com/jberezanski/ChocolateyPackages/blob/master/chocolatey-visualstudio.extension/extensions/Wait-VSInstallerProcesses.ps1
#>

$exitcode = $null
$vswhere = "${env:ProgramFiles} (x86)\Microsoft Visual Studio\Installer\vswhere"
$vsinstaller = "${env:ProgramFiles} (x86)\Microsoft Visual Studio\Installer\vs_installer.exe"
$vsInstallOut = "$env:TEMP\vsinstall-out.txt"
$vsInstallErr = "$env:TEMP\vsinstall-err.txt"

& ${vswhere} -property installationPath | Foreach-Object -process {
    # Save the installlation path for the current iteration
    $installPath = $_

    # Make sure that previously running installers have all exited.
    Write-Debug ('Looking for previously running VS installer processes')
    $lazyQuitterProcessNames = @('vs_installershell', 'vs_installerservice')
    do
    {
        $lazyQuitterProcesses = Get-Process -Name $lazyQuitterProcessNames -ErrorAction SilentlyContinue | `
                                Where-Object { $null -ne $_ -and -not $_.HasExited }
        $lazyQuitterProcessCount = ($lazyQuitterProcesses | Measure-Object).Count
        if ($lazyQuitterProcessCount -gt 0)
        {
            try
            {
                $lazyQuitterProcesses | Sort-Object -Property Name, Id | ForEach-Object { '[{0}] {1}' -f $_.Id, $_.Name } | Write-Debug
                $lazyQuitterProcesses | Wait-Process -Timeout 10 -ErrorAction SilentlyContinue
            }
            finally
            {
                $lazyQuitterProcesses | ForEach-Object { $_.Dispose() }
                $lazyQuitterProcesses = $null
            }
        }
    }
    while ($lazyQuitterProcessCount -gt 0)

    ## Now kick off our install:
    ##
    ## --quiet: Don't open/show UI
    ## --force: Terminate any VS instances forcibly
    ## --norestart: Delay reboot after install, if needed
    ## --installWhileDownloading: Self-explanitory.
    ##
    ## Note: "--wait" doesn't.
    $vsinstallParams = @(
        "modify",
        "--installPath", "$installPath",
        "--add", "Microsoft.VisualStudio.Component.VC.v141.x86.x64",
        "--add", "Microsoft.VisualStudio.Component.WinXP",
        "--quiet", "--force", "--norestart", "--installWhileDownloading"
        )

    & $vsInstaller $vsinstallParams 2> $vsInstallErr > $vsInstallOut

    ## VS installer processes for which we want to wait because installation isn't complete. Yet.
    $vsinstallerProcesses = @( 'vs_bootstrapper', 'vs_setup_bootstrapper', 'vs_installer', 'vs_installershell', `
                               'vs_installerservice', 'setup') 
    $vsInstallerProcessFilter = { $_.Name -ne 'setup' -or $_.Path -like '*\Microsoft Visual Studio\Installer*\setup.exe' }
    do
    {
        Write-Debug ('Looking for running VS installer processes')
        $vsInstallerRemaining = Get-Process -Name $vsinstallerProcesses -ErrorAction SilentlyContinue | `
                                Where-Object { $null -ne $_ -and -not $_.HasExited } | `
                                Where-Object $vsInstallerProcessFilter
        $vsInstallerProcessCount = ($vsInstallerRemaining | Measure-Object).Count
        if ($vsInstallerProcessCount -gt 0)
        {
            try
            {
                Write-Debug "Found $vsInstallerProcessCount running Visual Studio installer processes:"
                $vsInstallerRemaining | Sort-Object -Property Name, Id | ForEach-Object { '[{0}] {1}' -f $_.Id, $_.Name } | Write-Debug

                foreach ($p in $vsInstallerProcesses)
                {
                  [void] $p.Handle # make sure we get the exit code http://stackoverflow.com/a/23797762/266876
                }

                Write-Debug ('Waiting 60 seconds for process(es) to exit...')
                $vsInstallerRemaining | Wait-Process -Timeout 60 -ErrorAction SilentlyContinue

                foreach ($proc in $vsInstallerProcesses)
                {
                  if (-not $proc.HasExited)
                  {
                    continue
                  }
                  if ($null -eq $exitCode)
                  {
                    $exitCode = $proc.ExitCode
                  }
                  if ($proc.ExitCode -ne 0 -and $null -ne $proc.ExitCode)
                  {
                    Write-Debug "$($proc.Name) process $($proc.Id) exited with code $($proc.ExitCode)"
                    if ($exitCode -eq 0)
                    {
                      $exitCode = $proc.ExitCode
                    }
                  }
                }
            }
            finally
            {
                $vsInstallerRemaining | ForEach-Object { $_.Dispose() }
                $vsInstallerRemaining = $null
            }
        }
    }
    while (($vsInstallerStartup -and $vsInstallerStartCount -gt 0) -or $vsInstallerProcessCount -gt 0)
}

if ((Test-Path $vsInstallOut -PathType Leaf) -and (Get-Item $vsInstallOut).length -gt 0kb)
{
  Write-Output "-- vsinstaller output:"
  Get-Content $vsInstallOut
}
else
{
  Write-Debug "-- No vsinstaller output."
}

if ((Test-Path $vsInstallErr -PathType Leaf) -and (Get-Item $vsInstallErr).length -gt 0kb)
{
  Write-Output "-- vsinstaller error output:"
  Get-Content $vsInstallErr
}
else
{
  Write-Debug "-- No vsinstaller error/diag output."
}

if ($null -ne $exitcode -and $exitcode -ne 0)
{
  throw "VS installer exited with non-zero exit status: $exitcode"
}
