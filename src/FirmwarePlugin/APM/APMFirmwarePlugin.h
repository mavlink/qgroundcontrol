/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FirmwarePlugin.h"
#include "FollowMe.h"
#include "QGCMAVLink.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtNetwork/QAbstractSocket>

Q_DECLARE_LOGGING_CATEGORY(APMFirmwarePluginLog)

struct APMCustomMode
{
    enum Mode : uint32_t {
        AUTO        = 3,
        GUIDED      = 4,
        RTL         = 6,
        SMART_RTL   = 21
    };
};

/// This is the base class for all stack specific APM firmware plugins
class APMFirmwarePlugin : public FirmwarePlugin
{
    Q_OBJECT

public:
    QList<MAV_CMD> supportedMissionCommands(QGCMAVLink::VehicleClass_t vehicleClass) const override;
    AutoPilotPlugin *autopilotPlugin(Vehicle* vehicle) const override;
    bool isCapable(const Vehicle *vehicle, FirmwareCapabilities capabilities) const override;
    void setGuidedMode(Vehicle *vehicle, bool guidedMode) const override;
    void guidedModeTakeoff(Vehicle *vehicle, double altitudeRel) const override;
    void guidedModeGotoLocation(Vehicle *vehicle, const QGeoCoordinate& gotoCoord) const override;
    double minimumTakeoffAltitudeMeters(Vehicle *vehicle) const override;
    void startMission(Vehicle *vehicle) const override;
    QStringList flightModes(Vehicle *vehicle) const override;
    QString flightMode(uint8_t base_mode, uint32_t custom_mode) const override;
    bool setFlightMode(const QString &flightMode, uint8_t *base_mode, uint32_t *custom_mode) const override;
    bool MAV_CMD_DO_SET_MODE_is_supported() const override { return true; }
    bool isGuidedMode(const Vehicle *vehicle) const override;
    QString gotoFlightMode() const override { return guidedFlightMode(); }
    QString rtlFlightMode() const override;
    QString smartRTLFlightMode() const override;
    QString missionFlightMode() const override;
    virtual QString guidedFlightMode() const;
    void pauseVehicle(Vehicle *vehicle) const override;
    void guidedModeRTL(Vehicle *vehicle, bool smartRTL) const override;
    void guidedModeChangeAltitude(Vehicle *vehicle, double altitudeChange, bool pauseVehicle) override;
    void guidedModeChangeHeading(Vehicle *vehicle, const QGeoCoordinate &headingCoord) const override;
    bool adjustIncomingMavlinkMessage(Vehicle *vehicle, mavlink_message_t *message) override;
    void adjustOutgoingMavlinkMessageThreadSafe(Vehicle *vehicle, LinkInterface *outgoingLink, mavlink_message_t *message) override;
    virtual void initializeStreamRates(Vehicle *vehicle);
    void initializeVehicle(Vehicle *vehicle) override;
    bool sendHomePositionToVehicle() const override { return true; }
    QString missionCommandOverrides(QGCMAVLink::VehicleClass_t vehicleClass) const override;
    QString _internalParameterMetaDataFile(const Vehicle* vehicle) const override;
    FactMetaData *_getMetaDataForFact(QObject *parameterMetaData, const QString &name, FactMetaData::ValueType_t type, MAV_TYPE vehicleType) const override;
    void _getParameterMetaDataVersionInfo(const QString &metaDataFile, int &majorVersion, int &minorVersion) const override;
    QObject *_loadParameterMetaData(const QString &metaDataFile) override;
    QString brandImageIndoor(const Vehicle *vehicle) const override { Q_UNUSED(vehicle); return QStringLiteral("/qmlimages/APM/BrandImage"); }
    QString brandImageOutdoor(const Vehicle *vehicle) const override { Q_UNUSED(vehicle); return QStringLiteral("/qmlimages/APM/BrandImage"); }
    QString getHobbsMeter(Vehicle *vehicle) const override;
    bool hasGripper(const Vehicle *vehicle) const override;
    const QVariantList &toolIndicators(const Vehicle *vehicle) override;
    double maximumEquivalentAirspeed(Vehicle *vehicle) const override;
    double minimumEquivalentAirspeed(Vehicle *vehicle) const override;
    bool fixedWingAirSpeedLimitsAvailable(Vehicle *vehicle) const override;
    void guidedModeChangeEquivalentAirspeedMetersSecond(Vehicle* vehicle, double airspeed_equiv) const override;
    QVariant mainStatusIndicatorContentItem(const Vehicle* vehicle) const override;
    void sendGCSMotionReport(Vehicle *vehicle, const FollowMe::GCSMotionReport& motionReport, uint8_t estimatationCapabilities) const override;

    // support for changing speed in Copter guide mode:
    bool mulirotorSpeedLimitsAvailable(Vehicle *vehicle) const override;
    double maximumHorizontalSpeedMultirotor(Vehicle *vehicle) const override;
    void guidedModeChangeGroundSpeedMetersSecond(Vehicle *vehicle, double speed) const override;

    static QPair<QMetaObject::Connection,QMetaObject::Connection> startCompensatingBaro(Vehicle *vehicle);
    static bool stopCompensatingBaro(const Vehicle *vehicle, QPair<QMetaObject::Connection,QMetaObject::Connection> updaters);
    static qreal calcAltOffsetPT(uint32_t atmospheric1, qreal temperature1, uint32_t atmospheric2, qreal temperature2);
    static qreal calcAltOffsetP(uint32_t atmospheric1, uint32_t atmospheric2);

protected:
    /// All access to singleton is through stack specific implementation
    explicit APMFirmwarePlugin(QObject *parent = nullptr);
    virtual ~APMFirmwarePlugin();

    void setSupportedModes(QList<APMCustomMode> supportedModes) { _supportedModes = supportedModes; }

    bool _coaxialMotors = false;

    const QString _guidedFlightMode = tr("Guided");
    const QString _rtlFlightMode = tr("RTL");
    const QString _smartRtlFlightMode = tr("Smart RTL");
    const QString _autoFlightMode = tr("Auto");

private slots:
    void _artooSocketError(QAbstractSocket::SocketError socketError);

private:
    void _adjustCalibrationMessageSeverity(mavlink_message_t *message) const;
    void _setInfoSeverity(mavlink_message_t *message) const;
    void _handleIncomingParamValue(Vehicle *vehicle, mavlink_message_t *message);
    bool _handleIncomingStatusText(Vehicle *vehicle, mavlink_message_t *message);
    void _handleIncomingHeartbeat(Vehicle *vehicle, mavlink_message_t *message);
    void _handleOutgoingParamSetThreadSafe(Vehicle *vehicle, LinkInterface *outgoingLink, mavlink_message_t *message);
    void _soloVideoHandshake();
    bool _guidedModeTakeoff(Vehicle *vehicle, double altitudeRel) const;
    void _handleRCChannels(Vehicle *vehicle, mavlink_message_t* message);
    void _handleRCChannelsRaw(Vehicle *vehicle, mavlink_message_t* message);
    QString _getLatestVersionFileUrl(Vehicle *vehicle) const final;
    QString _versionRegex() const final { return QStringLiteral(" V([0-9,\\.]*)$"); }
    QString _vehicleClassToString(QGCMAVLink::VehicleClass_t vehicleClass) const;

    static void _setBaroGndTemp(Vehicle *vehicle, qreal temperature);
    static void _setBaroAltOffset(Vehicle *vehicle, qreal offset);

    // Any instance data here must be global to all vehicles
    // Vehicle specific data should go into APMFirmwarePluginInstanceData

    QVariantList _toolIndicatorList;
    QList<APMCustomMode> _supportedModes;
    QMap<int /* vehicle id */, QMap<int /* componentId */, bool /* true: component is part of ArduPilot stack */>> _ardupilotComponentMap;

    QMutex _adjustOutgoingMavlinkMutex;

    static constexpr const char *_artooIP = "10.1.1.1"; ///< IP address of ARTOO controller
    static constexpr int _artooVideoHandshakePort = 5502;       ///< Port for video handshake on ARTOO controller

    static uint8_t _reencodeMavlinkChannel();
    static QMutex &_reencodeMavlinkChannelMutex();
};

/*===========================================================================*/

class APMFirmwarePluginInstanceData : public FirmwarePluginInstanceData
{
    Q_OBJECT

    using FirmwarePluginInstanceData::FirmwarePluginInstanceData;

public:

    // anyVersionSupportsCommand returns
    // CommandSupportedResult::SUPPORTED if any version of the
    // firmware has supported cmd.  It return UNSUPPORTED if no
    // version ever has.  It returns UNKNOWN if that information is
    // not known.
    CommandSupportedResult anyVersionSupportsCommand(MAV_CMD cmd) const final
    {
        switch (cmd) {
        case MAV_CMD_DO_SET_MISSION_CURRENT:
            return CommandSupportedResult::SUPPORTED;
        default:
            return CommandSupportedResult::UNKNOWN;
        }
    }

    QTime lastBatteryStatusTime;
    QTime lastHomePositionTime;

    bool MAV_CMD_DO_REPOSITION_supported = false;
    bool MAV_CMD_DO_REPOSITION_unsupported = false;
};
