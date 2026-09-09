#pragma once

#include "UnitTest.h"

class GPSConnectionConfigTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _nmeaValidation_data();
    void _nmeaValidation();
    void _rtkValidation_data();
    void _rtkValidation();
    void _settingsSnapshots();
    void _positionRoleValidation();
};
