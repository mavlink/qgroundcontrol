echo on

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

if not exist ".\%BUILD_DIR%\" mkdir %BUILD_DIR%
cd %BUILD_DIR%
qmake -r "%WORKSPACE%/qgroundcontrol.pro" -spec win32-msvc CONFIG+="%QGC_CONFIG%"
if %errorlevel%==0 (
    nmake
) else (
    echo "FATAL: Failed to build. qmake returned error level: %errorlevel%"
)

ENDLOCAL
