#pragma once

#include "BaseClasses/VehicleTest.h"

/// Verifies how MAV_CMD_DO_SET_HOME is sent for both "Set Home Here" and the GCS-position home
/// update: as COMMAND_INT in MAV_FRAME_GLOBAL when the vehicle advertises
/// MAV_PROTOCOL_CAPABILITY_COMMAND_INT (MockLink does), as COMMAND_LONG when it does not, without
/// a yaw, and with which altitude, for a vehicle with and without a home position.
class SetHomeTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit SetHomeTest(QObject* parent = nullptr)
        : VehicleTest(parent)
    {}

private slots:
    void _setHomeHereSendsCommandInt();
    void _gcsPositionUpdateSendsCommandInt_data();
    void _gcsPositionUpdateSendsCommandInt();
    void _gcsPositionUpdateWithoutHomePosition();
    void _setHomeHereSendsCommandLong();
    void _gcsPositionUpdateSendsCommandLong();

private:
    /// Replaces the connected vehicle with one that reports its capabilities without COMMAND_INT
    [[nodiscard]] bool _reconnectWithoutCommandInt();
};
