#pragma once

#include "UnitTest.h"

class SerialPortManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _finishedReceiverReleasesReservation_data();
    void _finishedReceiverReleasesReservation();
    void _routingExclusionsDoNotOccupyPorts();
    void _exclusiveReservations();
    void _singlePortInventory();
    void _inventoryNotificationsAndBaudRates();
};
