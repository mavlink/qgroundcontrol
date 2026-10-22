#pragma once

#include "BaseClasses/VehicleTest.h"

class FactPanelControllerTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit FactPanelControllerTest(QObject* parent = nullptr)
        : VehicleTest(parent)
    {
        setWaitForParameters(true);
    }

private slots:
    void _rebootNoticeSuppressedWhileRequested_data();
    void _rebootNoticeSuppressedWhileRequested();
    void _rebootNoticeSuppressionEndsWithVehicle();
};
