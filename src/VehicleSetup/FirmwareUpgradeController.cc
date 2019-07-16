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
#include "QGCQFileDialog.h"
#include "QGCApplication.h"
#include "QGCFileDownload.h"
#include "QGCOptions.h"
#include "QGCCorePlugin.h"

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
    return static_cast<uint>(( firmwareId.autopilotStackType |
             (firmwareId.firmwareType << 8) |
             (firmwareId.firmwareVehicleType << 16) ));
}

/// @Brief Constructs a new FirmwareUpgradeController Widget. This widget is used within the PX4VehicleConfig set of screens.
FirmwareUpgradeController::FirmwareUpgradeController(void)
    : _singleFirmwareURL                (qgcApp()->toolbox()->corePlugin()->options()->firmwareUpgradeSingleURL())
    , _singleFirmwareMode               (!_singleFirmwareURL.isEmpty())
    , _downloadManager                  (nullptr)
    , _downloadNetworkReply             (nullptr)
    , _statusLog                        (nullptr)
    , _selectedFirmwareType             (StableFirmware)
    , _image                            (nullptr)
    , _apmBoardDescriptionReplaceText   ("<APMBoardDescription>")
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
    qCDebug(FirmwareUpgradeLog) << "_flash stackType:firmwareType:vehicleType" << stackType << firmwareType << vehicleType;
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

void FirmwareUpgradeController::flashSingleFirmwareMode(FirmwareType_t firmwareType)
{
    flash(SingleFirmwareMode, firmwareType, DefaultVehicleFirmware);
}

void FirmwareUpgradeController::cancel(void)
{
    _eraseTimer.stop();
    _threadController->cancel();
}

void FirmwareUpgradeController::_foundBoard(bool firstAttempt, const QSerialPortInfo& info, int boardType, QString boardName)
{
    _foundBoardInfo = info;
    _foundBoardType = static_cast<QGCSerialPortInfo::BoardType_t>(boardType);
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
    _bootloaderVersion = static_cast<uint32_t>(bootloaderVersion);
    _bootloaderBoardID = static_cast<uint32_t>(boardID);
    _bootloaderBoardFlashSize = static_cast<uint32_t>(flashSize);
    
    _appendStatusLog(tr("Connected to bootloader:"));
    _appendStatusLog(tr("  Version: %1").arg(_bootloaderVersion));
    _appendStatusLog(tr("  Board ID: %1").arg(_bootloaderBoardID));
    _appendStatusLog(tr("  Flash size: %1").arg(_bootloaderBoardFlashSize));
    
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

    //////////////////////////////////// PX4FMU aerocore firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t  rgAeroCoreFirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://gumstix-aerocore.s3.amazonaws.com/PX4/stable/aerocore_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://gumstix-aerocore.s3.amazonaws.com/PX4/beta/aerocore_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://gumstix-aerocore.s3.amazonaws.com/PX4/master/aerocore_default.px4"},
    #if !defined(NO_ARDUPILOT_DIALECT)
        { AutoPilotStackAPM, BetaFirmware,      CopterFirmware,         "http://firmware.ardupilot.org/Copter/beta/PX4/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    HeliFirmware,           "http://gumstix-aerocore.s3.amazonaws.com/Copter/stable/PX4-heli/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    PlaneFirmware,          "http://gumstix-aerocore.s3.amazonaws.com/Plane/stable/PX4/ArduPlane-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    RoverFirmware,          "http://gumstix-aerocore.s3.amazonaws.com/Rover/stable/PX4/APMrover2-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      HeliFirmware,           "http://firmware.ardupilot.org/Copter/beta/PX4-heli/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      PlaneFirmware,          "http://firmware.ardupilot.org/Plane/beta/PX4/ArduPlane-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      RoverFirmware,          "http://firmware.ardupilot.org/Rover/beta/PX4/APMrover2-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, CopterFirmware,         "http://gumstix-aerocore.s3.amazonaws.com/Copter/latest/PX4/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, HeliFirmware,           "http://gumstix-aerocore.s3.amazonaws.com/Copter/latest/PX4-heli/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, PlaneFirmware,          "http://gumstix-aerocore.s3.amazonaws.com/Plane/latest/PX4/ArduPlane-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, RoverFirmware,          "http://gumstix-aerocore.s3.amazonaws.com/Rover/latest/PX4/APMrover2-v2.px4"}
    #endif
    };

    //////////////////////////////////// AUAVX2_1 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgAUAVX2_1FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/auav-x21_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/auav-x21_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/auav-x21_default.px4"},
    #if !defined(NO_ARDUPILOT_DIALECT)
        { AutoPilotStackAPM, StableFirmware,    CopterFirmware,         "http://firmware.ardupilot.org/Copter/stable/PX4/ArduCopter-v3.px4"},
        { AutoPilotStackAPM, StableFirmware,    HeliFirmware,           "http://firmware.ardupilot.org/Copter/stable/PX4-heli/ArduCopter-v3.px4"},
        { AutoPilotStackAPM, StableFirmware,    PlaneFirmware,          "http://firmware.ardupilot.org/Plane/stable/PX4/ArduPlane-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    RoverFirmware,          "http://firmware.ardupilot.org/Rover/stable/PX4/APMrover2-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    SubFirmware,            "http://firmware.ardupilot.org/Sub/stable/PX4/ArduSub-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      CopterFirmware,         "http://firmware.ardupilot.org/Copter/beta/PX4/ArduCopter-v3.px4"},
        { AutoPilotStackAPM, BetaFirmware,      HeliFirmware,           "http://firmware.ardupilot.org/Copter/beta/PX4-heli/ArduCopter-v3.px4"},
        { AutoPilotStackAPM, BetaFirmware,      PlaneFirmware,          "http://firmware.ardupilot.org/Plane/beta/PX4/ArduPlane-v3.px4"},
        { AutoPilotStackAPM, BetaFirmware,      RoverFirmware,          "http://firmware.ardupilot.org/Rover/beta/PX4/APMrover2-v3.px4"},
        { AutoPilotStackAPM, BetaFirmware,      SubFirmware,            "http://firmware.ardupilot.org/Sub/beta/PX4/ArduSub-v3.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, CopterFirmware,         "http://firmware.ardupilot.org/Copter/latest/PX4/ArduCopter-v3.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, HeliFirmware,           "http://firmware.ardupilot.org/Copter/latest/PX4-heli/ArduCopter-v3.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, PlaneFirmware,          "http://firmware.ardupilot.org/Plane/latest/PX4/ArduPlane-v3.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, RoverFirmware,          "http://firmware.ardupilot.org/Rover/latest/PX4/APMrover2-v3.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, SubFirmware,            "http://firmware.ardupilot.org/Sub/latest/PX4/ArduSub-v3.px4"}
    #endif
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
        { SingleFirmwareMode,StableFirmware,    DefaultVehicleFirmware, _singleFirmwareURL},
    };
    //////////////////////////////////// ASCV1 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgASCV1FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/asc-v1_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/asc-v1_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/asc-v1_default.px4"},
    };

    //////////////////////////////////// Crazyflie 2.0 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgCrazyflie2FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/crazyflie_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/crazyflie_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/crazyflie_default.px4"},
    };

    //////////////////////////////////// Omnibus F4 SD firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgOmnibusF4SDFirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/omnibus_f4sd_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/omnibus_f4sd_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/omnibus_f4sd_default.px4"},
    };

    //////////////////////////////////// Kakute F7 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgKakuteF7FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/holybro_kakutef7_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/holybro_kakutef7_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/holybro_kakutef7_default.px4"},
    };
    
    //////////////////////////////////// FMUK66V3 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgFMUK66V3FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/nxp_fmuk66-v3_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/nxp_fmuk66-v3_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/nxp_fmuk66-v3_default.px4"},
    };

    /////////////////////////////// px4flow firmwares ///////////////////////////////////////
    FirmwareToUrlElement_t rgPX4FLowFirmwareArray[] = {
        { PX4FlowPX4, StableFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Flow/master/px4flow.px4" },
    #if !defined(NO_ARDUPILOT_DIALECT)
        { PX4FlowAPM, StableFirmware, DefaultVehicleFirmware, "http://firmware.ardupilot.org/Tools/PX4Flow/px4flow-klt-latest.px4" },
    #endif
    };

    /////////////////////////////// 3dr radio firmwares ///////////////////////////////////////
    FirmwareToUrlElement_t rg3DRRadioFirmwareArray[] = {
        { ThreeDRRadio, StableFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/SiK/stable/radio~hm_trp.ihx"}
    };

    // We build the maps for PX4 and ArduPilot firmwares dynamically using the data below

#if 0
    Example URLs for PX4 and ArduPilot
    { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/px4fmu-v4_default.px4"},
    { AutoPilotStackAPM, StableFirmware,    CopterFirmware,         "http://firmware.ardupilot.org/Copter/stable/PX4/ArduCopter-v4.px4"},
    { AutoPilotStackAPM, DeveloperFirmware, CopterChibiosFirmware,  "http://firmware.ardupilot.org/Copter/latest/fmuv4/arducopter.apj"},
#endif

    QString px4Url          ("http://px4-travis.s3.amazonaws.com/Firmware/%1/px4fmu-%2_default.px4");
    QString apmUrl          ("http://firmware.ardupilot.org/%1/%2/%3/%4-v%5.px4");
    QString apmChibiOSUrl   ("http://firmware.ardupilot.org/%1/%2/fmuv%3%4/%5.apj");

    QMap<FirmwareType_t, QString> px4MapFirmwareTypeToDir;
    px4MapFirmwareTypeToDir[StableFirmware] =       QStringLiteral("stable");
    px4MapFirmwareTypeToDir[BetaFirmware] =         QStringLiteral("beta");
    px4MapFirmwareTypeToDir[DeveloperFirmware] =    QStringLiteral("master");

#if !defined(NO_ARDUPILOT_DIALECT)
    QMap<FirmwareVehicleType_t, QString> apmMapVehicleTypeToDir;
    apmMapVehicleTypeToDir[CopterFirmware] =    QStringLiteral("Copter");
    apmMapVehicleTypeToDir[HeliFirmware] =      QStringLiteral("Copter");
    apmMapVehicleTypeToDir[PlaneFirmware] =     QStringLiteral("Plane");
    apmMapVehicleTypeToDir[RoverFirmware] =     QStringLiteral("Rover");
    apmMapVehicleTypeToDir[SubFirmware] =       QStringLiteral("Sub");
#endif

#if !defined(NO_ARDUPILOT_DIALECT)
    QMap<FirmwareVehicleType_t, QString> apmChibiOSMapVehicleTypeToDir;
    apmChibiOSMapVehicleTypeToDir[CopterChibiOSFirmware] =  QStringLiteral("Copter");
    apmChibiOSMapVehicleTypeToDir[HeliChibiOSFirmware] =    QStringLiteral("Copter");
    apmChibiOSMapVehicleTypeToDir[PlaneChibiOSFirmware] =   QStringLiteral("Plane");
    apmChibiOSMapVehicleTypeToDir[RoverChibiOSFirmware] =   QStringLiteral("Rover");
    apmChibiOSMapVehicleTypeToDir[SubChibiOSFirmware] =     QStringLiteral("Sub");
#endif

#if !defined(NO_ARDUPILOT_DIALECT)
    QMap<FirmwareType_t, QString> apmMapFirmwareTypeToDir;
    apmMapFirmwareTypeToDir[StableFirmware] =       QStringLiteral("stable");
    apmMapFirmwareTypeToDir[BetaFirmware] =         QStringLiteral("beta");
    apmMapFirmwareTypeToDir[DeveloperFirmware] =    QStringLiteral("latest");
#endif

#if !defined(NO_ARDUPILOT_DIALECT)
    QMap<FirmwareVehicleType_t, QString> apmMapVehicleTypeToPX4Dir;
    apmMapVehicleTypeToPX4Dir[CopterFirmware] = QStringLiteral("PX4");
    apmMapVehicleTypeToPX4Dir[HeliFirmware] =   QStringLiteral("PX4-heli");
    apmMapVehicleTypeToPX4Dir[PlaneFirmware] =  QStringLiteral("PX4");
    apmMapVehicleTypeToPX4Dir[RoverFirmware] =  QStringLiteral("PX4");
    apmMapVehicleTypeToPX4Dir[SubFirmware] =    QStringLiteral("PX4");
#endif

#if !defined(NO_ARDUPILOT_DIALECT)
    QMap<FirmwareVehicleType_t, QString> apmMapVehicleTypeToFilename;
    apmMapVehicleTypeToFilename[CopterFirmware] =   QStringLiteral("ArduCopter");
    apmMapVehicleTypeToFilename[HeliFirmware] =     QStringLiteral("ArduCopter");
    apmMapVehicleTypeToFilename[PlaneFirmware] =    QStringLiteral("ArduPlane");
    apmMapVehicleTypeToFilename[RoverFirmware] =    QStringLiteral("APMrover2");
    apmMapVehicleTypeToFilename[SubFirmware] =      QStringLiteral("ArduSub");
#endif

#if !defined(NO_ARDUPILOT_DIALECT)
    QMap<FirmwareVehicleType_t, QString> apmChibiOSMapVehicleTypeToFmuDir;
    apmChibiOSMapVehicleTypeToFmuDir[CopterChibiOSFirmware] =   QString();
    apmChibiOSMapVehicleTypeToFmuDir[HeliChibiOSFirmware] =     QStringLiteral("-heli");
    apmChibiOSMapVehicleTypeToFmuDir[PlaneChibiOSFirmware] =    QString();
    apmChibiOSMapVehicleTypeToFmuDir[RoverChibiOSFirmware] =    QString();
    apmChibiOSMapVehicleTypeToFmuDir[SubChibiOSFirmware] =      QString();
#endif

#if !defined(NO_ARDUPILOT_DIALECT)
    QMap<FirmwareVehicleType_t, QString> apmChibiOSMapVehicleTypeToFilename;
    apmChibiOSMapVehicleTypeToFilename[CopterChibiOSFirmware] = QStringLiteral("arducopter");
    apmChibiOSMapVehicleTypeToFilename[HeliChibiOSFirmware] =   QStringLiteral("arducopter-heli");
    apmChibiOSMapVehicleTypeToFilename[PlaneChibiOSFirmware] =  QStringLiteral("arduplane");
    apmChibiOSMapVehicleTypeToFilename[RoverChibiOSFirmware] =  QStringLiteral("ardurover");
    apmChibiOSMapVehicleTypeToFilename[SubChibiOSFirmware] =    QStringLiteral("ardusub");
#endif

    // PX4 Firmwares
    for (const FirmwareType_t& firmwareType: px4MapFirmwareTypeToDir.keys()) {
        QString dir = px4MapFirmwareTypeToDir[firmwareType];
        _rgFMUV5Firmware.insert      (FirmwareIdentifier(AutoPilotStackPX4, firmwareType, DefaultVehicleFirmware), px4Url.arg(dir).arg("v5"));
        _rgFMUV4PROFirmware.insert   (FirmwareIdentifier(AutoPilotStackPX4, firmwareType, DefaultVehicleFirmware), px4Url.arg(dir).arg("v4pro"));
        _rgFMUV4Firmware.insert      (FirmwareIdentifier(AutoPilotStackPX4, firmwareType, DefaultVehicleFirmware), px4Url.arg(dir).arg("v4"));
        _rgFMUV3Firmware.insert      (FirmwareIdentifier(AutoPilotStackPX4, firmwareType, DefaultVehicleFirmware), px4Url.arg(dir).arg("v3"));
        _rgPX4FMUV2Firmware.insert      (FirmwareIdentifier(AutoPilotStackPX4, firmwareType, DefaultVehicleFirmware), px4Url.arg(dir).arg("v2"));
    }

#if !defined(NO_ARDUPILOT_DIALECT)
    // ArduPilot Firmwares
    for (const FirmwareType_t& firmwareType: apmMapFirmwareTypeToDir.keys()) {
        QString firmwareTypeDir = apmMapFirmwareTypeToDir[firmwareType];
        for (const FirmwareVehicleType_t& vehicleType: apmMapVehicleTypeToDir.keys()) {
            QString vehicleTypeDir = apmMapVehicleTypeToDir[vehicleType];
            QString px4Dir = apmMapVehicleTypeToPX4Dir[vehicleType];
            QString filename = apmMapVehicleTypeToFilename[vehicleType];
            _rgFMUV5Firmware.insert  (FirmwareIdentifier(AutoPilotStackAPM, firmwareType, vehicleType), apmUrl.arg(vehicleTypeDir).arg(firmwareTypeDir).arg(px4Dir).arg(filename).arg("5"));
            _rgFMUV4Firmware.insert  (FirmwareIdentifier(AutoPilotStackAPM, firmwareType, vehicleType), apmUrl.arg(vehicleTypeDir).arg(firmwareTypeDir).arg(px4Dir).arg(filename).arg("4"));
            _rgFMUV3Firmware.insert  (FirmwareIdentifier(AutoPilotStackAPM, firmwareType, vehicleType), apmUrl.arg(vehicleTypeDir).arg(firmwareTypeDir).arg(px4Dir).arg(filename).arg("3"));
            _rgPX4FMUV2Firmware.insert  (FirmwareIdentifier(AutoPilotStackAPM, firmwareType, vehicleType), apmUrl.arg(vehicleTypeDir).arg(firmwareTypeDir).arg(px4Dir).arg(filename).arg("2"));
        }
    }
#endif

#if !defined(NO_ARDUPILOT_DIALECT)
    // ArduPilot ChibiOS Firmwares when board id is is a known type. These are added directly into the various board type hashes.
    for (const FirmwareType_t& firmwareType: apmMapFirmwareTypeToDir.keys()) {
        QString firmwareTypeDir = apmMapFirmwareTypeToDir[firmwareType];
        for (const FirmwareVehicleType_t& vehicleType: apmChibiOSMapVehicleTypeToDir.keys()) {
            QString vehicleTypeDir = apmChibiOSMapVehicleTypeToDir[vehicleType];
            QString fmuDir = apmChibiOSMapVehicleTypeToFmuDir[vehicleType];
            QString filename = apmChibiOSMapVehicleTypeToFilename[vehicleType];
            _rgFMUV5Firmware.insert      (FirmwareIdentifier(AutoPilotStackAPM, firmwareType, vehicleType), apmChibiOSUrl.arg(vehicleTypeDir).arg(firmwareTypeDir).arg("5").arg(fmuDir).arg(filename));
            _rgFMUV4Firmware.insert      (FirmwareIdentifier(AutoPilotStackAPM, firmwareType, vehicleType), apmChibiOSUrl.arg(vehicleTypeDir).arg(firmwareTypeDir).arg("4").arg(fmuDir).arg(filename));
            _rgFMUV3Firmware.insert      (FirmwareIdentifier(AutoPilotStackAPM, firmwareType, vehicleType), apmChibiOSUrl.arg(vehicleTypeDir).arg(firmwareTypeDir).arg("3").arg(fmuDir).arg(filename));
            _rgPX4FMUV2Firmware.insert      (FirmwareIdentifier(AutoPilotStackAPM, firmwareType, vehicleType), apmChibiOSUrl.arg(vehicleTypeDir).arg(firmwareTypeDir).arg("2").arg(fmuDir).arg(filename));
        }
    }
#endif

#if !defined(NO_ARDUPILOT_DIALECT)
    // ArduPilot ChibiOS Firmwares when board id is an unknown type but follows ArduPilot port info naming conventions.
    // This is only used if the board using the ArduPilot port naming scheme.
    for (const FirmwareType_t& firmwareType: apmMapFirmwareTypeToDir.keys()) {
        QString firmwareTypeDir = apmMapFirmwareTypeToDir[firmwareType];
        for (const FirmwareVehicleType_t& vehicleType: apmChibiOSMapVehicleTypeToDir.keys()) {
            QString namedURL("http://firmware.ardupilot.org/%1/%2/%3%4/%5.apj");
            QString vehicleTypeDir = apmChibiOSMapVehicleTypeToDir[vehicleType];
            QString fmuDir = apmChibiOSMapVehicleTypeToFmuDir[vehicleType];
            QString filename = apmChibiOSMapVehicleTypeToFilename[vehicleType];
            _rgAPMChibiosReplaceNamedBoardFirmware.insert(FirmwareIdentifier(AutoPilotStackAPM, firmwareType, vehicleType), namedURL.arg(vehicleTypeDir).arg(firmwareTypeDir).arg(_apmBoardDescriptionReplaceText).arg(fmuDir).arg(filename));
        }
    }
#endif

    int size = sizeof(rgAeroCoreFirmwareArray)/sizeof(rgAeroCoreFirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgAeroCoreFirmwareArray[i];
        _rgAeroCoreFirmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgAUAVX2_1FirmwareArray)/sizeof(rgAUAVX2_1FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgAUAVX2_1FirmwareArray[i];
        _rgAUAVX2_1Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
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

    size = sizeof(rgCrazyflie2FirmwareArray)/sizeof(rgCrazyflie2FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgCrazyflie2FirmwareArray[i];
        _rgCrazyflie2Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgOmnibusF4SDFirmwareArray)/sizeof(rgOmnibusF4SDFirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgOmnibusF4SDFirmwareArray[i];
        _rgOmnibusF4SDFirmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }
    
    size = sizeof(rgKakuteF7FirmwareArray)/sizeof(rgKakuteF7FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgKakuteF7FirmwareArray[i];
        _rgKakuteF7Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
    }

    size = sizeof(rgFMUK66V3FirmwareArray)/sizeof(rgFMUK66V3FirmwareArray[0]);
    for (int i = 0; i < size; i++) {
        const FirmwareToUrlElement_t& element = rgFMUK66V3FirmwareArray[i];
        _rgFMUK66V3Firmware.insert(FirmwareIdentifier(element.stackType, element.firmwareType, element.vehicleType), element.url);
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
    QHash<FirmwareUpgradeController::FirmwareIdentifier, QString>* rgFirmware;

    switch (boardId) {
    case Bootloader::boardIDPX4Flow:
        rgFirmware = &_rgPX4FLowFirmware;
        break;
    case Bootloader::boardIDPX4FMUV2:
        rgFirmware = &_rgPX4FMUV2Firmware;
        break;
    case Bootloader::boardIDPX4FMUV3:
        rgFirmware = &_rgFMUV3Firmware;
        break;
    case Bootloader::boardIDPX4FMUV4:
        rgFirmware = &_rgFMUV4Firmware;
        break;
    case Bootloader::boardIDPX4FMUV4PRO:
        rgFirmware = &_rgFMUV4PROFirmware;
        break;
    case Bootloader::boardIDPX4FMUV5:
        rgFirmware = &_rgFMUV5Firmware;
        break;
    case Bootloader::boardIDAeroCore:
        rgFirmware = &_rgAeroCoreFirmware;
        break;
    case Bootloader::boardIDAUAVX2_1:
        rgFirmware = &_rgAUAVX2_1Firmware;
        break;
    case Bootloader::boardIDMINDPXFMUV2:
        rgFirmware = &_rgMindPXFMUV2Firmware;
        break;
    case Bootloader::boardIDTAPV1:
        rgFirmware = &_rgTAPV1Firmware;
        break;
    case Bootloader::boardIDASCV1:
        rgFirmware = &_rgASCV1Firmware;
        break;
    case Bootloader::boardIDCrazyflie2:
        rgFirmware = &_rgCrazyflie2Firmware;
        break;
    case Bootloader::boardIDOmnibusF4SD:
        rgFirmware = &_rgOmnibusF4SDFirmware;
        break;
    case Bootloader::boardIDKakuteF7:
        rgFirmware = &_rgKakuteF7Firmware;
        break;
    case Bootloader::boardIDFMUK66V3:
        rgFirmware = &_rgFMUK66V3Firmware;
        break;
    case Bootloader::boardID3DRRadio:
        rgFirmware = &_rg3DRRadioFirmware;
        break;
    default:
        rgFirmware = Q_NULLPTR;
    }

    if (rgFirmware) {
        _rgFirmwareDynamic = *rgFirmware;
    } else {
        _rgFirmwareDynamic.clear();
    }

    // Special case handling for ArduPilot named ChibiOS board
    if (_foundBoardInfo.manufacturer() == QStringLiteral("ArduPilot") || _foundBoardInfo.manufacturer() == QStringLiteral("Hex/ProfiCNC")) {
        // Remove the ChibiOS by board id entries from the list
        for (const FirmwareIdentifier& firmwareId: _rgFirmwareDynamic.keys()) {
            switch (firmwareId.firmwareVehicleType) {
            case CopterChibiOSFirmware:
            case HeliChibiOSFirmware:
            case PlaneChibiOSFirmware:
            case RoverChibiOSFirmware:
            case SubChibiOSFirmware:
                _rgAPMChibiosReplaceNamedBoardFirmware.remove(firmwareId);
                break;
            default:
                break;
            }
        }

        // Add the ChibiOS by board description entries to the list
        for (const FirmwareIdentifier& firmwareId: _rgAPMChibiosReplaceNamedBoardFirmware.keys()) {
            QString namedUrl = _rgAPMChibiosReplaceNamedBoardFirmware[firmwareId];
            _rgFirmwareDynamic.insert(firmwareId, namedUrl.replace(_apmBoardDescriptionReplaceText, _foundBoardInfo.description().left(_foundBoardInfo.description().length() - 3)));
        }
    }

    return &_rgFirmwareDynamic;
}

/// @brief Prompts the user to select a firmware file if needed and moves the state machine to the next state.
void FirmwareUpgradeController::_getFirmwareFile(FirmwareIdentifier firmwareId)
{
    QHash<FirmwareIdentifier, QString>* prgFirmware = _firmwareHashForBoardId(static_cast<int>(_bootloaderBoardID));
    
    if (!prgFirmware && firmwareId.firmwareType != CustomFirmware) {
        _errorCancel(tr("Attempting to flash an unknown board type, you must select 'Custom firmware file'"));
        return;
    }
    
    if (firmwareId.firmwareType == CustomFirmware) {
        _firmwareFilename = QGCQFileDialog::getOpenFileName(nullptr,                                                                // Parent to main window
                                                            tr("Select Firmware File"),                                             // Dialog Caption
                                                            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),    // Initial directory
                                                            tr("Firmware Files (*.px4 *.apj *.bin *.ihx)"));                        // File filter
    } else {
        if (prgFirmware->contains(firmwareId)) {
            _firmwareFilename = prgFirmware->value(firmwareId);
        } else {
            _errorCancel(tr("Unable to find specified firmware download location"));
            return;
        }
    }
    
    if (_firmwareFilename.isEmpty()) {
        _errorCancel(tr("No firmware file selected"));
    } else {
        _downloadFirmware();
    }
}

/// @brief Begins the process of downloading the selected firmware file.
void FirmwareUpgradeController::_downloadFirmware(void)
{
    Q_ASSERT(!_firmwareFilename.isEmpty());
    
    _appendStatusLog(tr("Downloading firmware..."));
    _appendStatusLog(tr(" From: %1").arg(_firmwareFilename));
    
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
        _progressBar->setProperty("value", static_cast<float>(curr) / static_cast<float>(total));
    }
}

/// @brief Called when the firmware download completes.
void FirmwareUpgradeController::_firmwareDownloadFinished(QString remoteFile, QString localFile)
{
    Q_UNUSED(remoteFile);

    _appendStatusLog(tr("Download complete"));
    
    FirmwareImage* image = new FirmwareImage(this);
    
    connect(image, &FirmwareImage::statusMessage, this, &FirmwareUpgradeController::_status);
    connect(image, &FirmwareImage::errorMessage, this, &FirmwareUpgradeController::_error);
    
    if (!image->load(localFile, _bootloaderBoardID)) {
        _errorCancel(tr("Image load failed"));
        return;
    }
    
    // We can't proceed unless we have the bootloader
    if (!_bootloaderFound) {
        _errorCancel(tr("Bootloader not found"));
        return;
    }
    
    if (_bootloaderBoardFlashSize != 0 && image->imageSize() > _bootloaderBoardFlashSize) {
        _errorCancel(tr("Image size of %1 is too large for board flash size %2").arg(image->imageSize()).arg(_bootloaderBoardFlashSize));
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
    _image = nullptr;
    
    _appendStatusLog(tr("Upgrade complete"), true);
    _appendStatusLog("------------------------------------------", false);
    emit flashComplete();
    qgcApp()->toolbox()->linkManager()->setConnectionsAllowed();
}

void FirmwareUpgradeController::_error(const QString& errorString)
{
    delete _image;
    _image = nullptr;
    
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
        _progressBar->setProperty("value", static_cast<float>(curr) / static_cast<float>(total));
    }
}

/// @brief Moves the progress bar ahead on tick while erasing the board
void FirmwareUpgradeController::_eraseProgressTick(void)
{
    _eraseTickCount++;
    _progressBar->setProperty("value", static_cast<float>(_eraseTickCount*_eraseTickMsec) / static_cast<float>(_eraseTotalMsec));
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
    _appendStatusLog(tr("Upgrade cancelled"), true);
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

    QHash<FirmwareIdentifier, QString>* prgFirmware = _firmwareHashForBoardId(static_cast<int>(bootloaderBoardID));

    for (FirmwareIdentifier firmwareId: prgFirmware->keys()) {
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
        sender()->deleteLater();
        return;
    }

    // In order to determine the firmware and vehicle type for this file we find the matching entry in the firmware list

    QHash<FirmwareIdentifier, QString>* prgFirmware = _firmwareHashForBoardId(static_cast<int>(_bootloaderBoardID));

    QString remotePath = QFileInfo(remoteFile).path();
    for (FirmwareIdentifier firmwareId: prgFirmware->keys()) {
        if (remotePath == QFileInfo((*prgFirmware)[firmwareId]).path()) {
            qCDebug(FirmwareUpgradeLog) << "Adding version to map, version:firwmareType:vehicleType" << version << firmwareId.firmwareType << firmwareId.firmwareVehicleType;
            _apmVersionMap[firmwareId.firmwareType][firmwareId.firmwareVehicleType] = version;
        }
    }

    emit apmAvailableVersionsChanged();
    sender()->deleteLater();
}

void FirmwareUpgradeController::setSelectedFirmwareType(FirmwareType_t firmwareType)
{
    _selectedFirmwareType = firmwareType;
    emit selectedFirmwareTypeChanged(_selectedFirmwareType);
    emit apmAvailableVersionsChanged();
}

QStringList FirmwareUpgradeController::apmAvailableVersions(void)
{
    QStringList                     list;
    QList<FirmwareVehicleType_t>    vehicleTypes;

    // This allows us to force the order of the combo box display
    vehicleTypes << CopterChibiOSFirmware << HeliChibiOSFirmware << PlaneChibiOSFirmware << RoverChibiOSFirmware << SubChibiOSFirmware << CopterFirmware << HeliFirmware << PlaneFirmware << RoverFirmware << SubFirmware;

    _apmVehicleTypeFromCurrentVersionList.clear();

    for (FirmwareVehicleType_t vehicleType: vehicleTypes) {
        if (_apmVersionMap[_selectedFirmwareType].contains(vehicleType)) {
            QString version;

            switch (vehicleType) {
            case CopterFirmware:
                version = tr("NuttX - MultiRotor:");
                break;
            case HeliFirmware:
                version = tr("NuttX - Heli:");
                break;
            case CopterChibiOSFirmware:
                version = tr("ChibiOS- MultiRotor:");
                break;
            case HeliChibiOSFirmware:
                version = tr("ChibiOS - Heli:");
                break;
            case PlaneChibiOSFirmware:
            case RoverChibiOSFirmware:
            case SubChibiOSFirmware:
                version = tr("ChibiOS - ");
                break;
            default:
                version = tr("NuttX - ");
                break;
            }

            version += _apmVersionMap[_selectedFirmwareType][vehicleType];
            _apmVehicleTypeFromCurrentVersionList.append(vehicleType);

            list << version;
        }
    }

    return list;
}

FirmwareUpgradeController::FirmwareVehicleType_t FirmwareUpgradeController::vehicleTypeFromVersionIndex(int index)
{
    if (index < 0 || index >= _apmVehicleTypeFromCurrentVersionList.count()) {
        qWarning() << "Invalid index, index:count" << index << _apmVehicleTypeFromCurrentVersionList.count();
        return CopterFirmware;
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
