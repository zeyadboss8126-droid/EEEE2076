@echo off
setlocal enabledelayedexpansion

:: ── Configuration — adjust paths to match your machine ─────────────────────
set "SRC_DIR=%~dp0.."
set "BUILD_DIR=%SRC_DIR%\build"
set "DEPLOY_DIR=%BUILD_DIR%\deploy"
set "VTK_BIN=C:\Program Files (x86)\VTK\bin"
set "QT_BIN=C:\Qt\6.5.3\msvc2019_64\bin"
set "OPENVR_BIN=C:\OpenVR\bin\win64"
set "INNO_SETUP=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
:: ───────────────────────────────────────────────────────────────────────────

echo.
echo ============================================================
echo  McLaren Senna VR Viewer — Windows Installer Builder
echo ============================================================
echo.

:: Step 1 — CMake configure + build
echo [1/6] Configuring and building Release...
cmake -B "%BUILD_DIR%" -S "%SRC_DIR%" -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% NEQ 0 ( echo ERROR: cmake configure failed & pause & exit /b 1 )

cmake --build "%BUILD_DIR%" --config Release
if %ERRORLEVEL% NEQ 0 ( echo ERROR: cmake build failed & pause & exit /b 1 )

:: Step 2 — Create deploy staging area
echo.
echo [2/6] Staging deploy folder...
if not exist "%DEPLOY_DIR%" mkdir "%DEPLOY_DIR%"
copy /Y "%BUILD_DIR%\Release\%~nx0" "%DEPLOY_DIR%\" >nul 2>&1
copy /Y "%BUILD_DIR%\Release\Worksheet6.exe" "%DEPLOY_DIR%\"
if %ERRORLEVEL% NEQ 0 ( echo ERROR: could not find Worksheet6.exe & pause & exit /b 1 )

:: Step 3 — Qt runtime (windeployqt copies Qt DLLs + platform plugins)
echo.
echo [3/6] Running windeployqt...
"%QT_BIN%\windeployqt.exe" --release --no-translations "%DEPLOY_DIR%\Worksheet6.exe"
if %ERRORLEVEL% NEQ 0 ( echo WARNING: windeployqt returned a non-zero exit code )

:: Step 4 — VTK DLLs
echo.
echo [4/6] Copying VTK DLLs...
for %%F in ("%VTK_BIN%\vtk*.dll") do (
    copy /Y "%%F" "%DEPLOY_DIR%\" >nul
)

:: Step 5 — OpenVR DLL + STL model + VR bindings
echo.
echo [5/6] Copying OpenVR, STL model, and VR bindings...
copy /Y "%OPENVR_BIN%\openvr_api.dll" "%DEPLOY_DIR%\"
copy /Y "%SRC_DIR%\Mclaren_Senna_clean_centered.stl" "%DEPLOY_DIR%\"
if not exist "%DEPLOY_DIR%\vrbindings" mkdir "%DEPLOY_DIR%\vrbindings"
xcopy /E /Y /Q "%SRC_DIR%\vrbindings\*" "%DEPLOY_DIR%\vrbindings\"

:: Step 6 — Compile the Inno Setup script
echo.
echo [6/6] Building installer with Inno Setup...
if not exist "%INNO_SETUP%" (
    echo ERROR: Inno Setup not found at "%INNO_SETUP%"
    echo        Download from https://jrsoftware.org/isinfo.php and install it.
    pause & exit /b 1
)
"%INNO_SETUP%" "%~dp0windows_installer.iss"
if %ERRORLEVEL% NEQ 0 ( echo ERROR: Inno Setup compilation failed & pause & exit /b 1 )

echo.
echo ============================================================
echo  Done!  Installer is in installer\Output\
echo ============================================================
pause
