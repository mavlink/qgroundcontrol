// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#pragma once

#include "UnitTest.h"

// Exercises QGCAndroidSerialPort's production logic on host via injected MockQSerialPortEngine.
// Validates open/close lifecycle, configuration caching, write paths, read fan-out,
// exception → error mapping, and re-entrancy guards — all without JNI.
class QGCAndroidSerialPortTest : public UnitTest
{
    Q_OBJECT

private slots:
    void cleanup();

    void _open_pushesConfigToEngine();
    void _open_factoryReturningNull_fails();
    void _close_emitsAboutToCloseBeforeTeardown();
    void _close_emitsReadChannelFinished();
    void _close_clearsBuffersAndState();
    void _writeData_routesThroughEngine();
    void _dataReady_fillsReadBuffer_emitsReadyRead();
    void _exception_resourceUnavailable_setsErrorAndCloses();
    void _exception_permission_setsErrorOnly();
    void _expectClosure_suppressesResourceExceptionDuringClose();
    void _setters_whileClosed_cacheForNextOpen();
    void _setters_whileOpen_applyImmediately();
    void _testBatchedSetSerialParameters();
};
