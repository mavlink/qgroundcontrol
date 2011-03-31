@echo off
REM install qmake feature file pointing to the current directory

if not defined QTDIR goto err
echo QSERIALPORT_INCDIR = "%CD%\include" > qserialport.prf
echo QSERIALPORT_LIBDIR = "%CD%\lib" >> qserialport.prf
type qserialport.prf.in >> qserialport.prf
copy qserialport.prf "%QTDIR%\mkspecs\features"

echo Installed qserialport.prf as a qmake feature.
goto end

:err
echo Error: QTDIR not set (example: set QTDIR=C:\Qt\4.5.2).
goto end

:end
