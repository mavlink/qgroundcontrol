#pragma once

#include "UnitTest.h"

class SysStatusSensorInfoTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _initialState_test();
    void _updateSingleSensorHealthy_test();
    void _updateSingleSensorUnhealthy_test();
    void _updateSingleSensorDisabled_test();
    void _sensorOrdering_test();
    void _sensorInfoChangedSignal_test();
    void _noSignalOnSameState_test();
    void _sensorRemoval_test();
    void _multipleSensors_test();
    void _updateExistingSensorFlipsHealth_test();
    void _updateExistingSensorDisables_test();
    void _sensorNamesOrderingMirrorsStatusOrder_test();
};
