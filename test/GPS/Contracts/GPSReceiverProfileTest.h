#pragma once

#include "UnitTest.h"

class GPSReceiverProfileTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _endpointValidation_data();
    void _endpointValidation();
    void _inactiveFieldsDoNotChangeProfile();
};
