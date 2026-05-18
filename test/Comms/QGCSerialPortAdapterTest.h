// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#pragma once

#include "UnitTest.h"

// Validates QGCSerialPortAdapter's runtime backend dispatch. Covers both the default
// host path (wraps QSerialPort) and the test-override path (wraps QGCAndroidSerialPort
// with an injected MockQSerialPortEngine) used by host integration tests.
class QGCSerialPortAdapterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void cleanup();

    void _defaultBackend_isHost();
    void _forceAndroidBackend_routesThroughMockEngine();
    void _forceAndroidBackend_writeAndRead();
    void _forceAndroidBackend_resourceError_propagates();
};
