# SIMH pre-built binaries

Welcome to SIMH's pre-built binaries!

- `.deb`: Debian packages for Linux. To install:

    ```
    $ sudo dpkg -i <package file>
    ```

- `.zip`: ZIP-ed executable archive. Use your favorite or appropriate ZIP
  software to unzip the archive. The `simh-<version>/bin` subdirectory is the
  location of the simulators.

- `.exe`: Nullsoft Scriptable Install System (NSIS)-created Windows installer.

  NOTE: The executble is not code-signed. DO NOT DOUBLE CLICK ON THE EXECUTABLE
  TO INSTALL. If you do, you are likely to get a Windows Defender popup box that
  will prevent you from installing SIMH. Instead, use a CMD or PowerShell
  command window and execute the `.exe` from the command line prompt. For
  example, to install `simh-4.1.0-win32-native.exe`:

    ```
    ## PowerShell:
    PS> .\simh-4.1.0-win32-native

    ## CMD:
    > .\simh-4.1.0-win32-native
    ```

- `.msi`: WiX toolkit-created Windows MSI installer.

  These MSI files are not code-signed. DO NOT DOUBLE CLICK ON THE EXECUTABLE TO
  INSTALL. If you do, you are likely to get a Windows Defender popup box that
  will prevent you from installing SIMH. Instead, use a CMD or PowerShell command
  window to manually invoke `msiexec` to install SIMH. From either a PowerShell
  or CMD command window:

    ```
    > msiexec /qf /i simh-4.1.0-win32-native.msi
    ```
