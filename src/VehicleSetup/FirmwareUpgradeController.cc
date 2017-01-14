/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief PX4 Firmware Upgrade UI
///     @author Don Gagne <don@thegagnes.com>

#include "FirmwareUpgradeController.h"
#include "Bootloader.h"
#include "QGCFileDialog.h"
#include "QGCApplication.h"
#include "QGCFileDownload.h"

#include <QStandardPaths>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkProxy>

struct FirmwareToUrlElement_t {
    FirmwareUpgradeController::AutoPilotStackType_t    stackType;
    FirmwareUpgradeController::FirmwareType_t          firmwareType;
    FirmwareUpgradeController::FirmwareVehicleType_t   vehicleType;
    QString                                            url;
};

uint qHash(const FirmwareUpgradeController::FirmwareIdentifier& firmwareId)
{
    return ( firmwareId.autopilotStackType |
             (firmwareId.firmwareType << 8) |
             (firmwareId.firmwareVehicleType << 16) );
}

/// @Brief Constructs a new FirmwareUpgradeController Widget. This widget is used within the PX4VehicleConfig set of screens.
FirmwareUpgradeController::FirmwareUpgradeController(void)
    : _downloadManager(NULL)
    , _downloadNetworkReply(NULL)
    , _statusLog(NULL)
    , _selectedFirmwareType(StableFirmware)
    , _image(NULL)
{
    _threadController = new PX4FirmwareUpgradeThreadController(this);
    Q_CHECK_PTR(_threadController);

    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBoard,             this, &FirmwareUpgradeController::_foundBoard);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::noBoardFound,           this, &FirmwareUpgradeController::_noBoardFound);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::boardGone,              this, &FirmwareUpgradeController::_boardGone);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBootloader,        this, &FirmwareUpgradeController::_foundBootloader);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::bootloaderSyncFailed,   this, &FirmwareUpgradeController::_bootloaderSyncFailed);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::error,                  this, &FirmwareUpgradeController::_error);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::updateProgress,         this, &FirmwareUpgradeController::_updateProgress);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::status,                 this, &FirmwareUpgradeController::_status);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::eraseStarted,           this, &FirmwareUpgradeController::_eraseStarted);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::eraseComplete,          this, &FirmwareUpgradeController::_eraseComplete);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::flashComplete,          this, &FirmwareUpgradeController::_flashComplete);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::updateProgress,         this, &FirmwareUpgradeController::_updateProgress);
    
    connect(&_eraseTimer, &QTimer::timeout, this, &FirmwareUpgradeController::_eraseProgressTick);

    _initFirmwareHash();
    _determinePX4StableVersion();
}

FirmwareUpgradeController::~FirmwareUpgradeController()
{
    qgcApp()->toolbox()->linkManager()->setConnectionsAllowed();
}

void FirmwareUpgradeController::startBoardSearch(void)
{
    LinkManager* linkMgr = qgcApp()->toolbox()->linkManager();

    linkMgr->setConnectionsSuspended(tr("Connect not allowed during Firmware Upgrade."));

    // FIXME: Why did we get here with active vehicle?
    if (!qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()) {
        // We have to disconnect any inactive links
        linkMgr->disconnectAll();
    }

    _bootloaderFound = false;
    _startFlashWhenBootloaderFound = false;
    _threadController->startFindBoardLoop();
}

void FirmwareUpgradeController::flash(AutoPilotStackType_t stackType,
                                      FirmwareType_t firmwareType,
                                      FirmwareVehicleType_t vehicleType)
{
    FirmwareIdentifier firmwareId = FirmwareIdentifier(stackType, firmwareType, vehicleType);
    if (_bootloaderFound) {
        _getFirmwareFile(firmwareId);
    } else {
        // We haven't found the bootloader yet. Need to wait until then to flash
        _startFlashWhenBootloaderFound = true;
        _startFlashWhenBootloaderFoundFirmwareIdentity = firmwareId;
    }
}

void FirmwareUpgradeController::flash(const FirmwareIdentifier& firmwareId)
{
    flash(firmwareId.autopilotStackType, firmwareId.firmwareType, firmwareId.firmwareVehicleType);
}

void FirmwareUpgradeController::cancel(void)
{
    _eraseTimer.stop();
    _threadController->cancel();
}

void FirmwareUpgradeController::_foundBoard(bool firstAttempt, const QSerialPortInfo& info, int boardType, QString boardName)
{
    _foundBoardInfo = info;
    _foundBoardType = (QGCSerialPortInfo::BoardType_t)boardType;
    _foundBoardTypeName = boardName;
    _startFlashWhenBootloaderFound = false;

    if (_foundBoardType == QGCSerialPortInfo::BoardTypeSiKRadio) {
        if (!firstAttempt) {
            // Radio always flashes latest firmware, so we can start right away without
            // any further user input.
            _startFlashWhenBootloaderFound = true;
            _startFlashWhenBootloaderFoundFirmwareIdentity = FirmwareIdentifier(ThreeDRRadio,
                                                                                StableFirmware,
                                                                                DefaultVehicleFirmware);
        }
    }
    
    qCDebug(FirmwareUpgradeLog) << _foundBoardType << _foundBoardTypeName;
    emit boardFound();
}


void FirmwareUpgradeController::_noBoardFound(void)
{
    emit noBoardFound();
}

void FirmwareUpgradeController::_boardGone(void)
{
    emit boardGone();
}

/// @brief Called when the bootloader is connected to by the findBootloader process. Moves the state machine
///         to the next step.
void FirmwareUpgradeController::_foundBootloader(int bootloaderVersion, int boardID, int flashSize)
{
    _bootloaderFound = true;
    _bootloaderVersion = bootloaderVersion;
    _bootloaderBoardID = boardID;
    _bootloaderBoardFlashSize = flashSize;
    
    _appendStatusLog("Connected to bootloader:");
    _appendStatusLog(QString("  Version: %1").arg(_bootloaderVersion));
    _appendStatusLog(QString("  Board ID: %1").arg(_bootloaderBoardID));
    _appendStatusLog(QString("  Flash size: %1").arg(_bootloaderBoardFlashSize));
    
    if (_startFlashWhenBootloaderFound) {
        flash(_startFlashWhenBootloaderFoundFirmwareIdentity);
    }

    _loadAPMVersions(_bootloaderBoardID);
}


/// @brief intializes the firmware hashes with proper urls.
/// This happens only once for a class instance first time when it is needed.
void FirmwareUpgradeController::_initFirmwareHash()
{
    // indirect check whether this function has been called before or not
    // may have to be modified if _rgPX4FMUV2Firmware disappears
    if (!_rgPX4FMUV2Firmware.isEmpty()) {
        return;
    }

    //////////////////////////////////// PX4FMUV4 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgPX4FMV4FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/px4fmu-v4_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/px4fmu-v4_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/px4fmu-v4_default.px4"},
        { AutoPilotStackAPM, StableFirmware,    QuadFirmware,           "http://firmware.ardupilot.org/Copter/stable/PX4-quad/ArduCopter-v4.px4"},
        { AutoPilotStackAPM, StableFirmware,    X8Firmware,             "http://firmware.ardupilot.org/Copter/stable/PX4-octa-quad/ArduCopter-v4.px4"},
        { AutoPilotStackAPM, StableFirmware,    HexaFirmware,           "http://firmware.ardupilot.org/Copter/stable/PX4-hexa/ArduCopter-v4.px4"},
        { AutoPilotStackAPM, StableFirmware,    OctoFirmware,           "http://firmware.ardupilot.org/Copter/stable/PX4-octa/ArduCopter-v4.px4"},
        { AutoPilotStackAPM, StableFirmware,    YFirmware,              "http://firmware.ardupilot.org/Copter/stable/PX4-tri/ArduCopter-v4.px4"},
        { AutoPilotStackAPM, StableFirmware,    Y6Firmware,             "http://firmware.ardupilot.org/Copter/stable/PX4-y6/ArduCopter-v4.px4"},
        { AutoPilotStackAPM, StableFirmware,    HeliFirmware,           "http://firmware.ardupilot.org/Copter/stable/PX4-heli/ArduCopter-v4.px4"},
        { AutoPilotStackAPM, StableFirmware,    PlaneFirmware,          "http://firmware.ardupilot.org/Plane/stable/PX4/ArduPlane-v4.px4"},
        { AutoPilotStackAPM, StableFirmware,    RoverFirmware,          "http://firmware.ardupilot.org/Rover/stable/PX4/APMrover2-v4.px4"},
        { AutoPilotStackAPM, BetaFirmware,      QuadFirmware,           "http://firmware.ardupilot.org/Copter/beta/PX4-quad/ArduCopter-v4.px4"},
        { AutoPilotStackAPM, BetaFirmware,      X8Firmware,             "http://firmware.ardupilot.org/Copter/beta/PX4-octa-quad/ArduCopter-v4.px4"},
        { AutoPilotStackAPM, BetaFirmware,      HexaFirmware,           "http://firmware.ardupilot.org/Copter/beta/PX4-hexa/ArduCopter-v4.px4"},
        { AutoPilotStackAPM, BetaFirmware,      OctoFirmware,           "http://firmware.ardupilot.org/Copter/beta/PX4-octa/ArduCopter-v4.px4"},
        { AutoPilotStackAPM, BetaFirmware,      YFirmware,              "http://firmware.ardupilot.org/Copter/beta/PX4-tri/ArduCopter-v4.px4"},
        { AutoPilotStackAPM, BetaFirmware,      Y6Firmware,             "http://firmware.ardupilot.org/Copter/beta/PX4-y6/ArduCopter-v4.px4"},
        { AutoPilotStackAPM, BetaFirmware,      HeliFirmware,           "http://firmware.ardupilot.org/Copter/beta/PX4-heli/ArduCopter-v4.px4"},
        { AutoPilotStackAPM, BetaFirmware,      PlaneFirmware,          "http://firmware.ardupilot.org/Plane/beta/PX4/ArduPlane-v4.px4"},
        { AutoPilotStackAPM, BetaFirmware,      RoverFirmware,          "http://firmware.ardupilot.org/Rover/beta/PX4/APMrover2-v4.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, CopterFirmware,         "http://firmware.ardupilot.org/Copter/latest/PX4/ArduCopter-v4.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, HeliFirmware,           "http://firmware.ardupilot.org/Copter/latest/PX4-heli/ArduCopter-v4.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, PlaneFirmware,          "http://firmware.ardupilot.org/Plane/latest/PX4/ArduPlane-v4.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, RoverFirmware,          "http://firmware.ardupilot.org/Rover/latest/PX4/APMrover2-v4.px4"}
    };

    //////////////////////////////////// PX4FMUV2 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgPX4FMV2FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/px4fmu-v2_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/px4fmu-v2_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/px4fmu-v2_default.px4"},
        { AutoPilotStackAPM, StableFirmware,    QuadFirmware,           "http://firmware.ardupilot.org/Copter/stable/PX4-quad/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    X8Firmware,             "http://firmware.ardupilot.org/Copter/stable/PX4-octa-quad/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    HexaFirmware,           "http://firmware.ardupilot.org/Copter/stable/PX4-hexa/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    OctoFirmware,           "http://firmware.ardupilot.org/Copter/stable/PX4-octa/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    YFirmware,              "http://firmware.ardupilot.org/Copter/stable/PX4-tri/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    Y6Firmware,             "http://firmware.ardupilot.org/Copter/stable/PX4-y6/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    HeliFirmware,           "http://firmware.ardupilot.org/Copter/stable/PX4-heli/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    PlaneFirmware,          "http://firmware.ardupilot.org/Plane/stable/PX4/ArduPlane-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    RoverFirmware,          "http://firmware.ardupilot.org/Rover/stable/PX4/APMrover2-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      QuadFirmware,           "http://firmware.ardupilot.org/Copter/beta/PX4-quad/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      X8Firmware,             "http://firmware.ardupilot.org/Copter/beta/PX4-octa-quad/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      HexaFirmware,           "http://firmware.ardupilot.org/Copter/beta/PX4-hexa/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      OctoFirmware,           "http://firmware.ardupilot.org/Copter/beta/PX4-octa/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      YFirmware,              "http://firmware.ardupilot.org/Copter/beta/PX4-tri/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      Y6Firmware,             "http://firmware.ardupilot.org/Copter/beta/PX4-y6/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      HeliFirmware,           "http://firmware.ardupilot.org/Copter/beta/PX4-heli/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      PlaneFirmware,          "http://firmware.ardupilot.org/Plane/beta/PX4/ArduPlane-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      RoverFirmware,          "http://firmware.ardupilot.org/Rover/beta/PX4/APMrover2-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, CopterFirmware,         "http://firmware.ardupilot.org/Copter/latest/PX4/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, HeliFirmware,           "http://firmware.ardupilot.org/Copter/latest/PX4-heli/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, PlaneFirmware,          "http://firmware.ardupilot.org/Plane/latest/PX4/ArduPlane-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, RoverFirmware,          "http://firmware.ardupilot.org/Rover/latest/PX4/APMrover2-v2.px4"}
    };

    //////////////////////////////////// PX4FMU aerocore firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t  rgAeroCoreFirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://gumstix-aerocore.s3.amazonaws.com/PX4/stable/aerocore_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://gumstix-aerocore.s3.amazonaws.com/PX4/beta/aerocore_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://gumstix-aerocore.s3.amazonaws.com/PX4/master/aerocore_default.px4"},
        { AutoPilotStackAPM, StableFirmware,    QuadFirmware,           "http://gumstix-aerocore.s3.amazonaws.com/APM/Copter/stable/PX4-quad/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    X8Firmware,             "http://gumstix-aerocore.s3.amazonaws.com/Copter/stable/PX4-octa-quad/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    HexaFirmware,           "http://gumstix-aerocore.s3.amazonaws.com/Copter/stable/PX4-hexa/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    OctoFirmware,           "http://gumstix-aerocore.s3.amazonaws.com/Copter/stable/PX4-octa/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    YFirmware,              "http://gumstix-aerocore.s3.amazonaws.com/Copter/stable/PX4-tri/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    Y6Firmware,             "http://gumstix-aerocore.s3.amazonaws.com/Copter/stable/PX4-y6/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    HeliFirmware,           "http://gumstix-aerocore.s3.amazonaws.com/Copter/stable/PX4-heli/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    PlaneFirmware,          "http://gumstix-aerocore.s3.amazonaws.com/Plane/stable/PX4/ArduPlane-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    RoverFirmware,          "http://gumstix-aerocore.s3.amazonaws.com/Rover/stable/PX4/APMrover2-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      QuadFirmware,           "http://gumstix-aerocore.s3.amazonaws.com/Copter/beta/PX4-quad/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      X8Firmware,             "http://gumstix-aerocore.s3.amazonaws.com/Copter/beta/PX4-octa-quad/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      HexaFirmware,           "http://gumstix-aerocore.s3.amazonaws.com/Copter/beta/PX4-hexa/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      OctoFirmware,           "http://gumstix-aerocore.s3.amazonaws.com/Copter/beta/PX4-octa/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      YFirmware,              "http://gumstix-aerocore.s3.amazonaws.com/Copter/beta/PX4-tri/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      Y6Firmware,             "http://gumstix-aerocore.s3.amazonaws.com/Copter/beta/PX4-y6/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      HeliFirmware,           "http://gumstix-aerocore.s3.amazonaws.com/Copter/beta/PX4-heli/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      PlaneFirmware,          "http://gumstix-aerocore.s3.amazonaws.com/Plane/beta/PX4/ArduPlane-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      RoverFirmware,          "http://gumstix-aerocore.s3.amazonaws.com/Rover/beta/PX4/APMrover2-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, QuadFirmware,           "http://gumstix-aerocore.s3.amazonaws.com/Copter/latest/PX4-quad/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, X8Firmware,             "http://gumstix-aerocore.s3.amazonaws.com/Copter/latest/PX4-octa-quad/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, HexaFirmware,           "http://gumstix-aerocore.s3.amazonaws.com/Copter/latest/PX4-hexa/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, OctoFirmware,           "http://gumstix-aerocore.s3.amazonaws.com/Copter/latest/PX4-octa/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, YFirmware,              "http://gumstix-aerocore.s3.amazonaws.com/Copter/latest/PX4-tri/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, Y6Firmware,             "http://gumstix-aerocore.s3.amazonaws.com/Copter/latest/PX4-y6/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, HeliFirmware,           "http://gumstix-aerocore.s3.amazonaws.com/Copter/latest/PX4-heli/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, PlaneFirmware,          "http://gumstix-aerocore.s3.amazonaws.com/Plane/latest/PX4/ArduPlane-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, RoverFirmware,          "http://gumstix-aerocore.s3.amazonaws.com/Rover/latest/PX4/APMrover2-v2.px4"}
    };

    /////////////////////////////// FMUV1 firmwares ///////////////////////////////////////////
    FirmwareToUrlElement_t rgPX4FMUV1FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/px4fmu-v1_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/px4fmu-v1_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/px4fmu-v1_default.px4"},
        { AutoPilotStackAPM, StableFirmware,    QuadFirmware,           "http://firmware.ardupilot.org/Copter/stable/PX4-quad/ArduCopter-v1.px4"},
        { AutoPilotStackAPM, StableFirmware,    X8Firmware,             "http://firmware.ardupilot.org/Copter/stable/PX4-octa-quad/ArduCopter-v1.px4"},
        { AutoPilotStackAPM, StableFirmware,    HexaFirmware,           "http://firmware.ardupilot.org/Copter/stable/PX4-hexa/ArduCopter-v1.px4"},
        { AutoPilotStackAPM, StableFirmware,    OctoFirmware,           "http://firmware.ardupilot.org/Copter/stable/PX4-octa/ArduCopter-v1.px4"},
        { AutoPilotStackAPM, StableFirmware,    YFirmware,              "http://firmware.ardupilot.org/Copter/stable/PX4-tri/ArduCopter-v1.px4"},
        { AutoPilotStackAPM, StableFirmware,    Y6Firmware,             "http://firmware.ardupilot.org/Copter/stable/PX4-y6/ArduCopter-v1.px4"},
        { AutoPilotStackAPM, StableFirmware,    HeliFirmware,           "http://firmware.ardupilot.org/Copter/stable/PX4-heli/ArduCopter-v1.px4"},
        { AutoPilotStackAPM, StableFirmware,    PlaneFirmware,          "http://firmware.ardupilot.org/Plane/stable/PX4/ArduPlane-v1.px4"},
        { AutoPilotStackAPM, StableFirmware,    RoverFirmware,          "http://firmware.ardupilot.org/Rover/stable/PX4/APMrover2-v1.px4"},
        { AutoPilotStackAPM, BetaFirmware,      QuadFirmware,           "http://firmware.ardupilot.org/Copter/beta/PX4-quad/ArduCopter-v1.px4"},
        { AutoPilotStackAPM, BetaFirmware,      X8Firmware,             "http://firmware.ardupilot.org/Copter/beta/PX4-octa-quad/ArduCopter-v1.px4"},
        { AutoPilotStackAPM, BetaFirmware,      HexaFirmware,           "http://firmware.ardupilot.org/Copter/beta/PX4-hexa/ArduCopter-v1.px4"},
        { AutoPilotStackAPM, BetaFirmware,      OctoFirmware,           "http://firmware.ardupilot.org/Copter/beta/PX4-octa/ArduCopter-v1.px4"},
        { AutoPilotStackAPM, BetaFirmware,      YFirmware,              "http://firmware.ardupilot.org/Copter/beta/PX4-tri/ArduCopter-v1.px4"},
        { AutoPilotStackAPM, BetaFirmware,      Y6Firmware,             "http://firmware.ardupilot.org/Copter/beta/PX4-y6/ArduCopter-v1.px4"},
        { AutoPilotStackAPM, BetaFirmware,      HeliFirmware,           "http://firmware.ardupilot.org/Copter/beta/PX4-heli/ArduCopter-v1.px4"},
        { AutoPilotStackAPM, BetaFirmware,      PlaneFirmware,          "http://firmware.ardupilot.org/Plane/beta/PX4/ArduPlane-v1.px4"},
        { AutoPilotStackAPM, BetaFirmware,      RoverFirmware,          "http://firmware.ardupilot.org/Rover/beta/PX4/APMrover2-v1.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, CopterFirmware,         "http://firmware.ardupilot.org/Copter/latest/PX4/ArduCopter-v1.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, HeliFirmware,           "http://firmware.ardupilot.org/Copter/latest/PX4-heli/ArduCopter-v1.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, PlaneFirmware,          "http://firmware.ardupilot.org/Plane/latest/PX4/ArduPlane-v1.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, RoverFirmware,          "http://firmware.ardupilot.org/Rover/latest/PX4/APMrover2-v1.px4"}
    };

    //////////////////////////////////// MindPXFMUV2 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgMindPXFMUV2FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/mindpx-v2_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/mindpx-v2_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/mindpx-v2_default.px4"},
    };
    //////////////////////////////////// TAPV1 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgTAPV1FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/tap-v1_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/tap-v1_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/tap-v1_default.px4"},
    };
    //////////////////////////////////// ASCV1 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgASCV1FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/asc-v1_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/asc-v1_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/asc-v1_default.px4"},
    };
    /////////////////////////////// px4flow firmwares ///////////////////////////////////////
    FirmwareToUrlElement_t rgPX4FLowFirmwareArray[] = {
        { PX4Flow, StableFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Flow/master/px4flow.px4" },
    };

    /////////////////////////////// 3dr radio firmwares ///////////////////////////////////////
    FirmwareToUrlElement_t rg3DRRadioFirmwareArray[] = {
        { ThreeDRRadio, StableFirmware, DefaultVehicleFirmware, "http://firmware.ardupilot.org/SiK/stable/radio~hm_trp.ihx"}
    };

    // populate hashes now
    int size = sizeof(rgPX4FMV4FirmwareArray)/sizeof(rgPX4FMV4FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgPX4FMV4FirmwareArray[i];
        _rgPX4FMUV4Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgPX4FMV2FirmwareArray)/sizeof(rgPX4FMV2FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgPX4FMV2FirmwareArray[i];
        _rgPX4FMUV2Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgAeroCoreFirmwareArray)/sizeof(rgAeroCoreFirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgAeroCoreFirmwareArray[i];
        _rgAeroCoreFirmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgPX4FMUV1FirmwareArray)/sizeof(rgPX4FMUV1FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgPX4FMUV1FirmwareArray[i];
        _rgPX4FMUV1Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgMindPXFMUV2FirmwareArray)/sizeof(rgMindPXFMUV2FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgMindPXFMUV2FirmwareArray[i];
        _rgMindPXFMUV2Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgTAPV1FirmwareArray)/sizeof(rgTAPV1FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgTAPV1FirmwareArray[i];
        _rgTAPV1Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgASCV1FirmwareArray)/sizeof(rgASCV1FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgASCV1FirmwareArray[i];
        _rgASCV1Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgPX4FLowFirmwareArray)/sizeof(rgPX4FLowFirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgPX4FLowFirmwareArray[i];
        _rgPX4FLowFirmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rg3DRRadioFirmwareArray)/sizeof(rg3DRRadioFirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rg3DRRadioFirmwareArray[i];
        _rg3DRRadioFirmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }
}

/// @brief Called when the findBootloader process is unable to sync to the bootloader. Moves the state
///         machine to the appropriate error state.
void FirmwareUpgradeController::_bootloaderSyncFailed(void)
{
    _errorCancel("Unable to sync with bootloader.");
}

QHash<FirmwareUpgradeController::FirmwareIdentifier, QString>* FirmwareUpgradeController::_firmwareHashForBoardId(int boardId)
{
    switch (boardId) {
    case Bootloader::boardIDPX4FMUV1:
        return &_rgPX4FMUV1Firmware;
    case Bootloader::boardIDPX4Flow:
        return &_rgPX4FLowFirmware;
    case Bootloader::boardIDPX4FMUV2:
        return &_rgPX4FMUV2Firmware;
    case Bootloader::boardIDPX4FMUV4:
        return &_rgPX4FMUV4Firmware;
    case Bootloader::boardIDAeroCore:
        return &_rgAeroCoreFirmware;
    case Bootloader::boardIDMINDPXFMUV2:
        return &_rgMindPXFMUV2Firmware;
    case Bootloader::boardIDTAPV1:
        return &_rgTAPV1Firmware;
    case Bootloader::boardIDASCV1:
        return &_rgASCV1Firmware;
    case Bootloader::boardID3DRRadio:
        return &_rg3DRRadioFirmware;
    default:
        return NULL;
    }
}

/// @brief Prompts the user to select a firmware file if needed and moves the state machine to the next state.
void FirmwareUpgradeController::_getFirmwareFile(FirmwareIdentifier firmwareId)
{
    QHash<FirmwareIdentifier, QString>* prgFirmware = _firmwareHashForBoardId(_bootloaderBoardID);
    
    if (!prgFirmware && firmwareId.firmwareType != CustomFirmware) {
        _errorCancel("Attempting to flash an unknown board type, you must select 'Custom firmware file'");
        return;
    }
    
    if (firmwareId.firmwareType == CustomFirmware) {
        _firmwareFilename = QGCFileDialog::getOpenFileName(NULL,                                                                // Parent to main window
                                                           "Select Firmware File",                                              // Dialog Caption
                                                           QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), // Initial directory
                                                           "Firmware Files (*.px4 *.bin *.ihx)");                               // File filter
    } else {

        if (prgFirmware->contains(firmwareId)) {
            _firmwareFilename = prgFirmware->value(firmwareId);
        } else {
            _errorCancel("Unable to find specified firmware download location");
            return;
        }
    }
    
    if (_firmwareFilename.isEmpty()) {
        _errorCancel("No firmware file selected");
    } else {
        _downloadFirmware();
    }
}

/// @brief Begins the process of downloading the selected firmware file.
void FirmwareUpgradeController::_downloadFirmware(void)
{
    Q_ASSERT(!_firmwareFilename.isEmpty());
    
    _appendStatusLog("Downloading firmware...");
    _appendStatusLog(QString(" From: %1").arg(_firmwareFilename));
    
    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::downloadFinished, this, &FirmwareUpgradeController::_firmwareDownloadFinished);
    connect(downloader, &QGCFileDownload::downloadProgress, this, &FirmwareUpgradeController::_firmwareDownloadProgress);
    connect(downloader, &QGCFileDownload::error,            this, &FirmwareUpgradeController::_firmwareDownloadError);
    downloader->download(_firmwareFilename);
}

/// @brief Updates the progress indicator while downloading
void FirmwareUpgradeController::_firmwareDownloadProgress(qint64 curr, qint64 total)
{
    // Take care of cases where 0 / 0 is emitted as error return value
    if (total > 0) {
        _progressBar->setProperty("value", (float)curr / (float)total);
    }
}

/// @brief Called when the firmware download completes.
void FirmwareUpgradeController::_firmwareDownloadFinished(QString remoteFile, QString localFile)
{
    Q_UNUSED(remoteFile);

    _appendStatusLog("Download complete");
    
    FirmwareImage* image = new FirmwareImage(this);
    
    connect(image, &FirmwareImage::statusMessage, this, &FirmwareUpgradeController::_status);
    connect(image, &FirmwareImage::errorMessage, this, &FirmwareUpgradeController::_error);
    
    if (!image->load(localFile, _bootloaderBoardID)) {
        _errorCancel("Image load failed");
        return;
    }
    
    // We can't proceed unless we have the bootloader
    if (!_bootloaderFound) {
        _errorCancel("Bootloader not found");
        return;
    }
    
    if (_bootloaderBoardFlashSize != 0 && image->imageSize() > _bootloaderBoardFlashSize) {
        _errorCancel(QString("Image size of %1 is too large for board flash size %2").arg(image->imageSize()).arg(_bootloaderBoardFlashSize));
        return;
    }

    _threadController->flash(image);
}

/// @brief Called when an error occurs during download
void FirmwareUpgradeController::_firmwareDownloadError(QString errorMsg)
{
    _errorCancel(errorMsg);
}

/// @brief returns firmware type as a string
QString FirmwareUpgradeController::firmwareTypeAsString(FirmwareType_t type) const
{
    switch (type) {
    case StableFirmware:
        return "stable";
    case DeveloperFirmware:
        return "developer";
    case BetaFirmware:
        return "beta";
    default:
        return "custom";
    }
}

/// @brief Signals completion of one of the specified bootloader commands. Moves the state machine to the
///         appropriate next step.
void FirmwareUpgradeController::_flashComplete(void)
{
    delete _image;
    _image = NULL;
    
    _appendStatusLog("Upgrade complete", true);
    _appendStatusLog("------------------------------------------", false);
    emit flashComplete();
    qgcApp()->toolbox()->linkManager()->setConnectionsAllowed();
}

void FirmwareUpgradeController::_error(const QString& errorString)
{
    delete _image;
    _image = NULL;
    
    _errorCancel(QString("Error: %1").arg(errorString));
}

void FirmwareUpgradeController::_status(const QString& statusString)
{
    _appendStatusLog(statusString);
}

/// @brief Updates the progress bar from long running bootloader commands
void FirmwareUpgradeController::_updateProgress(int curr, int total)
{
    // Take care of cases where 0 / 0 is emitted as error return value
    if (total > 0) {
        _progressBar->setProperty("value", (float)curr / (float)total);
    }
}

/// @brief Moves the progress bar ahead on tick while erasing the board
void FirmwareUpgradeController::_eraseProgressTick(void)
{
    _eraseTickCount++;
    _progressBar->setProperty("value", (float)(_eraseTickCount*_eraseTickMsec) / (float)_eraseTotalMsec);
}

/// Appends the specified text to the status log area in the ui
void FirmwareUpgradeController::_appendStatusLog(const QString& text, bool critical)
{
    Q_ASSERT(_statusLog);
    
    QVariant returnedValue;
    QVariant varText;
    
    if (critical) {
        varText = QString("<font color=\"yellow\">%1</font>").arg(text);
    } else {
        varText = text;
    }
    
    QMetaObject::invokeMethod(_statusLog,
                              "append",
                              Q_RETURN_ARG(QVariant, returnedValue),
                              Q_ARG(QVariant, varText));
}

void FirmwareUpgradeController::_errorCancel(const QString& msg)
{
    _appendStatusLog(msg, false);
    _appendStatusLog("Upgrade cancelled", true);
    _appendStatusLog("------------------------------------------", false);
    emit error();
    cancel();
    qgcApp()->toolbox()->linkManager()->setConnectionsAllowed();
}

void FirmwareUpgradeController::_eraseStarted(void)
{
    // We set up our own progress bar for erase since the erase command does not provide one
    _eraseTickCount = 0;
    _eraseTimer.start(_eraseTickMsec);
}

void FirmwareUpgradeController::_eraseComplete(void)
{
    _eraseTimer.stop();
}

void FirmwareUpgradeController::_loadAPMVersions(uint32_t bootloaderBoardID)
{
    _apmVersionMap.clear();

    QHash<FirmwareIdentifier, QString>* prgFirmware = _firmwareHashForBoardId(bootloaderBoardID);

    foreach (FirmwareIdentifier firmwareId, prgFirmware->keys()) {
        if (firmwareId.autopilotStackType == AutoPilotStackAPM) {
            QString versionFile = QFileInfo(prgFirmware->value(firmwareId)).path() + "/git-version.txt";

            qCDebug(FirmwareUpgradeLog) << "Downloading" << versionFile;
            QGCFileDownload* downloader = new QGCFileDownload(this);
            connect(downloader, &QGCFileDownload::downloadFinished, this, &FirmwareUpgradeController::_apmVersionDownloadFinished);
            downloader->download(versionFile);
        }
    }
}

void FirmwareUpgradeController::_apmVersionDownloadFinished(QString remoteFile, QString localFile)
{
    qCDebug(FirmwareUpgradeLog) << "Download complete" << remoteFile << localFile;

    // Now read the version file and pull out the version string

    QFile versionFile(localFile);
    versionFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream stream(&versionFile);
    QString versionContents = stream.readAll();

    QString version;
    QRegularExpression re("APMVERSION: (.*)$");
    QRegularExpressionMatch match = re.match(versionContents);
    if (match.hasMatch()) {
        version = match.captured(1);
    }

    if (version.isEmpty()) {
        qWarning() << "Unable to parse version info from file" << remoteFile;
        return;
    }

    // In order to determine the firmware and vehicle type for this file we find the matching entry in the firmware list

    QHash<FirmwareIdentifier, QString>* prgFirmware = _firmwareHashForBoardId(_bootloaderBoardID);

    QString remotePath = QFileInfo(remoteFile).path();
    foreach (FirmwareIdentifier firmwareId, prgFirmware->keys()) {
        if (remotePath == QFileInfo((*prgFirmware)[firmwareId]).path()) {
            qCDebug(FirmwareUpgradeLog) << "Adding version to map, version:firwmareType:vehicleType" << version << firmwareId.firmwareType << firmwareId.firmwareVehicleType;
            _apmVersionMap[firmwareId.firmwareType][firmwareId.firmwareVehicleType] = version;
        }
    }

    emit apmAvailableVersionsChanged();
}

void FirmwareUpgradeController::setSelectedFirmwareType(FirmwareType_t firmwareType)
{
    _selectedFirmwareType = firmwareType;
    emit selectedFirmwareTypeChanged(_selectedFirmwareType);
    emit apmAvailableVersionsChanged();
}

QStringList FirmwareUpgradeController::apmAvailableVersions(void)
{
    QStringList list;

    _apmVehicleTypeFromCurrentVersionList.clear();

    foreach (FirmwareVehicleType_t vehicleType, _apmVersionMap[_selectedFirmwareType].keys()) {
        QString version;

        switch (vehicleType) {
        case QuadFirmware:
            version = "Quad - ";
            break;
        case X8Firmware:
            version = "X8 - ";
            break;
        case HexaFirmware:
            version = "Hexa - ";
            break;
        case OctoFirmware:
            version = "Octo - ";
            break;
        case YFirmware:
            version = "Y - ";
            break;
        case Y6Firmware:
            version = "Y6 - ";
            break;
        case HeliFirmware:
            version = "Heli - ";
            break;
        case CopterFirmware:
            version = "MultiRotor - ";
            break;
            break;
        case PlaneFirmware:
        case RoverFirmware:
        case DefaultVehicleFirmware:
            break;
        }

        version += _apmVersionMap[_selectedFirmwareType][vehicleType];
        _apmVehicleTypeFromCurrentVersionList.append(vehicleType);

        list << version;
    }

    return list;
}

FirmwareUpgradeController::FirmwareVehicleType_t FirmwareUpgradeController::vehicleTypeFromVersionIndex(int index)
{
    if (index < 0 || index >= _apmVehicleTypeFromCurrentVersionList.count()) {
        qWarning() << "Invalid index, index:count" << index << _apmVehicleTypeFromCurrentVersionList.count();
        return QuadFirmware;
    }

    return _apmVehicleTypeFromCurrentVersionList[index];
}

void FirmwareUpgradeController::_determinePX4StableVersion(void)
{
    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::downloadFinished, this, &FirmwareUpgradeController::_px4ReleasesGithubDownloadFinished);
    connect(downloader, &QGCFileDownload::error, this, &FirmwareUpgradeController::_px4ReleasesGithubDownloadError);
    downloader->download(QStringLiteral("https://api.github.com/repos/PX4/Firmware/releases"));
}

void FirmwareUpgradeController::_px4ReleasesGithubDownloadFinished(QString remoteFile, QString localFile)
{
    Q_UNUSED(remoteFile);

    QFile jsonFile(localFile);
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(FirmwareUpgradeLog) << "Unable to open github px4 releases json file" << localFile << jsonFile.errorString();
        return;
    }
    QByteArray bytes = jsonFile.readAll();
    jsonFile.close();

    QJsonParseError jsonParseError;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        qCWarning(FirmwareUpgradeLog) <<  "Unable to open px4 releases json document" << localFile << jsonParseError.errorString();
        return;
    }

    // Json should be an array of release objects
    if (!doc.isArray()) {
        qCWarning(FirmwareUpgradeLog) <<  "px4 releases json document is not an array" << localFile;
        return;
    }
    QJsonArray releases = doc.array();

    // The first release marked prerelease=false is stable
    // The first release marked prerelease=true is beta
    bool foundStable = false;
    bool foundBeta = false;
    for (int i=0; i<releases.count() && (!foundStable || !foundBeta); i++) {
        QJsonObject release = releases[i].toObject();
        if (!foundStable && !release["prerelease"].toBool()) {
            _px4StableVersion = release["name"].toString();
            emit px4StableVersionChanged(_px4StableVersion);
            qCDebug(FirmwareUpgradeLog()) << "Found px4 stable version" << _px4StableVersion;
            foundStable = true;
        } else if (!foundBeta && release["prerelease"].toBool()) {
            _px4BetaVersion = release["name"].toString();
            emit px4StableVersionChanged(_px4BetaVersion);
            qCDebug(FirmwareUpgradeLog()) << "Found px4 beta version" << _px4BetaVersion;
            foundBeta = true;
        }
    }

    if (!foundStable) {
        qCDebug(FirmwareUpgradeLog()) << "Unable to find px4 stable version" << localFile;
    }
    if (!foundBeta) {
        qCDebug(FirmwareUpgradeLog()) << "Unable to find px4 beta version" << localFile;
    }
}

void FirmwareUpgradeController::_px4ReleasesGithubDownloadError(QString errorMsg)
{
    qCWarning(FirmwareUpgradeLog) << "PX4 releases github download failed" << errorMsg;
}
