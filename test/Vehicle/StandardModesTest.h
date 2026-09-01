#pragma once

#include "BaseClasses/VehicleTest.h"

class StandardModesTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit StandardModesTest(QObject* parent = nullptr) : VehicleTest(parent) {}

private slots:
    void _monitorSequenceBumpTriggersRequery();
};
