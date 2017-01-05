/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef APMFirmwarePlugin_H
#define APMFirmwarePlugin_H

#include "FirmwarePlugin.h"
#include "QGCLoggingCategory.h"
#include "APMParameterMetaData.h"
#include "APMGeoFenceManager.h"
#include "APMRallyPointManager.h"

#include <QAbstractSocket>

Q_DECLARE_LOGGING_CATEGORY(APMFirmwarePluginLog)

class APMFirmwareVersion
{
public:
    APMFirmwareVersion(const QString &versionText = "");
    bool isValid() const;
    bool isBeta() const;
    bool isDev() const;
    bool operator<(const APMFirmwareVersion& other) const;
    QString versionString() const { return _versionString; }
    QString vehicleType() const { return _vehicleType; }
    int majorNumber() const { return _major; }
    int minorNumber() const { return _minor; }
    int patchNumber() const { return _patch; }

private:
    void _parseVersion(const QString &versionText);
    QString _versionString;
    QString _vehicleType;
    int     _major;
    int     _minor;
    int     _patch;
};

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

    QList<VehicleComponent*> componentsForVehicle(AutoPilotPlugin* vehicle) final;
    QList<MAV_CMD> supportedMissionCommands(void) final;

    AutoPilotPlugin*    autopilotPlugin                 (Vehicle* vehicle) final;
    bool                isCapable                       (const Vehicle *vehicle, FirmwareCapabilities capabilities);
    QStringList         flightModes                     (Vehicle* vehicle) final;
    QString             flightMode                      (uint8_t base_mode, uint32_t custom_mode) const final;
    bool                setFlightMode                   (const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode) final;
    bool                isGuidedMode                    (const Vehicle* vehicle) const final;
    void                pauseVehicle                    (Vehicle* vehicle);
    int                 manualControlReservedButtonCount(void);
    bool                adjustIncomingMavlinkMessage    (Vehicle* vehicle, mavlink_message_t* message) final;
    void                adjustOutgoingMavlinkMessage    (Vehicle* vehicle, LinkInterface* outgoingLink, mavlink_message_t* message) final;
    void                initializeVehicle               (Vehicle* vehicle) final;
    bool                sendHomePositionToVehicle       (void) final;
    void                addMetaDataToFact               (QObject* parameterMetaData, Fact* fact, MAV_TYPE vehicleType) final;
    QString             getDefaultComponentIdParam      (void) const final { return QString("SYSID_SW_TYPE"); }
    QString             missionCommandOverrides         (MAV_TYPE vehicleType) const;
    QString             getVersionParam                 (void) final { return QStringLiteral("SYSID_SW_MREV"); }
    QString             internalParameterMetaDataFile   (Vehicle* vehicle) final;
    void                getParameterMetaDataVersionInfo (const QString& metaDataFile, int& majorVersion, int& minorVersion) final { APMParameterMetaData::getParameterMetaDataVersionInfo(metaDataFile, majorVersion, minorVersion); }
    QObject*            loadParameterMetaData           (const QString& metaDataFile);
    GeoFenceManager*    newGeoFenceManager              (Vehicle* vehicle) { return new APMGeoFenceManager(vehicle); }
    RallyPointManager*  newRallyPointManager            (Vehicle* vehicle) { return new APMRallyPointManager(vehicle); }
    QString             brandImage                      (const Vehicle* vehicle) const { Q_UNUSED(vehicle); return QStringLiteral("/qmlimages/APM/BrandImage"); }
    QString             missionFlightMode               (void) final;
    QString             rtlFlightMode                   (void) final;

protected:
    /// All access to singleton is through stack specific implementation
    APMFirmwarePlugin(void);
    void setSupportedModes(QList<APMCustomMode> supportedModes);

    bool _coaxialMotors;

private slots:
    void _artooSocketError(QAbstractSocket::SocketError socketError);

private:
    void _adjustSeverity(mavlink_message_t* message) const;
    void _adjustCalibrationMessageSeverity(mavlink_message_t* message) const;
    static bool _isTextSeverityAdjustmentNeeded(const APMFirmwareVersion& firmwareVersion);
    void _setInfoSeverity(mavlink_message_t* message) const;
    QString _getMessageText(mavlink_message_t* message) const;
    void _handleIncomingParamValue(Vehicle* vehicle, mavlink_message_t* message);
    bool _handleIncomingStatusText(Vehicle* vehicle, mavlink_message_t* message);
    void _handleIncomingHeartbeat(Vehicle* vehicle, mavlink_message_t* message);
    void _handleOutgoingParamSet(Vehicle* vehicle, LinkInterface* outgoingLink, mavlink_message_t* message);
    void _soloVideoHandshake(Vehicle* vehicle);

    bool                    _textSeverityAdjustmentNeeded;
    QList<APMCustomMode>    _supportedModes;
    QMap<QString, QTime>    _noisyPrearmMap;

    static const char*  _artooIP;
    static const int    _artooVideoHandshakePort;

};

#endif
