/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QTimer>
#include <QGeoCoordinate>

#include "MAVLinkLib.h"
#include "QmlObjectListModel.h"
#include "f4_autonomy/f4_autonomy.h"
Q_DECLARE_LOGGING_CATEGORY(OnboardComputersManagerLog)

class Vehicle;

//-----------------------------------------------------------------------------
/// Onboard Computers Manager
class OnboardComputersManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(int currentComputerComponent            READ currentComputerComponent WRITE setCurrentComputerComponent NOTIFY currentComputerComponentChanged)
    Q_PROPERTY(QList<QVariantMap>computersInfo         READ computersInfo                                              NOTIFY computersInfoChanged)
public:
    OnboardComputersManager(Vehicle* vehicle, QObject *parent=nullptr);
    virtual ~OnboardComputersManager() = default;

    static void registerQmlTypes();

    //current computer
    int currentComputerComponent()                   { return _currentComputerComponent; }  ///< Current selected computer component

    // virtual
    virtual void        setCurrentComputerComponent(int sel);

    QList<QVariantMap>  computersInfo();
    QVariantMap         computerInfo(uint8_t compId);


    virtual void rebootAllOnboardComputers();

    // TODO: this function should be moved to some node handling VIO communication
    /*!
     * \brief send an external position estimate command to the current onboard computer running a navigation algorithm.
     */
    Q_INVOKABLE virtual void sendExternalPositionEstimate(const QGeoCoordinate& coord);
    Q_INVOKABLE virtual bool checkVersion(QString desctiption,int major,int minor=0,int patch=0);

    // This is public to avoid some circular include problems caused by statics
    class OnboardComputerStruct {
    public:
        OnboardComputerStruct(uint8_t compId_, Vehicle* vehicle_);
        OnboardComputerStruct() = default;
        QElapsedTimer lastHeartbeat{};
        uint8_t compId{0};
        uint8_t infoRequestCnt{0};

        struct OsVersion{
            uint8_t major{0};
            uint8_t minor{0};
            uint8_t patch{0};
            uint8_t firmwareVersion{0};
            bool compatible(OsVersion& other) const{
                return major == other.major &&
                    (minor > other.minor ||
                    (minor == other.minor && patch >= other.patch));
            }
            bool isValid() const {
                return major != 0 || minor != 0 || patch != 0;
            }
            void fromUint32(uint32_t value) {
                firmwareVersion = (value >> 0 ) & 0xFF;
                patch           = (value >> 8 ) & 0xFF;
                minor           = (value >> 16) & 0xFF;
                major           = (value >> 24) & 0xFF;
            }

            uint32_t toUint32() const {
                return (uint32_t(firmwareVersion) << 0)  |
                       (uint32_t(patch)           << 8)  |
                       (uint32_t(minor)           << 16) |
                       (uint32_t(major)           << 24);
            }
        }osVersion;

        mavlink_companion_version_t info{0};
        Vehicle* vehicle{nullptr};
    };

signals:
    void onboardComputersChanged();
    void currentComputerComponentChanged(uint8_t compID);
    void streamChanged();
    void onboardComputerTimeout(uint8_t compID);
    void onboardComputerInfoUpdated(uint8_t compID);
    void onboardComputerInfoRecieveError( uint8_t compID);
    void computersListChanged();
    void computersInfoChanged();

protected slots:
    virtual void _vehicleMessageReceived(int sysid, int componentid, int severity, QString text, QString description);
    virtual void _vehicleReady(bool ready);
    virtual void _mavlinkMessageReceived(const mavlink_message_t& message);
    void _checkTimeouts();

protected:
    virtual void _handleHeartbeat(const mavlink_message_t& message);
    virtual void _handleCompanionVersion(const mavlink_message_t& message);
    // TODO: we could extend this with handling of ONBOARD_COMPUTER_STATUS mavlink message, but it is still WIP
    Vehicle* _vehicle = nullptr;
    bool _vehicleReadyState = false;
    int _currentComputerComponent = 0;
    const int _companionVersionMaxRetryCount=2;
    const int _timeoutCheckInterval=2000;
    QTimer    _timeoutCheckTimer;
    QMap<uint8_t, OnboardComputerStruct> _onboardComputers;
};
