#pragma once

#include "UnitTest.h"

class VehicleUomRegistrationManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _unavailableAdapterDisablesOperations_data();
    void _unavailableAdapterDisablesOperations();
};
