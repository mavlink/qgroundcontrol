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

#include "MAVLinkLib.h"
#include "QmlObjectListModel.h"
#include "f4_autonomy/f4_autonomy.h"
Q_DECLARE_LOGGING_CATEGORY(OnboardComputersManagerLog)

class Vehicle;

//-----------------------------------------------------------------------------
/// Onboard Computers Manager
class OnboardComputersManager : public QObject {
    Q_OBJECT

   public:
    OnboardComputersManager(Vehicle* vehicle);
    virtual ~OnboardComputersManager() = default;

    static void registerQmlTypes();

    Q_PROPERTY(int currentComputerComponent            READ currentComputerComponent WRITE setCurrentComputerComponent NOTIFY currentComputerComponentChanged)
    Q_PROPERTY(QList<QVariantMap>computersInfo     READ computersInfo                                      NOTIFY computersInfoChanged)
    //current computer
    int currentComputerComponent()                   { return _currentComputerComponent; }  ///< Current selected computer index

    // virtual
    virtual void        setCurrentComputerComponent(int sel);

    QList<QVariantMap>  computersInfo();
    QVariantMap         computerInfo(uint8_t compId);

    // This is public to avoid some circular include problems caused by statics
    class OnboardComputerStruct {
       public:
        OnboardComputerStruct(uint8_t compId_, Vehicle* vehicle_);
        OnboardComputerStruct() = default;
        QElapsedTimer lastHeartbeat{};
        uint8_t compId{0};
        uint8_t infoRequestCnt{0};
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
    virtual void _vehicleReady(bool ready);
    virtual void _mavlinkMessageReceived(const mavlink_message_t& message);
    void _checkTimeouts();

   public:
    virtual void rebootAllOnboardComputers();

    // TODO: this function should be moved to some node handling VIO communication
    /*!
     * \brief send an external position estimate command to the current onboard computer running a navigation algorithm.
     */
    Q_INVOKABLE virtual void sendExternalPositionEstimate(const QGeoCoordinate& coord);

   protected:
    virtual void _handleHeartbeat(const mavlink_message_t& message);
    virtual void _handleCompanionVersion(const mavlink_message_t& message);
    // TODO: we could extend this with handling of ONBOARD_COMPUTER_STATUS mavlink message, but it is still WIP
    Vehicle* _vehicle = nullptr;
    bool _vehicleReadyState = false;
    int _currentComputerComponent = 0;
    const int _companionVersionMaxRetryCount=4;
    const int _timeoutCheckInterval=2000;
    QTimer    _timeoutCheckTimer;
    QMap<uint8_t, OnboardComputerStruct> _onboardComputers;
};
