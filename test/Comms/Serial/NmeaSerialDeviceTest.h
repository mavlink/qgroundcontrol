// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#pragma once

#include "UnitTest.h"

// Validates NmeaSerialDevice — the worker-threaded, read-only QIODevice that feeds QNmeaPositionInfoSource.
// Exercises the open/close lifecycle and the empty-buffer / read-only contract without real hardware
// (the worker fails to open a bogus port and exits cleanly).
class NmeaSerialDeviceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _construct_baseline();
    void _openClose_lifecycle();
    void _readOnly_notWritable();
};
