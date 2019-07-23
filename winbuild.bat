@echo ON

SET "MSVC_PATH=C:\Qt\5.11.0\msvc2015\bin"
SET "QT_PATH=C:\Qt\Tools\QtCreator\bin"

SET PATH=%MSVC_PATH%;%QT_PATH%;%PATH%
ECHO %PATH%

call vcvarsall.bat

del /F /Q BUILD
mkdir build
cd build
call "%MSVC_PATH%/qmake.exe" CONFIG-=debug_and_release CONFIG+=installer ../qgroundcontrol.pro
jom
cd ..
