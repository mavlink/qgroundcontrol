:: echo off

:: Populate VS2015 script variables
call "%VC_VARS_SCRIPT%" x86

mkdir build
cd build
qmake -r "%WORKSPACE%/qgroundcontrol.pro" -spec win32-msvc CONFIG+="%QGC_CONFIG%" 
if %errorlevel%==0 (
    nmake
) else (
    echo "FATAL: Failed to build. qmake returned error level: %errorlevel%"
)
if %errorlevel%==0 (
    copy /Y .\release\*-installer.exe ..\
) else (
    echo "WARNING: Failed to copy the installer output. copy returned error level: %errorlevel%"
)
