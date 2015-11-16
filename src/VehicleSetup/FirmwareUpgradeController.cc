/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @brief PX4 Firmware Upgrade UI
///     @author Don Gagne <don@thegagnes.com>

#include "FirmwareUpgradeController.h"
#include "Bootloader.h"
#include "QGCFileDialog.h"
#include "QGCMessageBox.h"


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
FirmwareUpgradeController::FirmwareUpgradeController(void) :
    _downloadManager(NULL),
    _downloadNetworkReply(NULL),
    _statusLog(NULL),
    _image(NULL)
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
    
    connect(qgcApp()->toolbox()->linkManager(), &LinkManager::linkDisconnected, this, &FirmwareUpgradeController::_linkDisconnected);

    connect(&_eraseTimer, &QTimer::timeout, this, &FirmwareUpgradeController::_eraseProgressTick);
}

void FirmwareUpgradeController::startBoardSearch(void)
{
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

void FirmwareUpgradeController::_foundBoard(bool firstAttempt, const QSerialPortInfo& info, int type)
{
    _foundBoardInfo = info;
    switch (type) {
        case QGCSerialPortInfo::BoardTypePX4FMUV1:
            _foundBoardType = "PX4 FMU V1";
            _startFlashWhenBootloaderFound = false;
            break;
        case QGCSerialPortInfo::BoardTypePX4FMUV2:
            _foundBoardType = "Pixhawk";
            _startFlashWhenBootloaderFound = false;
            break;
        case QGCSerialPortInfo::BoardTypeAeroCore:
            _foundBoardType = "AeroCore";
            _startFlashWhenBootloaderFound = false;
            break;
        case QGCSerialPortInfo::BoardTypePX4Flow:
            _foundBoardType = "PX4 Flow";
            _startFlashWhenBootloaderFound = false;
            break;
        case QGCSerialPortInfo::BoardType3drRadio:
            _foundBoardType = "3DR Radio";
            if (!firstAttempt) {
                // Radio always flashes latest firmware, so we can start right away without
                // any further user input.
                _startFlashWhenBootloaderFound = true;
                _startFlashWhenBootloaderFoundFirmwareIdentity = FirmwareIdentifier(ThreeDRRadio,
                                                                                    StableFirmware,
                                                                                    DefaultVehicleFirmware);
            }
            break;
    }
    
    qCDebug(FirmwareUpgradeLog) << _foundBoardType;
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

    //////////////////////////////////// PX4FMUV2 firmwares //////////////////////////////////////////////////
    FirmwareToUrlElement_t rgPX4FMV2FirmwareArray[] = {
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/stable/px4fmu-v2_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/px4fmu-v2_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/px4fmu-v2_default.px4"},
        { AutoPilotStackAPM, StableFirmware,    QuadFirmware,           "http://firmware.diydrones.com/Copter/stable/PX4-quad/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    X8Firmware,             "http://firmware.diydrones.com/Copter/stable/PX4-octa-quad/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    HexaFirmware,           "http://firmware.diydrones.com/Copter/stable/PX4-hexa/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    OctoFirmware,           "http://firmware.diydrones.com/Copter/stable/PX4-octa/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    YFirmware,              "http://firmware.diydrones.com/Copter/stable/PX4-tri/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    Y6Firmware,             "http://firmware.diydrones.com/Copter/stable/PX4-y6/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    HeliFirmware,           "http://firmware.diydrones.com/Copter/stable/PX4-heli/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    PlaneFirmware,          "http://firmware.diydrones.com/Plane/stable/PX4/ArduPlane-v2.px4"},
        { AutoPilotStackAPM, StableFirmware,    RoverFirmware,          "http://firmware.diydrones.com/Rover/stable/PX4/APMrover2-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      QuadFirmware,           "http://firmware.diydrones.com/Copter/beta/PX4-quad/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      X8Firmware,             "http://firmware.diydrones.com/Copter/beta/PX4-octa-quad/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      HexaFirmware,           "http://firmware.diydrones.com/Copter/beta/PX4-hexa/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      OctoFirmware,           "http://firmware.diydrones.com/Copter/beta/PX4-octa/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      YFirmware,              "http://firmware.diydrones.com/Copter/beta/PX4-tri/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      Y6Firmware,             "http://firmware.diydrones.com/Copter/beta/PX4-y6/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      HeliFirmware,           "http://firmware.diydrones.com/Copter/beta/PX4-heli/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      PlaneFirmware,          "http://firmware.diydrones.com/Plane/beta/PX4/ArduPlane-v2.px4"},
        { AutoPilotStackAPM, BetaFirmware,      RoverFirmware,          "http://firmware.diydrones.com/Rover/beta/PX4/APMrover2-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, QuadFirmware,           "http://firmware.diydrones.com/Copter/latest/PX4-quad/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, X8Firmware,             "http://firmware.diydrones.com/Copter/latest/PX4-octa-quad/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, HexaFirmware,           "http://firmware.diydrones.com/Copter/latest/PX4-hexa/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, OctoFirmware,           "http://firmware.diydrones.com/Copter/latest/PX4-octa/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, YFirmware,              "http://firmware.diydrones.com/Copter/latest/PX4-tri/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, Y6Firmware,             "http://firmware.diydrones.com/Copter/latest/PX4-y6/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, HeliFirmware,           "http://firmware.diydrones.com/Copter/latest/PX4-heli/ArduCopter-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, PlaneFirmware,          "http://firmware.diydrones.com/Plane/latest/PX4/ArduPlane-v2.px4"},
        { AutoPilotStackAPM, DeveloperFirmware, RoverFirmware,          "http://firmware.diydrones.com/Rover/latest/PX4/APMrover2-v2.px4"}
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
        { AutoPilotStackPX4, StableFirmware,    DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/latest/px4fmu-v1_default.px4"},
        { AutoPilotStackPX4, BetaFirmware,      DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/beta/px4fmu-v1_default.px4"},
        { AutoPilotStackPX4, DeveloperFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/px4fmu-v1_default.px4"},
    };

    /////////////////////////////// px4flow firmwares ///////////////////////////////////////
    FirmwareToUrlElement_t rgPX4FLowFirmwareArray[] = {
        { PX4Flow, StableFirmware, DefaultVehicleFirmware, "http://px4-travis.s3.amazonaws.com/Flow/master/px4flow.px4" },
    };

    /////////////////////////////// 3dr radio firmwares ///////////////////////////////////////
    FirmwareToUrlElement_t rg3DRRadioFirmwareArray[] = {
        { ThreeDRRadio, StableFirmware, DefaultVehicleFirmware, "http://firmware.diydrones.com/SiK/stable/radio~hm_trp.ihx"}
    };

    // populate hashes now
    int size = sizeof(rgPX4FMV2FirmwareArray)/sizeof(rgPX4FMV2FirmwareArray[0]);
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

/// @brief Prompts the user to select a firmware file if needed and moves the state machine to the next state.
void FirmwareUpgradeController::_getFirmwareFile(FirmwareIdentifier firmwareId)
{
    // make sure the firmware hashes are populated
    _initFirmwareHash();

    // Select the firmware set based on board type
    
    QHash<FirmwareIdentifier, QString> prgFirmware;
    
    switch (_bootloaderBoardID) {
        case Bootloader::boardIDPX4FMUV1:
            prgFirmware = _rgPX4FMUV1Firmware;
            break;
            
        case Bootloader::boardIDPX4Flow:
            prgFirmware = _rgPX4FLowFirmware;
            break;
            
        case Bootloader::boardIDPX4FMUV2:
            prgFirmware = _rgPX4FMUV2Firmware;
            break;
            
        case Bootloader::boardIDAeroCore:
            prgFirmware = _rgAeroCoreFirmware;
            break;
            
        case Bootloader::boardID3DRRadio:
            prgFirmware = _rg3DRRadioFirmware;
            break;
            
        default:
            break;
    }
    
    if (prgFirmware.isEmpty() && firmwareId.firmwareType != CustomFirmware) {
        _errorCancel("Attempting to flash an unknown board type, you must select 'Custom firmware file'");
        return;
    }
    
    if (firmwareId.firmwareType == CustomFirmware) {
        _firmwareFilename = QGCFileDialog::getOpenFileName(NULL,                                                                // Parent to main window
                                                           "Select Firmware File",                                              // Dialog Caption
                                                           QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), // Initial directory
                                                           "Firmware Files (*.px4 *.bin *.ihx)");                               // File filter
    } else {

        if (prgFirmware.contains(firmwareId)) {
            _firmwareFilename = prgFirmware.value(firmwareId);
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
    
    // Split out filename from path
    QString firmwareFilename = QFileInfo(_firmwareFilename).fileName();
    Q_ASSERT(!firmwareFilename.isEmpty());
    
    // Determine location to download file to
    QString downloadFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (downloadFile.isEmpty()) {
        downloadFile = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (downloadFile.isEmpty()) {
            _errorCancel("Unabled to find writable download location. Tried downloads and temp directory.");
            return;
        }
    }
    Q_ASSERT(!downloadFile.isEmpty());
    downloadFile += "/"  + firmwareFilename;

    QUrl firmwareUrl;
    if (_firmwareFilename.startsWith("http:")) {
        firmwareUrl.setUrl(_firmwareFilename);
    } else {
        firmwareUrl = QUrl::fromLocalFile(_firmwareFilename);
    }
    Q_ASSERT(firmwareUrl.isValid());
    
    QNetworkRequest networkRequest(firmwareUrl);
    
    // Store download file location in user attribute so we can retrieve when the download finishes
    networkRequest.setAttribute(QNetworkRequest::User, downloadFile);
    
    _downloadManager = new QNetworkAccessManager(this);
    Q_CHECK_PTR(_downloadManager);
    _downloadNetworkReply = _downloadManager->get(networkRequest);
    Q_ASSERT(_downloadNetworkReply);
    connect(_downloadNetworkReply, &QNetworkReply::downloadProgress, this, &FirmwareUpgradeController::_downloadProgress);
    connect(_downloadNetworkReply, &QNetworkReply::finished, this, &FirmwareUpgradeController::_downloadFinished);
    // FIXME
    //connect(_downloadNetworkReply, &QNetworkReply::error, this, &FirmwareUpgradeController::_downloadError);
    connect(_downloadNetworkReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(_downloadError(QNetworkReply::NetworkError)));
}

/// @brief Updates the progress indicator while downloading
void FirmwareUpgradeController::_downloadProgress(qint64 curr, qint64 total)
{
    // Take care of cases where 0 / 0 is emitted as error return value
    if (total > 0) {
        _progressBar->setProperty("value", (float)curr / (float)total);
    }
}

/// @brief Called when the firmware download completes.
void FirmwareUpgradeController::_downloadFinished(void)
{
    _appendStatusLog("Download complete");
    
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    Q_ASSERT(reply);
    
    Q_ASSERT(_downloadNetworkReply == reply);
    
    _downloadManager->deleteLater();
    _downloadManager = NULL;
    
    // When an error occurs or the user cancels the download, we still end up here. So bail out in
    // those cases.
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }
    
    // Download file location is in user attribute
    QString downloadFilename = reply->request().attribute(QNetworkRequest::User).toString();
    Q_ASSERT(!downloadFilename.isEmpty());
    
    // Store downloaded file in download location
    QFile file(downloadFilename);
    if (!file.open(QIODevice::WriteOnly)) {
        _errorCancel(QString("Could not save downloaded file to %1. Error: %2").arg(downloadFilename).arg(file.errorString()));
        return;
    }
    
    file.write(reply->readAll());
    file.close();
    FirmwareImage* image = new FirmwareImage(this);
    
    connect(image, &FirmwareImage::statusMessage, this, &FirmwareUpgradeController::_status);
    connect(image, &FirmwareImage::errorMessage, this, &FirmwareUpgradeController::_error);
    
    if (!image->load(downloadFilename, _bootloaderBoardID)) {
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
void FirmwareUpgradeController::_downloadError(QNetworkReply::NetworkError code)
{
    QString errorMsg;
    
    if (code == QNetworkReply::OperationCanceledError) {
        errorMsg = "Download cancelled";
    } else {
        errorMsg = QString("Error during download. Error: %1").arg(code);
    }
    _errorCancel(errorMsg);
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

bool FirmwareUpgradeController::qgcConnections(void)
{
    return qgcApp()->toolbox()->linkManager()->anyActiveLinks();
}

void FirmwareUpgradeController::_linkDisconnected(LinkInterface* link)
{
    Q_UNUSED(link);
    emit qgcConnectionsChanged(qgcConnections());
}

void FirmwareUpgradeController::_errorCancel(const QString& msg)
{
    _appendStatusLog(msg, false);
    _appendStatusLog("Upgrade cancelled", true);
    _appendStatusLog("------------------------------------------", false);
    emit error();
    cancel();
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
