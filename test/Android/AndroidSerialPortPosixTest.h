#pragma once

#include "UnitTest.h"

/// Android's QSerialPort over its POSIX backend, built for the host and run against pseudo-terminals.
class AndroidSerialPortPosixTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _writeTimeoutReportsProgress();
    void _unsupportedSettingKeepsItsError();
    void _setsRatesAndParitiesWithoutAConstant();
    void _openRefusesARateOfZeroOrLess_data();
    void _openRefusesARateOfZeroOrLess();
    void _writeToALostDeviceIsAResourceError();
    void _waitForReadyRead();
    void _fullReadBufferLeavesInputWithTheDevice();
    void _closeDiscardsUndeliveredData();
    void _deviceLossIsOneResourceError();
    void _controlLineUnsupported_data();
    void _controlLineUnsupported();
};
