/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FirmwarePlugin.h"
#include "QGCLoggingCategory.h"
#include "APMParameterMetaData.h"
#include "FollowMe.h"

#include <QAbstractSocket>

Q_DECLARE_LOGGING_CATEGORY(APMFirmwarePluginLog)

class APMCustomMode
{
public:
    APMCustomMode(uint32_t mode, bool settable);
    uint32_t modeAsInt() const { return _mode; }
    bool canBeSet() const { return _settable; }
    QString modeString() const;

protected:
    void setEnumToStringMapping(const QMap<uint32_t,QString>& enumToString);

private:
    uint32_t               _mode;
    bool                   _settable;
    QMap<uint32_t,QString> _enumToString;
};

/// This is the base class for all stack specific APM firmware plugins
class APMFirmwarePlugin : public FirmwarePlugin
{
    Q_OBJECT

public:
    // Overrides from FirmwarePlugin

    QList<VehicleComponent*> componentsForVehicle(AutoPilotPlugin* vehicle) override;
    QList<MAV_CMD> supportedMissionCommands(QGCMAVLink::VehicleClass_t vehicleClass) override;

    AutoPilotPlugin*    autopilotPlugin                 (Vehicle* vehicle) override;
    bool                isCapable                       (const Vehicle *vehicle, FirmwareCapabilities capabilities) override;
    void                setGuidedMode                   (Vehicle* vehicle, bool guidedMode) override;
    void                guidedModeTakeoff               (Vehicle* vehicle, double altitudeRel) override;
    void                guidedModeGotoLocation          (Vehicle* vehicle, const QGeoCoordinate& gotoCoord) override;
    double              minimumTakeoffAltitude          (Vehicle* vehicle) override;
    void                startMission                    (Vehicle* vehicle) override;
    QStringList         flightModes                     (Vehicle* vehicle) override;
    QString             flightMode                      (uint8_t base_mode, uint32_t custom_mode) const override;
    bool                setFlightMode                   (const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode) override;
    bool                isGuidedMode                    (const Vehicle* vehicle) const override;
    QString             gotoFlightMode                  (void) const override { return QStringLiteral("Guided"); }
    QString             rtlFlightMode                   (void) const override { return QString("RTL"); }
    QString             smartRTLFlightMode              (void) const override { return QString("Smart RTL"); }
    QString             missionFlightMode               (void) const override { return QString("Auto"); }
    void                pauseVehicle                    (Vehicle* vehicle) override;
    void                guidedModeRTL                   (Vehicle* vehicle, bool smartRTL) override;
    void                guidedModeChangeAltitude        (Vehicle* vehicle, double altitudeChange, bool pauseVehicle) override;
    bool                adjustIncomingMavlinkMessage    (Vehicle* vehicle, mavlink_message_t* message) override;
    void                adjustOutgoingMavlinkMessageThreadSafe(Vehicle* vehicle, LinkInterface* outgoingLink, mavlink_message_t* message) override;
    virtual void        initializeStreamRates           (Vehicle* vehicle);
    void                initializeVehicle               (Vehicle* vehicle) override;
    bool                sendHomePositionToVehicle       (void) override;
    QString             missionCommandOverrides         (QGCMAVLink::VehicleClass_t vehicleClass) const override;
    QString             _internalParameterMetaDataFile  (Vehicle* vehicle) override;
    FactMetaData*       _getMetaDataForFact             (QObject* parameterMetaData, const QString& name, FactMetaData::ValueType_t type, MAV_TYPE vehicleType) override;
    void                _getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion) override { APMParameterMetaData::getParameterMetaDataVersionInfo(metaDataFile, majorVersion, minorVersion); }
    QObject*            _loadParameterMetaData          (const QString& metaDataFile) override;
    QString             brandImageIndoor                (const Vehicle* vehicle) const override { Q_UNUSED(vehicle); return QStringLiteral("/qmlimages/APM/BrandImage"); }
    QString             brandImageOutdoor               (const Vehicle* vehicle) const override { Q_UNUSED(vehicle); return QStringLiteral("/qmlimages/APM/BrandImage"); }
    QString             getHobbsMeter                   (Vehicle* vehicle) override; 

protected:
    /// All access to singleton is through stack specific implementation
    APMFirmwarePlugin(void);

    void setSupportedModes  (QList<APMCustomMode> supportedModes);
    void _sendGCSMotionReport(Vehicle* vehicle, FollowMe::GCSMotionReport& motionReport, uint8_t estimatationCapabilities);

    bool                _coaxialMotors;

private slots:
    void _artooSocketError(QAbstractSocket::SocketError socketError);

private:
    void _adjustCalibrationMessageSeverity(mavlink_message_t* message) const;
    void _setInfoSeverity(mavlink_message_t* message) const;
    QString _getMessageText(mavlink_message_t* message) const;
    void _handleIncomingParamValue(Vehicle* vehicle, mavlink_message_t* message);
    bool _handleIncomingStatusText(Vehicle* vehicle, mavlink_message_t* message);
    void _handleIncomingHeartbeat(Vehicle* vehicle, mavlink_message_t* message);
    void _handleOutgoingParamSetThreadSafe(Vehicle* vehicle, LinkInterface* outgoingLink, mavlink_message_t* message);
    void _soloVideoHandshake(void);
    bool _guidedModeTakeoff(Vehicle* vehicle, double altitudeRel);
    void _handleRCChannels(Vehicle* vehicle, mavlink_message_t* message);
    void _handleRCChannelsRaw(Vehicle* vehicle, mavlink_message_t* message);
    QString _getLatestVersionFileUrl(Vehicle* vehicle) override;
    QString _versionRegex() override;

    // Any instance data here must be global to all vehicles
    // Vehicle specific data should go into APMFirmwarePluginInstanceData

    QList<APMCustomMode>    _supportedModes;
    QMap<int /* vehicle id */, QMap<int /* componentId */, bool /* true: component is part of ArduPilot stack */>> _ardupilotComponentMap;

    QMutex _adjustOutgoingMavlinkMutex;

    static const char*      _artooIP;
    static const int        _artooVideoHandshakePort;

    static uint8_t          _reencodeMavlinkChannel();
    static QMutex&          _reencodeMavlinkChannelMutex();
};

class APMFirmwarePluginInstanceData : public QObject
{
    Q_OBJECT

public:
    APMFirmwarePluginInstanceData(QObject* parent = nullptr)
        : QObject(parent)
    {

    }

    QTime lastBatteryStatusTime;
    QTime lastHomePositionTime;
};
