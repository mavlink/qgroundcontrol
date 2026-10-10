#pragma once

#include <QtPositioning/QGeoCoordinate>

#include "BaseClasses/VehicleTest.h"

/// Verifies that MAV_CMD_DO_SET_HOME is sent as COMMAND_INT in MAV_FRAME_GLOBAL when the
/// vehicle advertises MAV_PROTOCOL_CAPABILITY_COMMAND_INT (MockLink does), for both
/// "Set Home Here" and the GCS-position home update.
class SetHomeTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit SetHomeTest(QObject* parent = nullptr)
        : VehicleTest(parent)
    {}

private slots:
    void _setHomeHereSendsCommandInt();
    void _gcsPositionUpdateSendsCommandInt();

private:
    /// Waits for a DO_SET_HOME COMMAND_INT and checks its frame and location
    void _verifySetHomeCommandInt(const QGeoCoordinate& coord, double amslAltitude);
};
