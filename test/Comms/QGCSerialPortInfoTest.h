#pragma once

#include "TestFixtures.h"

/// Unit tests for QGCSerialPortInfo.
/// Tests USB board identification and serial port info functionality.
/// Uses OfflineTest since it doesn't require a vehicle connection.
class QGCSerialPortInfoTest : public OfflineTest
{
    Q_OBJECT

public:
    QGCSerialPortInfoTest() = default;

private slots:
    // JSON data loading tests
    void _testLoadJsonData();
    void _testJsonDataStructure();

    // Board type conversion tests
    void _testBoardClassStringToType();
    void _testBoardTypeToString();

    // System port detection tests
    void _testIsSystemPortKnownPorts();

    // Available ports tests
    void _testAvailablePortsReturnsValidList();
};
