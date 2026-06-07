:: Copyright (C) 2018 The Qt Company Ltd.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

@echo off

REM We clear INCLUDE and LIB, because we want to obtain pristine values.
REM PATH cannot be cleared, because then the script does not even run,
REM and it would be counterproductive anyway (see toolchain.prf).
set INCLUDE=
set LIB=

call %* || exit 1
REM VS2015 does not set errorlevel in case of failure.
if "%INCLUDE%" == "" exit 1

echo =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=
echo %INCLUDE%
echo %LIB%
echo %PATH%
