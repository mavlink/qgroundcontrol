#pragma once

#include "BaseClasses/VehicleTest.h"

class ComponentInformationManagerTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void _requestAllComponentInformationCompletes();
    void _progressSignalsStayInRange();
    void _requestCanRunMultipleTimes();
    void _compInfoAccessorsReturnValidObjects();
    void _requestCompletesForArduPilot();
};
