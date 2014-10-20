/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#include "PX4FirmwareUpgrade.h"

#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFileDialog>

#if 0
static const quint32 crctab[] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static quint32 crc32(const uint8_t *src, unsigned len, unsigned state)
{
    for (unsigned i = 0; i < len; i++) {
        state = crctab[(state ^ src[i]) & 0xff] ^ (state >> 8);
    }
    return state;
}
#endif

// FIXME: QIODevice has error string
const struct PX4FirmwareUpgrade::serialPortErrorString PX4FirmwareUpgrade::_rgSerialPortErrors[14] = {
    { QSerialPort::NoError,                     "No error occurred." },
    { QSerialPort::DeviceNotFoundError,         "An error occurred while attempting to open a non-existing device." },
    { QSerialPort::PermissionError,             "An error occurred while attempting to open an already opened device by another process or a user not having enough permission and credentials to open." },
    { QSerialPort::OpenError,                   "An error occurred while attempting to open an already opened device in this object." },
    { QSerialPort::NotOpenError,                "This error occurs when an operation is executed that can only be successfully performed if the device is open." },
    { QSerialPort::ParityError,                 "Parity error detected by the hardware while reading data." },
    { QSerialPort::FramingError,                "Framing error detected by the hardware while reading data." },
    { QSerialPort::BreakConditionError,         "Break condition detected by the hardware on the input line." },
    { QSerialPort::WriteError,                  "An I/O error occurred while writing the data." },
    { QSerialPort::ReadError,                   "An I/O error occurred while reading the data." },
    { QSerialPort::ResourceError,               "An I/O error occurred when a resource becomes unavailable, e.g. when the device is unexpectedly removed from the system." },
    { QSerialPort::UnsupportedOperationError,   "The requested device operation is not supported or prohibited by the running operating system." },
    { QSerialPort::TimeoutError,                "A timeout error occurred." },
    { QSerialPort::UnknownError,                "An unidentified error occurred." }
};

/// @Brief Constructs a new PX4FirmwareUpgrade Widget. This widget is used within the PX4VehicleConfig set of screens.
PX4FirmwareUpgrade::PX4FirmwareUpgrade(QWidget *parent) :
    QWidget(parent),
    _upgradeState(upgradeStateBegin),
    _downloadManager(NULL),
    _downloadNetworkReply(NULL)
{
    _ui = new Ui::PX4FirmwareUpgrade;
    _ui->setupUi(this);
    
    _threadController = new PX4FirmwareUpgradeThreadController(this);
    Q_CHECK_PTR(_threadController);

    // Connect standard ui elements
    connect(_ui->tryAgain, &QPushButton::clicked, this, &PX4FirmwareUpgrade::_tryAgainButton);
    connect(_ui->cancel, &QPushButton::clicked, this, &PX4FirmwareUpgrade::_cancelButton);
    connect(_ui->next, &QPushButton::clicked, this, &PX4FirmwareUpgrade::_nextButton);
    // FIXME
    //connect(_ui->firmwareCombo, &QComboBox::currentIndexChanged, this, &PX4FirmwareUpgrade::_firmwareSelected);
    connect(_ui->firmwareCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(_firmwareSelected(int)));
    
    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBoard, this, &PX4FirmwareUpgrade::_foundBoard);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBootloader, this, &PX4FirmwareUpgrade::_foundBootloader);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::bootloaderSyncFailed, this, &PX4FirmwareUpgrade::_bootloaderSyncFailed);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::findTimeout, this, &PX4FirmwareUpgrade::_findTimeout);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::rebootComplete, this, &PX4FirmwareUpgrade::_rebootComplete);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::updateProgress, this, &PX4FirmwareUpgrade::_updateProgress);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::upgradeComplete, this, &PX4FirmwareUpgrade::_upgradeComplete);
    
    _setupState(upgradeStateBegin);
}

PX4FirmwareUpgrade::~PX4FirmwareUpgrade()
{
}

const PX4FirmwareUpgrade::stateMachineEntry* PX4FirmwareUpgrade::_getStateMachineEntry(enum PX4FirmwareUpgrade::upgradeStates state)
{
    static const char* msgBegin = "Connect your Pixhawk or PX4Flow board via USB. Then click the 'Next' button to begin the firmware upgrade process.";
    static const char* msgBoardSearch = "Searching for board...";
    static const char* msgBoardNotFound = "Unable to detect your board. If the board is currently connected via USB. Disconnect it, reconnect it and click 'Try Again'.";
    static const char* msgBootloaderSearch = "Bootloader search";
    static const char* msgBootloaderSearchAfterReboot = "Bootloader search after reboot";
    static const char* msgBootloaderNotFound = "Bootloader not found";
    static const char* msgFirmwareSelect = "Please select the firmware you would like to upload to the board from the dropdown to the right.";
    static const char* msgFirmwareSelected = "Click Next to download";
    static const char* msgFirmwareDownloading = "Firmware downloading";
    static const char* msgFirmwareDownloadFailed = "Firmware download failed";
    static const char* msgFirmwareBoardUpgrading = "Board upgrading";
    static const char* msgFirmwareBoardUpgradeFailed = "Board upgrade failed";
    static const char* msgFirmwareBoardUpgraded = "Board upgraded";

    static const stateMachineEntry rgStateMachine[] = {
        { upgradeStateBegin,                        &PX4FirmwareUpgrade::_findBoard,        NULL,                                       NULL,                               msgBegin },
        { upgradeStateBoardSearch,                  NULL,                                   &PX4FirmwareUpgrade::_cancelFind,           NULL,                               msgBoardSearch },
        { upgradeStateBoardNotFound,                NULL,                                   &PX4FirmwareUpgrade::_cancel,               &PX4FirmwareUpgrade::_findBoard,    msgBoardNotFound },
        { upgradeStateBootloaderSearch,             NULL,                                   &PX4FirmwareUpgrade::_cancelFind,           NULL,                               msgBootloaderSearch },
        { upgradeStateBootloaderSearchAfterReboot,  NULL,                                   &PX4FirmwareUpgrade::_cancelFind,           NULL,                               msgBootloaderSearchAfterReboot },
        { upgradeStateBootloaderNotFound,           NULL,                                   &PX4FirmwareUpgrade::_cancel,               NULL,                               msgBootloaderNotFound },
        { upgradeStateFirmwareSelect,               &PX4FirmwareUpgrade::_getFirmwareFile,  &PX4FirmwareUpgrade::_cancel,               NULL,                               msgFirmwareSelect },
        { upgradeStateFirmwareSelected,             &PX4FirmwareUpgrade::_downloadFirmware, &PX4FirmwareUpgrade::_cancelDownload,       NULL,                               msgFirmwareSelected },
        { upgradeStateFirmwareDownloading,          NULL,                                   &PX4FirmwareUpgrade::_cancelDownload,       NULL,                               msgFirmwareDownloading },
        { upgradeStateDownloadFailed,               NULL,                                   &PX4FirmwareUpgrade::_cancel,               NULL,                                    msgFirmwareDownloadFailed },
        { upgradeStateBoardUpgrading,               NULL,                                   NULL,                                       NULL,                                    msgFirmwareBoardUpgrading },
        { upgradeStateBoardUpgradeFailed,           NULL,                                   &PX4FirmwareUpgrade::_cancel,               NULL,                                msgFirmwareBoardUpgradeFailed },
        { upgradeStateBoardUpgraded,                NULL,                                   NULL,                                       NULL,                                     msgFirmwareBoardUpgraded },
    };
    
    const stateMachineEntry* entry = &rgStateMachine[state];
    Q_ASSERT(entry->state == state);
    return entry;
}

void PX4FirmwareUpgrade::_setupState(enum PX4FirmwareUpgrade::upgradeStates state)
{
    _upgradeState = state;
    
    const stateMachineEntry* stateMachine = _getStateMachineEntry(state);
    
    _ui->tryAgain->setEnabled(stateMachine->tryAgain != NULL);
    _ui->skip->setEnabled(false);
    _ui->cancel->setEnabled(stateMachine->cancel != NULL);
    _ui->next->setEnabled(stateMachine->next != NULL);
    
    _ui->statusLog->setText(stateMachine->msg);
    
    _updateIndicatorUI();
}

void PX4FirmwareUpgrade::_updateIndicatorUI(void)
{
    bool boardFound = _upgradeState >= upgradeStateBootloaderSearch;
    _ui->boardFoundCheck->setCheckState(boardFound ? Qt::Checked : Qt::Unchecked);
    _ui->port->setVisible(boardFound);
    _ui->description->setVisible(boardFound);
    if (boardFound) {
        _ui->port->setText("Port: " + _portName);
        _ui->description->setText("Name: " +_portDescription);
    }
    
    bool bootloaderFound = _upgradeState >= upgradeStateFirmwareSelect;
    _ui->bootloaderFoundCheck->setCheckState(bootloaderFound ? Qt::Checked : Qt::Unchecked);
    _ui->bootloaderVersion->setVisible(bootloaderFound);
    _ui->boardID->setVisible(bootloaderFound);
    _ui->icon->setVisible(bootloaderFound);
    if (bootloaderFound) {
        _ui->bootloaderVersion->setText(QString("Version %1").arg(_bootloaderVersion));
        _ui->boardID->setText(QString("Board ID %1").arg(_boardID));
        _setBoardIcon(_boardID);
        _setFirmwareCombo(_boardID);
    }
    
    _ui->firmwareCombo->setVisible(_upgradeState >= upgradeStateFirmwareSelect);
    _ui->firmwareCombo->setEnabled(_upgradeState == upgradeStateFirmwareSelect);
    
    _ui->firmwareDownloadedCheck->setCheckState((_upgradeState >= upgradeStateBoardUpgrading) ? Qt::Checked : Qt::Unchecked);
    
    _ui->boardUpgradedCheck->setCheckState((_upgradeState >= upgradeStateBoardUpgraded) ? Qt::Checked : Qt::Unchecked);
}

void PX4FirmwareUpgrade::_nextButton(void)
{
    const stateMachineEntry* stateMachine = _getStateMachineEntry(_upgradeState);
    
    Q_ASSERT(stateMachine->next != NULL);
    
    (this->*stateMachine->next)();
}


void PX4FirmwareUpgrade::_cancelButton(void)
{
    const stateMachineEntry* stateMachine = _getStateMachineEntry(_upgradeState);
    
    Q_ASSERT(stateMachine->cancel != NULL);
    
    (this->*stateMachine->cancel)();
}

void PX4FirmwareUpgrade::_tryAgainButton(void)
{
    const stateMachineEntry* stateMachine = _getStateMachineEntry(_upgradeState);
    
    Q_ASSERT(stateMachine->tryAgain != NULL);
    
    (this->*stateMachine->tryAgain)();
}

void PX4FirmwareUpgrade::_cancelFind(void)
{
    _threadController->cancelFind();
    _cancel();
}

void PX4FirmwareUpgrade::_cancel(void)
{
    _setupState(upgradeStateBegin);
}

void PX4FirmwareUpgrade::_findBoard(void)
{
    _setupState(upgradeStateBoardSearch);
    _threadController->findBoard(10000);
}

void PX4FirmwareUpgrade::_foundBoard(const QString portName, QString portDescription)
{
    qDebug() << "Found board";
    _portName = portName;
    _portDescription = portDescription;
    _setupState(upgradeStateBootloaderSearch);
    _findBootloader();
}

void PX4FirmwareUpgrade::_findBootloader(void)
{
    _setupState(upgradeStateBootloaderSearch);
    _threadController->findBootloader(_portName, 5000);
}

void PX4FirmwareUpgrade::_foundBootloader(int bootloaderVersion, int boardID, int flashSize)
{
    qDebug() << "Found bootloader";
    _bootloaderVersion = bootloaderVersion;
    _boardID = boardID;
    _boardFlashSize = flashSize;
    _setupState(upgradeStateFirmwareSelect);
}

void PX4FirmwareUpgrade::_bootloaderSyncFailed(void)
{
    if (_upgradeState == upgradeStateBootloaderSearch) {
        // We can connect to the board, but we can't get the bootloader. Likely means that board is currently running FMU
        // firware. Reboot the board to so we can catch the bootloader on the way by.
        qDebug() << "Bootloader sync failed, trying reboot";
        _setupState(upgradeStateBootloaderSearchAfterReboot);
        _threadController->sendFMUReboot(_portName);
    } else if (_upgradeState == upgradeStateBootloaderSearchAfterReboot) {
        // We can connect to the board, but we still can't get the bootloader even after a reboot. This is fatal.
        qDebug() << "Bootloader sync failed after reboot";
        _setupState(upgradeStateBootloaderNotFound);
    } else {
        Q_ASSERT(false);
    }
    
}

void PX4FirmwareUpgrade::_findTimeout(void)
{
    if (_upgradeState == upgradeStateBoardSearch) {
        qDebug() << "Timeout on board search";
        _setupState(upgradeStateBoardNotFound);
    } else if (_upgradeState == upgradeStateBootloaderSearch) {
        qDebug() << "Timeout on bootloader search";
        _setupState(upgradeStateBoardNotFound);
    } else if (_upgradeState == upgradeStateBootloaderSearchAfterReboot) {
        qDebug() << "Timeout on bootloader search after reboot";
        _setupState(upgradeStateBoardNotFound);
    } else {
        Q_ASSERT(false);
    }
}

const char* PX4FirmwareUpgrade::_serialPortErrorString(int error)
{
Again:
    for (size_t i=0; i<sizeof(_rgSerialPortErrors)/sizeof(_rgSerialPortErrors[0]); i++) {
        if (error == _rgSerialPortErrors[i].error) {
            return _rgSerialPortErrors[i].errorString;
        }
    }
    
    error = QSerialPort::UnknownError;
    goto Again;
    
    Q_ASSERT(false);
    
    return NULL;
}

/// @brief Sets the board image into the icon label according to the board id.
void PX4FirmwareUpgrade::_setBoardIcon(int boardID)
{
    QString imageFile;
    
    switch (boardID) {
        case _boardIDPX4FMUV1:
            imageFile = ":/files/images/px4/boards/px4fmu_1.x.png";
            break;
            
        case _boardIDPX4Flow:
            imageFile = ":/files/images/px4/boards/px4flow_1.x.png";
            break;
            
        case _boardIDPX4FMUV2:
            imageFile = ":/files/images/px4/boards/px4fmu_2.x.png";
            break;
            
        default:
            qDebug() << "Invalid board ID";
            Q_ASSERT(false);
            break;
    }
    
    bool success = _boardIcon.load(imageFile);
    Q_ASSERT(success);
    
    int w = _ui->icon->width();
    int h = _ui->icon->height();
    
    _ui->icon->setPixmap(_boardIcon.scaled(w, h, Qt::KeepAspectRatio));
}

/// @brief Sets up the selections in the firmware combox box associated with the specified
///     board id.
void PX4FirmwareUpgrade::_setFirmwareCombo(int boardID)
{
    _ui->firmwareCombo->clear();
    
    static const char* rgPX4FMUV1Firmware[3] =
    {
        "http://px4.oznet.ch/stable/px4fmu-v1_default.px4",
        "http://px4.oznet.ch/beta/px4fmu-v1_default.px4",
        "http://px4.oznet.ch/continuous/px4fmu-v1_default.px4"
    };
    
    static const char* rgPX4FMUV2Firmware[3] =
    {
        "http://px4.oznet.ch/stable/px4fmu-v2_default.px4",
        "http://px4.oznet.ch/beta/px4fmu-v2_default.px4",
        "http://px4.oznet.ch/continuous/px4fmu-v2_default.px4"
    };
    
    static const char* rgPX4FlowFirmware[3] =
    {
        "http://px4.oznet.ch/stable/px4flow.px4",
        "http://px4.oznet.ch/beta/px4flow.px4",
        "http://px4.oznet.ch/continuous/px4flow.px4"
    };
    
    const char** prgFirmware;
    switch (boardID) {
        case _boardIDPX4FMUV1:
            prgFirmware = rgPX4FMUV1Firmware;
            break;

        case _boardIDPX4Flow:
            prgFirmware = rgPX4FlowFirmware;
            break;

        case _boardIDPX4FMUV2:
            prgFirmware = rgPX4FMUV2Firmware;
            break;
            
        default:
            qDebug() << "Invalid board ID";
            Q_ASSERT(false);
            break;
    }
    
    _ui->firmwareCombo->addItem(tr("Please select firmware to load"));
    _ui->firmwareCombo->addItem(tr("Standard Version (stable)"), prgFirmware[0]);
    _ui->firmwareCombo->addItem(tr("Beta Testing (beta)"), prgFirmware[1]);
    _ui->firmwareCombo->addItem(tr("Developer Build (master)"), prgFirmware[2]);
    _ui->firmwareCombo->addItem(tr("Custom firmware file..."), "selectfile");
}

void PX4FirmwareUpgrade::_firmwareSelected(int index)
{
#define SELECT_FIRMWARE_PREFIX "Please select the firmware you would like to upload to the board from the dropdown to the right.\n\n"
#define SELECT_FIRMWARE_LICENSE "By clicking Next you agree to the terms and disclaimer of the BSD open source license, as redistributed with the source code."
    
    if (_upgradeState == upgradeStateFirmwareSelect) {
        switch (index) {
            case 0:
                _ui->statusLog->setText(tr(SELECT_FIRMWARE_PREFIX));
                break;
                
            case 1:
            case 4:
                _ui->statusLog->setText(tr(SELECT_FIRMWARE_PREFIX SELECT_FIRMWARE_LICENSE));
                break;
                
            case 2:
                _ui->statusLog->setText(tr(SELECT_FIRMWARE_PREFIX
                                          "WARNING: BETA FIRMWARE\n"
                                          "This firmware version is ONLY intended for beta testers. "
                                          "Although it has received FLIGHT TESTING, it represents actively changed code. Do NOT use for normal operation.\n\n"
                                          SELECT_FIRMWARE_LICENSE));
                break;
                
            case 3:
                _ui->statusLog->setText(tr(SELECT_FIRMWARE_PREFIX
                                          "WARNING: CONTINUOUS BUILD FIRMWARE\n"
                                          "This firmware has NOT BEEN FLIGHT TESTED. "
                                          "It is only intended for DEVELOPERS. Run bench tests without props first. "
                                          "Do NOT fly this without addional safety precautions. Follow the mailing "
                                          "list actively when using it.\n\n"
                                          SELECT_FIRMWARE_LICENSE));
                break;
        }
        _ui->next->setEnabled(!_ui->firmwareCombo->itemData(index).toString().isEmpty());
    }
}

void PX4FirmwareUpgrade::_getFirmwareFile(void)
{
    int index = _ui->firmwareCombo->currentIndex();
    Q_ASSERT(index >= 0 && index <= 4);
    _firmwareFilename = _ui->firmwareCombo->itemData(index).toString();
    Q_ASSERT(!_firmwareFilename.isEmpty());
    if (_firmwareFilename == "selectfile") {
        _firmwareFilename = QFileDialog::getOpenFileName(this,
                                                    tr("Select Firmware File"),                                             // Dialog title
                                                     QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),   // Initial directory
                                                     tr("Firmware Files (*.px4 *.bin)"));                                   // File filter

    }
    if (!_firmwareFilename.isEmpty()) {
        _setupState(upgradeStateFirmwareSelected);
    }
}

#if 0
void PX4FirmwareUpgrade::_cancelUpgrade(void)
{
    // Cancel any in progress download
    if (_downloadManager) {
        if (_downloadNetworkReply) {
            _downloadNetworkReply->abort();
        }
        _downloadManager->deleteLater();
        _downloadManager = NULL;
    }
    
    // If the bootloader is open, close it
    _bootloaderPort.close();
    
    _upgradeBegin();
}

#endif

/// @brief Begins the process of downloading the firmware file.
/// @return true: Download process has begin, false: Could not start download process
void PX4FirmwareUpgrade::_downloadFirmware(void)
{
    // Split out filename from path
    Q_ASSERT(!_firmwareFilename.isEmpty());
    QString firmwareFilename = QFileInfo(_firmwareFilename).fileName();
    Q_ASSERT(!firmwareFilename.isEmpty());
    
    // Determine location to download file to
    QString downloadFile = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (downloadFile.isEmpty()) {
        downloadFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        if (downloadFile.isEmpty()) {
            _ui->statusLabel->setText(tr("Unabled to find writable download location. Tried downloads and temp directory."));
            return;
        }
    }
    Q_ASSERT(!downloadFile.isEmpty());
    downloadFile += "/" + firmwareFilename;

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
    connect(_downloadNetworkReply, &QNetworkReply::downloadProgress, this, &PX4FirmwareUpgrade::_downloadProgress);
    connect(_downloadNetworkReply, &QNetworkReply::finished, this, &PX4FirmwareUpgrade::_downloadFinished);
    // FIXME
    //connect(_downloadNetworkReply, &QNetworkReply::error, this, &PX4FirmwareUpgrade::_downloadError);
    connect(_downloadNetworkReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(_downloadError(QNetworkReply::NetworkError)));
    
    _setupState(upgradeStateFirmwareDownloading);
}

void PX4FirmwareUpgrade::_cancelDownload(void)
{
    
}

void PX4FirmwareUpgrade::_downloadProgress(qint64 curr, qint64 total)
{
    // Take care of cases where 0 / 0 is emitted as error return value
    if (total > 0) {
        _ui->progressBar->setValue((curr*100) / total);
    }
}

/// @brief Called when the firmware download completes.
void PX4FirmwareUpgrade::_downloadFinished(void)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    Q_ASSERT(reply);
    
    // FIXME: Need better way to do all this error handling duplication

    // Download file location is in user attribute
    QString downloadFilename = reply->request().attribute(QNetworkRequest::User).toString();
    Q_ASSERT(!downloadFilename.isEmpty());
    
    // Store file in download location
    QFile file(downloadFilename);
    if (!file.open(QIODevice::WriteOnly)) {
        _ui->statusLabel->setText(tr("Could not save downloaded file to %1. Error: %2").arg(downloadFilename).arg(file.errorString()));
        _setupState(upgradeStateDownloadFailed);
        return;
    }
    
    file.write(reply->readAll());
    file.close();
    
    _downloadManager->deleteLater();
    _downloadManager = NULL;
    
    if (downloadFilename.endsWith(".px4")) {
        // We need to collect information from the .px4 file as well as pull the binary image out to a file.
        
        QFile px4File(downloadFilename);
        if (!px4File.open(QIODevice::ReadOnly | QIODevice::Text)) {
            _ui->statusLabel->setText(tr("Unable to open firmware file %1, error: %2").arg(downloadFilename).arg(px4File.errorString()));
            _setupState(upgradeStateDownloadFailed);
            return;
        }
        
        QByteArray bytes = px4File.readAll();
        px4File.close();
        QJsonDocument doc = QJsonDocument::fromJson(bytes);
        
        if (doc.isNull()) {
            // FIXME: Better error message
            _ui->statusLabel->setText(tr("supplied file is not a valid JSON document"));
            _setupState(upgradeStateDownloadFailed);
            return;
        }
        
        QJsonObject px4Json = doc.object();
        
        // FIXME: Magic strings
        static const char* rgJsonKeys[] = { "board_id", "image_size", "description", "git_identity" };
        for (size_t i=0; i<sizeof(rgJsonKeys)/sizeof(rgJsonKeys[0]); i++) {
            if (!px4Json.contains(rgJsonKeys[i])) {
                _ui->statusLabel->setText(tr("Incorrectly formatted firmware file. No %1 key.").arg(rgJsonKeys[i]));
                _setupState(upgradeStateDownloadFailed);
                return;
            }
        }
        
        uint32_t firmwareBoardID = (uint32_t)px4Json.value(QString("board_id")).toInt();
        if (firmwareBoardID != _boardID) {
            _ui->statusLabel->setText(tr("Downloaded firmware board id does not match hardware board id: %1 != %2").arg(firmwareBoardID).arg(_boardID));
            _setupState(upgradeStateDownloadFailed);
            return;
        }
        
        _imageSize = px4Json.value(QString("image_size")).toInt();
        if (_imageSize == 0) {
            _ui->statusLabel->setText(tr("Image size of 0 in .px4 file %1").arg(downloadFilename));
            _setupState(upgradeStateDownloadFailed);
            return;
        }
        qDebug() << "Image size from px4:" << _imageSize;
        
        // Convert image from base-64 and decompress
        
        // XXX Qt's JSON string handling is terribly broken, strings
        // with some length (18K / 25K) are just weirdly cut.
        // The code below works around this by manually 'parsing'
        // for the image string. Since its compressed / checksummed
        // this should be fine.
        
        QStringList list = QString(bytes).split("\"image\": \"");
        list = list.last().split("\"");
        
        // Convert String to QByteArray and unzip it
        QByteArray raw;

        // Store image size
        raw.append((unsigned char)((_imageSize >> 24) & 0xFF));
        raw.append((unsigned char)((_imageSize >> 16) & 0xFF));
        raw.append((unsigned char)((_imageSize >> 8) & 0xFF));
        raw.append((unsigned char)((_imageSize >> 0) & 0xFF));
        
        QByteArray raw64 = list.first().toUtf8();
        
        raw.append(QByteArray::fromBase64(raw64));
        QByteArray uncompressed = qUncompress(raw);
        
        QByteArray b = uncompressed;

        if (b.count() == 0) {
            _ui->statusLabel->setText(tr("Firmware file has 0 length image"));
            _setupState(upgradeStateDownloadFailed);
            return;
        }
        if (b.count() != (int)_imageSize) {
            _ui->statusLabel->setText(tr("Image size for decompressed image does not match stored image size: Expected(%1) Actual(%2)").arg(_imageSize).arg(b.count()));
            _setupState(upgradeStateDownloadFailed);
            return;
        }
        
        // pad image to 4-byte boundary
        while ((b.count() % 4) != 0) {
            b.append(static_cast<char>(static_cast<unsigned char>(0xFF)));
        }
        
        // Store decompressed image file in same location as original download file
        QDir downloadDir = QFileInfo(downloadFilename).dir();
        QString decompressFilename = downloadDir.filePath("PX4FlashUpgrade.bin");
        
        QFile decompressFile(decompressFilename);
        if (!decompressFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            _ui->statusLabel->setText(tr("Unable to open decompressed file %1 for writing, error: %2").arg(decompressFilename).arg(decompressFile.errorString()));
            _setupState(upgradeStateDownloadFailed);
            return;
        }
        
        qint64 bytesWritten = decompressFile.write(b);
        if (bytesWritten != b.count()) {
            _ui->statusLabel->setText(tr("Write failed for decompressed image file, error: %1").arg(decompressFile.errorString()));
            _setupState(upgradeStateDownloadFailed);
            return;
        }
        decompressFile.close();
        
        _firmwareFilename = decompressFilename;
    } else if (downloadFilename.endsWith(".bin")) {
        uint32_t firmwareBoardID = 0;
        
        // Take some educated guesses on board id based on firmware build system file name conventions
        
        if (downloadFilename.toLower().contains("px4fmu-v1")) {
            firmwareBoardID = _boardIDPX4FMUV2;
        } else if (downloadFilename.toLower().contains("px4flow")) {
            firmwareBoardID = _boardIDPX4Flow;
        } else if (downloadFilename.toLower().contains("px4fmu-v1")) {
            firmwareBoardID = _boardIDPX4FMUV1;
        }
        
        if (firmwareBoardID != 0 &&  firmwareBoardID != _boardID) {
            _ui->statusLabel->setText(tr("Downloaded firmware board id does not match hardware board id: %1 != %2").arg(firmwareBoardID).arg(_boardID));
            _setupState(upgradeStateDownloadFailed);
            return;
        }
        
        _firmwareFilename = downloadFilename;
        
        QFile binFile(_firmwareFilename);
        if (!binFile.open(QIODevice::ReadOnly)) {
            _ui->statusLabel->setText(tr("Unabled to open firmware file %1, %2").arg(_firmwareFilename).arg(binFile.errorString()));
            _setupState(upgradeStateDownloadFailed);
        }
        _imageSize = (uint32_t)binFile.size();
        binFile.close();
    } else {
        // Standard firmware builds (stable/continuous/...) are always .bin or .px4. Select file dialog for custom
        // firmware filters to .bin and .px4. So we should ever get a file that ends in anything else.
        Q_ASSERT(false);
    }
    
    if (_imageSize > _boardFlashSize) {
        _ui->statusLabel->setText(tr("Image size of %1 is too large for board flash size %2").arg(_imageSize).arg(_boardFlashSize));
        _setupState(upgradeStateDownloadFailed);
        return;
    }

    //_program();
}

void PX4FirmwareUpgrade::_downloadError(QNetworkReply::NetworkError code)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    Q_ASSERT(reply);
    
    reply->abort();
    _downloadManager->deleteLater();
    _downloadManager = NULL;
    
    // FIXME: Text for error code
    _ui->statusLabel->setText(tr("Error during download. Error: %1").arg(code));
    
    _setupState(upgradeStateDownloadFailed);
}

void PX4FirmwareUpgrade::_program(void)
{
    _setupState(upgradeStateBoardUpgrading);
    _threadController->program(_firmwareFilename);
}

void PX4FirmwareUpgrade::_rebootComplete(void)
{
    _threadController->findBootloader(_portName, 10000);
}

void PX4FirmwareUpgrade::_updateProgress(int curr, int total)
{
    _ui->progressBar->setValue((curr*100) / total);
}

void PX4FirmwareUpgrade::_upgradeComplete(void)
{
    _setupState(upgradeStateBoardUpgraded);
}
