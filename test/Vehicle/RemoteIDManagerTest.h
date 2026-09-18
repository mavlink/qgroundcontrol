#pragma once

#include <QtCore/QVariant>

#include "BaseClasses/VehicleTest.h"

class RemoteIDManagerTest : public VehicleTestNoInitialConnect
{
    Q_OBJECT

public:
    explicit RemoteIDManagerTest(QObject* parent = nullptr)
        : VehicleTestNoInitialConnect(parent)
    {
    }

private slots:
    void init() override;
    void cleanup() override;

    void _operatorIDBroadcastGating();
    void _operatorIDBroadcastNullPadded();
    void _basicIDMissingFlagFollowsArmStatusError();
    void _liveGpsFailureDiagnostics_data();
    void _liveGpsFailureDiagnostics();
    void _gpsAltitudePolicy_data();
    void _gpsAltitudePolicy();
    void _liveGpsArrivalBudget_data();
    void _liveGpsArrivalBudget();

private:
    QVariant _savedOperatorIDEU;
    QVariant _savedOperatorIDFAA;
    QVariant _savedOperatorIDType;
    QVariant _savedRegion;
    QVariant _savedSendOperatorID;
    QVariant _savedLocationType;
};
