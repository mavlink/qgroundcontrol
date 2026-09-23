#pragma once

#include "BaseClasses/VehicleTest.h"

class StandardModesTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit StandardModesTest(QObject* parent = nullptr) : VehicleTest(parent) {}

private slots:
    void _monitorSequenceBumpTriggersRequery();
    void _singleModeDoesNotDependOnPeriodicTelemetry();
    void _duplicateDeliveryDoesNotDuplicateModes();

private:
    bool _singleModeReceived = false;
    bool _singleModeValid = false;
};
