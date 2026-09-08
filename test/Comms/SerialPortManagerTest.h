#pragma once

#include "UnitTest.h"

class SerialPortManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _exclusiveReservations();
    void _singlePortInventory();
};
