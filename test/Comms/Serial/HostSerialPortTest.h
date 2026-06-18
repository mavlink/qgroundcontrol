// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#pragma once

#include "UnitTest.h"

// Validates HostSerialPort (wraps QSerialPort) — the host build's QGCSerialPort impl, and the
// Android "/dev/tty*" direct-UART path. AndroidSerialPort is exercised on Android-only builds.
class HostSerialPortTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _construct_noPortBaseline();
    void _invalidConfigSetsError();
    void _ptyLoopback_readsAndWrites();
};
