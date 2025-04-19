/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "LinkConfiguration.h"

#include <QtCore/QLoggingCategory>
#include "MAVLinkLib.h"

Q_DECLARE_LOGGING_CATEGORY(MockConfigurationLog)

class MockConfiguration : public LinkConfiguration
{
    Q_OBJECT
    Q_PROPERTY(int      firmware            READ firmware           WRITE setFirmware           NOTIFY firmwareChanged)
    Q_PROPERTY(int      vehicle             READ vehicle            WRITE setVehicle            NOTIFY vehicleChanged)
    Q_PROPERTY(bool     sendStatus          READ sendStatusText     WRITE setSendStatusText     NOTIFY sendStatusChanged)
    Q_PROPERTY(bool     incrementVehicleId  READ incrementVehicleId WRITE setIncrementVehicleId NOTIFY incrementVehicleIdChanged)

public:
    explicit MockConfiguration(const QString &name, QObject *parent = nullptr);
    explicit MockConfiguration(const MockConfiguration *copy, QObject *parent = nullptr);
    ~MockConfiguration();

    LinkType type() const final { return LinkConfiguration::TypeMock; }
    void copyFrom(const LinkConfiguration *source) final;
    void loadSettings(QSettings &settings, const QString &root) final;
    void saveSettings(QSettings &settings, const QString &root) const final;
    QString settingsURL() const final { return QStringLiteral("MockLinkSettings.qml"); }
    QString settingsTitle() const final { return tr("Mock Link Settings"); }

    int firmware() const { return static_cast<int>(_firmwareType); }
    void setFirmware(int type) { _firmwareType = static_cast<MAV_AUTOPILOT>(type); emit firmwareChanged(); }
    int vehicle() const { return static_cast<int>(_vehicleType); }
    void setVehicle(int type) { _vehicleType = static_cast<MAV_TYPE>(type); emit vehicleChanged(); }
    bool incrementVehicleId() const { return _incrementVehicleId; }
    void setIncrementVehicleId(bool incrementVehicleId) { _incrementVehicleId = incrementVehicleId; emit incrementVehicleIdChanged(); }
    MAV_AUTOPILOT firmwareType() const { return _firmwareType; }
    void setFirmwareType(MAV_AUTOPILOT firmwareType) { _firmwareType = firmwareType; emit firmwareChanged(); }
    uint16_t boardVendorId() const { return _boardVendorId; }
    uint16_t boardProductId() const { return _boardProductId; }
    void setBoardVendorProduct(uint16_t vendorId, uint16_t productId) { _boardVendorId = vendorId; _boardProductId = productId; }
    MAV_TYPE vehicleType() const { return _vehicleType; }
    void setVehicleType(MAV_TYPE vehicleType) { _vehicleType = vehicleType; emit vehicleChanged(); }
    bool sendStatusText() const { return _sendStatusText; }
    void setSendStatusText(bool sendStatusText) { _sendStatusText = sendStatusText; emit sendStatusChanged(); }

    enum FailureMode_t {
        FailNone,                                                   // No failures
        FailParamNoReponseToRequestList,                            // Do no respond to PARAM_REQUEST_LIST
        FailMissingParamOnInitialReqest,                            // Not all params are sent on initial request, should still succeed since QGC will re-query missing params
        FailMissingParamOnAllRequests,                              // Not all params are sent on initial request, QGC retries will fail as well
        FailInitialConnectRequestMessageAutopilotVersionFailure,    // REQUEST_MESSAGE:AUTOPILOT_VERSION returns failure
        FailInitialConnectRequestMessageAutopilotVersionLost,       // REQUEST_MESSAGE:AUTOPILOT_VERSION success, AUTOPILOT_VERSION never sent
        FailInitialConnectRequestMessageProtocolVersionFailure,     // REQUEST_MESSAGE:PROTOCOL_VERSION returns failure
        FailInitialConnectRequestMessageProtocolVersionLost,        // REQUEST_MESSAGE:PROTOCOL_VERSION success, PROTOCOL_VERSION never sent
    };
    FailureMode_t failureMode() const { return _failureMode; }
    void setFailureMode(FailureMode_t failureMode) { _failureMode = failureMode; }

signals:
    void firmwareChanged();
    void vehicleChanged();
    void sendStatusChanged();
    void incrementVehicleIdChanged();

private:
    MAV_AUTOPILOT _firmwareType = MAV_AUTOPILOT_PX4;
    MAV_TYPE _vehicleType = MAV_TYPE_QUADROTOR;
    bool _sendStatusText = false;
    FailureMode_t _failureMode = FailNone;
    bool _incrementVehicleId = true;
    uint16_t _boardVendorId = 0;
    uint16_t _boardProductId = 0;

    static constexpr const char *_firmwareTypeKey = "FirmwareType";
    static constexpr const char *_vehicleTypeKey = "VehicleType";
    static constexpr const char *_sendStatusTextKey = "SendStatusText";
    static constexpr const char *_incrementVehicleIdKey = "IncrementVehicleId";
    static constexpr const char *_failureModeKey = "FailureMode";
};
