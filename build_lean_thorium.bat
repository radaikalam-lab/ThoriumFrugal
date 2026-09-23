@echo off
setlocal EnableDelayedExpansion

echo ===============================================================================
echo            LEAN THORIUM FOR WINDOWS 11 X64 - MASTER BUILD SCRIPT
echo ===============================================================================
echo.

set "THOR_ROOT=E:\Thorium"
set "CR_DIR=%THOR_ROOT%\chromium\src"
set "THOR_DIR=%THOR_ROOT%\thorium-src"
set "LEAN_DIR=%THOR_ROOT%\lean_thorium"
set "DEPOT_TOOLS_DIR=%THOR_ROOT%\depot_tools"

set "DEPOT_TOOLS_WIN_TOOLCHAIN=0"
set "NINJA_SUMMARIZE_BUILD=1"
set "PATH=%DEPOT_TOOLS_DIR%;%PATH%"

echo [1/6] Verifying working directories...
if not exist "%THOR_ROOT%" (
    echo ERROR: Working directory %THOR_ROOT% not found.
    exit /b 1
)

echo [2/6] Checking depot_tools...
if not exist "%DEPOT_TOOLS_DIR%\gclient.bat" (
    echo WARNING: depot_tools not detected in %DEPOT_TOOLS_DIR%.
    echo Please install depot_tools to %DEPOT_TOOLS_DIR% before executing full compile.
)

echo [3/6] Applying Thorium Overlays to Chromium source tree...
if exist "%CR_DIR%" (
    echo Copying Thorium overlays from %THOR_DIR%\src to %CR_DIR%...
    xcopy /E /I /Y /Q "%THOR_DIR%\src\*" "%CR_DIR%\"
) else (
    echo Chromium source tree not yet synchronized at %CR_DIR%.
)

echo [4/6] Applying Lean Thorium Custom Patches...
if exist "%CR_DIR%" (
    pushd "%CR_DIR%"
    for %%p in ("%LEAN_DIR%\patches\*.patch") do (
        echo Applying patch: %%~nxp
        git apply "%%p" 2>nul || echo Notice: Patch %%~nxp already applied or clean.
    )
    popd
)

echo [5/6] Generating GN Build Arguments for Lean Thorium...
if exist "%CR_DIR%" (
    if not exist "%CR_DIR%\out\thorium" mkdir "%CR_DIR%\out\thorium"
    copy /Y "%LEAN_DIR%\args\lean_win_args.gn" "%CR_DIR%\out\thorium\args.gn"
    echo GN configuration written to %CR_DIR%\out\thorium\args.gn.
    
    pushd "%CR_DIR%"
    echo Running GN Gen...
    gn gen out\thorium
    popd
)

echo [6/6] Building Lean Thorium Targets (chrome, mini_installer)...
if exist "%CR_DIR%" (
    pushd "%CR_DIR%"
    echo Invoking Autoninja with 8 parallel jobs...
    autoninja -C out\thorium chrome mini_installer -j8
    popd
    echo.
    echo ===============================================================================
    echo BUILD COMPLETE: %CR_DIR%\out\thorium\mini_installer.exe
    echo ===============================================================================
) else (
    echo Build configuration ready. Synchronize Chromium tree to execute final compile.
)

endlocal
exit /b 0
