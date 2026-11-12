#pragma once

#include "UnitTest.h"

/// A receiver connected over Bluetooth, without a radio: real receiver workers on their threads, the session's pipe
/// and its Bluetooth end, over workers of the application's Bluetooth links that the tests script.
class GPSReceiverBluetoothTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _passiveReceiverOverBluetooth();
    void _openIsRefused_data();
    void _openIsRefused();
    void _savedDeviceReconnects();
    void _disconnectEndsLink_data();
    void _disconnectEndsLink();
    void _baseIsConfiguredOverBluetooth();
    void _effectiveConnection_data();
    void _effectiveConnection();
};
