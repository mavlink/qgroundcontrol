:: echo off

::
:: Script assumes that the right QT bin folder is in the PATH env var
::

echo Before calling scripts
:: Prepare the environment for windeployqt
call qtenv2.bat
:: Populate VS2015 script variables
call "%VC_VARS_SCRIPT%" x86

SETLOCAL

if not "%1" == "" (
    set TARGET_DIR=%1
) else (
    set TARGET_DIR=release
)

if not "%2" == "" (
    set BUILD_DIR=%2
) else (
    set BUILD_DIR=build
)

:: These batch scripts change the current directory
cd /d %WORKSPACE%
echo After calling scripts
cd

if not exist ".\%BUILD_DIR%\" mkdir %BUILD_DIR%
cd %BUILD_DIR%
qmake -r "%WORKSPACE%\qgroundcontrol.pro" -spec win32-msvc CONFIG+="%QGC_CONFIG%"
if %errorlevel%==0 (
    dir
    C:\Qt\Tools\QtCreator\bin\jom /f Makefile
) else (
    echo "FATAL: Failed to build. qmake returned error level: %errorlevel%"
)

ENDLOCAL
