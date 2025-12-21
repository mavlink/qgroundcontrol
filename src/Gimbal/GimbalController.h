/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "Gimbal.h"
#include "MAVLinkLib.h"

Q_DECLARE_LOGGING_CATEGORY(GimbalControllerLog)

class QmlObjectListModel;
class Vehicle;

class GimbalController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("QmlObjectListModel.h")

    Q_PROPERTY(Gimbal*              activeGimbal    READ activeGimbal WRITE setActiveGimbal NOTIFY activeGimbalChanged)
    Q_PROPERTY(QmlObjectListModel*  gimbals         READ gimbals                            CONSTANT)

public:
    GimbalController(Vehicle *vehicle);
    ~GimbalController();

    Gimbal *activeGimbal() const { return _activeGimbal; }
    QmlObjectListModel *gimbals() const { return _gimbals; }

    void setActiveGimbal(Gimbal *gimbal);

    void sendPitchYawFlags(uint32_t flags);
    Q_INVOKABLE void gimbalOnScreenControl(float panpct, float tiltpct, bool clickAndPoint, bool clickAndDrag, bool rateControl, bool retract = false, bool neutral = false, bool yawlock = false);
    Q_INVOKABLE void sendPitchBodyYaw(float pitch, float yaw, bool showError = true);
    Q_INVOKABLE void sendPitchAbsoluteYaw(float pitch, float yaw, bool showError = true);
    Q_INVOKABLE void setGimbalRetract(bool set);
    Q_INVOKABLE void setGimbalYawLock(bool set);
    Q_INVOKABLE void acquireGimbalControl();
    Q_INVOKABLE void releaseGimbalControl();
    Q_INVOKABLE void sendRate();

    /// Send gimbal attitude rates directly without using active gimbal's rate properties
    /// @param pitch_rate_deg_s Pitch rate in degrees per second
    /// @param yaw_rate_deg_s Yaw rate in degrees per second
    Q_INVOKABLE void sendGimbalRate(float pitch_rate_deg_s, float yaw_rate_deg_s);

signals:
    void activeGimbalChanged();
    void showAcquireGimbalControlPopup(); // This triggers a popup in QML asking the user for aproval to take control

public slots:
    // These slots are conected with joysticks for button control
    void gimbalYawLock(bool yawLock) { setGimbalYawLock(yawLock); }
    Q_INVOKABLE void centerGimbal();
    void gimbalPitchStart(int direction);
    void gimbalYawStart(int direction);
    void gimbalPitchStop();
    void gimbalYawStop();

private slots:
    void _mavlinkMessageReceived(const mavlink_message_t& message);
    void _rateSenderTimeout();

private:
    struct GimbalPairId {
        GimbalPairId() = default;
        GimbalPairId(uint8_t _managerCompid, uint8_t _deviceId)
            : managerCompid(_managerCompid)
            , deviceId(_deviceId) {}

        // In order to use this as a key, we need to implement <,
        bool operator<(const GimbalPairId &other) const {
            // We compare managerCompid primarily, if they are equal, we compare the deviceId
            if (managerCompid < other.managerCompid) {
                return true;
            } else if (managerCompid > other.managerCompid) {
                return false;
            } else if (deviceId < other.deviceId) {
                return true;
            } else {
                return false;
            }
        }

        bool operator==(const GimbalPairId &other) const {
            return (managerCompid == other.managerCompid) && (deviceId == other.deviceId);
        }

        uint8_t managerCompid = 0;
        uint8_t deviceId = 0;
    };

    void _requestGimbalInformation(uint8_t compid);
    void _handleHeartbeat(const mavlink_message_t &message);
    void _handleGimbalManagerInformation(const mavlink_message_t &message);
    void _handleGimbalManagerStatus(const mavlink_message_t &message);
    void _handleGimbalDeviceAttitudeStatus(const mavlink_message_t &message);
    void _checkComplete(Gimbal &gimbal, GimbalPairId pairId);
    bool _tryGetGimbalControl();
    bool _yawInVehicleFrame(uint32_t flags);

    void _sendGimbalAttitudeRates(float pitch_rate_deg_s, float yaw_rate_deg_s);

    QTimer _rateSenderTimer;
    Vehicle *_vehicle = nullptr;
    Gimbal *_activeGimbal = nullptr;

    struct PotentialGimbalManager {
        unsigned requestGimbalManagerInformationRetries = 6;
        bool receivedGimbalManagerInformation = false;
    };
    QMap<uint8_t, PotentialGimbalManager> _potentialGimbalManagers; // key is compid

    QMap<GimbalPairId, Gimbal*> _potentialGimbals;
    QmlObjectListModel *_gimbals = nullptr;

    static constexpr const char *_gimbalFactGroupNamePrefix = "gimbal";
};
