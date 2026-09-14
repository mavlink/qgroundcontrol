#pragma once

#include "UnitTest.h"

class SerialPortManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _routingExclusionsDoNotOccupyPorts();
    void _exclusiveReservations();
    void _displayMetadataUsesCachedInventory();
    void _singlePortInventory();
    void _inventoryNotificationsAndBaudRates();
};
