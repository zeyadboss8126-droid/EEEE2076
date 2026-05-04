@echo off
setlocal enabledelayedexpansion

set "SRC_DIR=%~dp0.."
set "BUILD_DIR=%SRC_DIR%\build"
set "DEPLOY_DIR=%BUILD_DIR%\deploy"

set "QT_ROOT=C:\Qt\6.10.2\msvc2022_64"
set "QT_BIN=%QT_ROOT%\bin"
set "Qt6_DIR=%QT_ROOT%\lib\cmake\Qt6"

set "VTK_DIR=C:\Program Files (x86)\VTK\lib\cmake\vtk-9.6"
set "VTK_BIN=C:\Program Files (x86)\VTK\bin"

set "OPENVR_BIN=C:\OpenVR\bin\win64"
set "INNO_SETUP=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"

set "PATH=%QT_BIN%;%VTK_BIN%;%PATH%"

echo Cleaning old build...
rmdir /s /q "%BUILD_DIR%" 2>nul

echo.
echo [1/6] Configuring CMake...
cmake -B "%BUILD_DIR%" -S "%SRC_DIR%" ^
  -G "Visual Studio 18 2026" -A x64 ^
  -DQt6_DIR="%Qt6_DIR%" ^
  -DVTK_DIR="%VTK_DIR%"

if errorlevel 1 (
    echo ERROR: cmake configure failed
    pause
    exit /b 1
)

echo.
echo [2/6] Building Release...
cmake --build "%BUILD_DIR%" --config Release

if errorlevel 1 (
    echo ERROR: cmake build failed
    pause
    exit /b 1
)

echo.
echo [3/6] Creating deploy folder...
rmdir /s /q "%DEPLOY_DIR%" 2>nul
mkdir "%DEPLOY_DIR%"

copy /Y "%BUILD_DIR%\Release\Worksheet6.exe" "%DEPLOY_DIR%\"
if errorlevel 1 (
    echo ERROR: could not find Worksheet6.exe
    pause
    exit /b 1
)

echo.
echo [4/6] Running windeployqt...
"%QT_BIN%\windeployqt.exe" --release --force --no-translations "%DEPLOY_DIR%\Worksheet6.exe"

if errorlevel 1 (
    echo ERROR: windeployqt failed
    pause
    exit /b 1
)

echo.
echo [5/6] Copying VTK/OpenVR/assets...
copy /Y "%VTK_BIN%\vtk*.dll" "%DEPLOY_DIR%\" >nul
copy /Y "%OPENVR_BIN%\openvr_api.dll" "%DEPLOY_DIR%\"
copy /Y "%SRC_DIR%\Mclaren_Senna_clean_centered.stl" "%DEPLOY_DIR%\"

if not exist "%DEPLOY_DIR%\vrbindings" mkdir "%DEPLOY_DIR%\vrbindings"
xcopy /E /Y /Q "%SRC_DIR%\vrbindings\*" "%DEPLOY_DIR%\vrbindings\"

echo.
echo [6/6] Building installer...
"%INNO_SETUP%" "%~dp0windows_installer.iss"

if errorlevel 1 (
    echo ERROR: Inno Setup failed
    pause
    exit /b 1
)

echo.
echo Done. Installer is in installer\Output\
pause
