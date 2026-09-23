@echo off
setlocal EnableDelayedExpansion

echo ===============================================================================
echo            LEAN THORIUM - ROLLBACK TO STOCK THORIUM CONFIGURATION
echo ===============================================================================
echo.

set "THOR_ROOT=E:\Thorium"
set "CR_DIR=%THOR_ROOT%\chromium\src"
set "LEAN_DIR=%THOR_ROOT%\lean_thorium"

if not exist "%CR_DIR%" (
    echo Chromium directory %CR_DIR% does not exist. Nothing to revert.
    exit /b 0
)

pushd "%CR_DIR%"

echo [1/3] Reverting Lean Thorium Patches in reverse order...
for %%p in ("%LEAN_DIR%\patches\*.patch") do (
    echo Reverting: %%~nxp
    git apply -R "%%p" 2>nul || echo Notice: %%~nxp was not applied.
)

echo [2/3] Restoring Stock Thorium GN Arguments...
if exist "%THOR_ROOT%\thorium-src\win_args.gn" (
    copy /Y "%THOR_ROOT%\thorium-src\win_args.gn" "%CR_DIR%\out\thorium\args.gn"
    echo Restored stock win_args.gn.
)

echo [3/3] Regenerating GN Build Files...
gn gen out\thorium

popd

echo.
echo ===============================================================================
echo ROLLBACK COMPLETE: Stock Thorium configuration restored.
echo ===============================================================================

endlocal
exit /b 0
