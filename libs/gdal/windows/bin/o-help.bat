@echo off
:: Osgeo4w Quick Help -- report available executables in o4w bin directory
::
:: 2012-02-06, Matt Wilkie <maphew@gmail.com>
:: 2018-11-04, Jürgen Fischer <jef@norbit.de>
:: License: Open source MIT, http://www.opensource.org/licenses/mit-license
::
setlocal enabledelayedexpansion

call "%~dp0\o4w_env.bat"

echo.                   -={ OSGeo4W Shell Commands }=-

pushd %OSGEO4W_ROOT%\bin

set c=
for %%a in (*.exe *.com *.bat) do (
	set _s=%%a
	set _s=!_s:.exe=!
	set _s=!_s:.com=!
	set _s=!_s:.bat=!
	if "!c!"=="" (
		set _s=!_s!                                            
		set _s=!_s:~0,40!
		set c=!_s!
	) else (
		echo.  !c!!_s!
		set c=
	)
)
if not "%c%"=="" echo.  %c%

popd

rem Report gdal version, if present
for %%g in (gdalinfo.exe) do if not "%%~dp$PATH:g"=="" (echo. & gdalinfo --version)
