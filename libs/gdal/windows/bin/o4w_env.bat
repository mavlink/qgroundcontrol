REM Make parent of this script location our current directory,
REM converting UNC path to drive letter if needed
pushd %~dp0
cd ..

REM set OSGEO4W_ROOT to short path version
for %%i in ("%CD%") do set OSGEO4W_ROOT=%%~fsi

REM start with clean path
set path=%OSGEO4W_ROOT%\bin;%WINDIR%\system32;%WINDIR%;%WINDIR%\system32\WBem

for %%f in ("%OSGEO4W_ROOT%\etc\ini\*.bat") do call "%%f"

popd
