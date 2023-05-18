$vsFlavor=$args[0]
$vsSetupDir="C:\Program Files (x86)\Microsoft Visual Studio\installer"
$vswhere="${vsSetupDir}\vswhere.exe"
$vsinstaller="${vsSetupDir}\vs_installer.exe"

$vstudios = @{
  "vs2017" = @{
    names = @{
      "Visual Studio Enterprise 2019" = "Microsoft.VisualStudio.Product.Enterprise"
      "Visual Studio Professional 2019" = "Microsoft.VisualStudio.Product.Professional"
      "Visual Studio Community 2019" = "Microsoft.VisualStudio.Product.Community"
    }
    channelId = "VisualStudio.15.Release"
    v141toolkit = @( "Microsoft.VisualStudio.Component.WinXP" )
  }
  "vs2019" = @{
    names = @{
      "Visual Studio Enterprise 2019" = "Microsoft.VisualStudio.Product.Enterprise"
      "Visual Studio Professional 2019" = "Microsoft.VisualStudio.Product.Professional"
      "Visual Studio Community 2019" = "Microsoft.VisualStudio.Product.Community"
    }
    displayName = "Visual Studio Community 2019"
    channelId = "VisualStudio.16.Release"
    v141toolkit = @( "Microsoft.VisualStudio.Component.VC.v141.x86.x64", "Microsoft.VisualStudio.Component.WinXP" )
  }
  "vs2022" = @{
    names = @{
      "Visual Studio Enterprise 2022" = "Microsoft.VisualStudio.Product.Enterprise"
      "Visual Studio Professional 2022" = "Microsoft.VisualStudio.Product.Professional"
      "Visual Studio Community 2022" = "Microsoft.VisualStudio.Product.Community"
    }
    installedChannelId = "VisualStudio.17.Release"
    v141toolkit = @( "Microsoft.VisualStudio.Component.VC.v141.x86.x64", "Microsoft.VisualStudio.Component.WinXP" )
  }
}

if ($args.length -eq 0) {
  "Womabt!!"
  exit 1
}

$wantedVS = $vstudios[$args[0]]
if ($wantedVS -eq $null) {
  "Wibbles."
  exit 1
}

Set-PSDebug -Trace 1

if (Test-Path -Path $vsinstaller) {
  if (Test-Path -Path $vswhere) {
    $vsinfo=$(& $vswhere -format json | ConvertFrom-JSON)
    foreach ($vs in $vsinfo) {
      $productId = $wantedVS.names[$vs.displayName]
      if ($productId -ne $null) {
        Write-Output $("Found: " + $vs.displayName)

        $args =  @( "modify", "--quiet")
        $args += @( "--channelId", $wantedVS.installedChannelId )
        $args += @( "--productId", $productId )

        foreach ($c in $wantedVS.v141toolkit) {
          $args += @( "--add", $c )
        }

        Write-Output $( ( @("Executing: ", $vsinstaller) + $args ) -join " " )

        $proc = Start-Process -Verbose -NoNewWindow -PassThru $vsinstaller -ArgumentList $args
        $proc.WaitForExit()

        $proc = Start-Process -NoNewWindow -PassThru $vsinstaller -ArgumentList @("export", "--channelId", $wantedVS.installedChannelId, "--productId", $productId, "--config", "vsinstall.after", "--quiet")
        $proc.WaitForExit()

        if (Test-Path -Path .\vsinstall.after) {
          Get-Content .\vsinstall.after
        }
      }
    }
  } else {
    Write-Ouput @("vswhere not found or available. ", $vswhere) -join " "
  }
} else {
  Write-Ouput @("VS installer not found or available. ", $vsinstaller) -join " "
}
