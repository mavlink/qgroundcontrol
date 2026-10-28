#pragma once

#include "BaseClasses/VehicleTest.h"

class VehicleComponent;

/// What the PX4 components show on the Summary page, read from a PX4 MockLink's parameters
class PX4SummaryItemsTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit PX4SummaryItemsTest(QObject* parent = nullptr)
        : VehicleTest(parent)
    {
        setWaitForParameters(true);
    }

private slots:
    void _summaryItems_data();
    void _summaryItems();
    void _summaryItemsChangedWithParameter();
    void _summaryItemsChangedWhenBatteryFactAdded();
    void _requiresSetupChangedWithParameter();

private:
    VehicleComponent* _component(const QString& name) const;
};
