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

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QtCore>
#include <QDebug>
#include <QFileDialog>
#include <QThread>
#include <QNetworkRequest>
#include <QNetworkReply>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif
#include <QFileInfo>
#include <QApplication>
#include <QDesktopWidget>
#include <QTimer>
#include <QWebHistory>
#include <QWebSettings>
#include <QMessageBox>
#include <QSettings>

#include <QGC.h>

#include "PX4FirmwareUpgrade.h"

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
    _ui.setupUi(this);

#if 0
    // FIXME
    struct BoardInfo {
        const char* name;
        int         id;
    };
    static const BoardInfo rgBoardInfo[] = {
        { "PX4FMU v1.6+",   5 },
        { "PX4FLOW v1.1+",  6 },
        { "PX4IO v1.3+",    7 },
        { "PX4 board #8",   8 },
        { "PX4 board #9",   9 },
        { "PX4 board #10",  10 },
        { "PX4 board #11",  11 },
    };
#endif
    
    // Connect standard ui elements
    connect(_ui.tryAgain, &QPushButton::clicked, this, &PX4FirmwareUpgrade::_tryAgain);
    connect(_ui.cancel, &QPushButton::clicked, this, &PX4FirmwareUpgrade::_cancelUpgrade);
    connect(_ui.next, &QPushButton::clicked, this, &PX4FirmwareUpgrade::_nextStep);
    // FIXME
    //connect(_ui.firmwareCombo, &QComboBox::currentIndexChanged, this, &PX4FirmwareUpgrade::_firmwareSelected);
    connect(_ui.firmwareCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(_firmwareSelected(int)));
    
    _upgradeBegin();
}

// FIXME: What about destructor for app close during upgrade process

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
    
    int w = _ui.icon->width();
    int h = _ui.icon->height();
    
    _ui.icon->setPixmap(_boardIcon.scaled(w, h, Qt::KeepAspectRatio));
}

/// @brief Sets up the selections in the firmware combox box associated with the specified
///     board id.
void PX4FirmwareUpgrade::_setFirmwareCombo(int boardID)
{
    _ui.firmwareCombo->clear();
    
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
    
    _ui.firmwareCombo->addItem(tr("Please select firmware to load"));
    _ui.firmwareCombo->addItem(tr("Standard Version (stable)"), prgFirmware[0]);
    _ui.firmwareCombo->addItem(tr("Beta Testing (beta)"), prgFirmware[1]);
    _ui.firmwareCombo->addItem(tr("Developer Build (master)"), prgFirmware[2]);
    _ui.firmwareCombo->addItem(tr("Custom firmware file..."), "selectfile");
}

void PX4FirmwareUpgrade::_firmwareSelected(int index)
{
#define SELECT_FIRMWARE_PREFIX "Please select the firmware you would like to upload to the board from the dropdown to the right.\n\n"
#define SELECT_FIRMWARE_LICENSE "By clicking Next you agree to the terms and disclaimer of the BSD open source license, as redistributed with the source code."
    
    if (_upgradeState == upgradeStateBootloaderFound) {
        switch (index) {
            case 0:
                _ui.statusLog->setText(tr(SELECT_FIRMWARE_PREFIX));
                break;
                
            case 1:
            case 4:
                _ui.statusLog->setText(tr(SELECT_FIRMWARE_PREFIX SELECT_FIRMWARE_LICENSE));
                break;
                
            case 2:
                _ui.statusLog->setText(tr(SELECT_FIRMWARE_PREFIX
                                          "WARNING: BETA FIRMWARE\n"
                                          "This firmware version is ONLY intended for beta testers. "
                                          "Although it has received FLIGHT TESTING, it represents actively changed code. Do NOT use for normal operation.\n\n"
                                          SELECT_FIRMWARE_LICENSE));
                break;
                
            case 3:
                _ui.statusLog->setText(tr(SELECT_FIRMWARE_PREFIX
                                          "WARNING: CONTINUOUS BUILD FIRMWARE\n"
                                          "This firmware has NOT BEEN FLIGHT TESTED. "
                                          "It is only intended for DEVELOPERS. Run bench tests without props first. "
                                          "Do NOT fly this without addional safety precautions. Follow the mailing "
                                          "list actively when using it.\n\n"
                                          SELECT_FIRMWARE_LICENSE));
                break;
        }
        _ui.next->setEnabled(!_ui.firmwareCombo->itemData(index).toString().isEmpty());
    }
}


void PX4FirmwareUpgrade::_nextStep(void)
{
    int index;
    
    switch (_upgradeState) {
        case upgradeStateBegin:
            if (_findBoard()) {
                _upgradeBoardFound();
            } else {
                _upgradeBoardNotFound();
            }
            break;
            
        case upgradeStateBootloaderFound:
            index = _ui.firmwareCombo->currentIndex();
            Q_ASSERT(index >= 0 && index <= 4);
            _firmwareFilename = _ui.firmwareCombo->itemData(index).toString();
            Q_ASSERT(!_firmwareFilename.isEmpty());
            if (_firmwareFilename == "selectfile") {
                _firmwareFilename = QFileDialog::getOpenFileName(this,
                                                            tr("Select Firmware File"),                                             // Dialog title
                                                             QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),   // Initial directory
                                                             tr("Firmware Files (*.px4 *.bin)"));                                   // File filter

            }
            if (!_firmwareFilename.isEmpty()) {
                _upgradeFirmwareSelected();
            }
            break;
            
        case upgradeStateFirmwareSelected:
            if (_downloadFirmware()) {
                _upgradeFirmwareDownloading();
            } else {
                _upgradeDownloadFailed();
            }
            break;
            
        case upgradeStateFirmwareDownloaded:
            if (_bootloaderProgram()) {
                _upgradeBoardUpgraded();
            } else {
                _upgradeBoardUpgradeFailed();
            }
            break;
            
        default:
            Q_ASSERT(false);
            break;
    }
}

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

void PX4FirmwareUpgrade::_tryAgain(void)
{
    switch (_upgradeState) {
        case upgradeStateBoardNotFound:
            _upgradeState = upgradeStateBegin;
            _nextStep();
            break;
            
        case upgradeStateDownloadFailed:
            // FIXME: Duplication
            if (_downloadFirmware()) {
                _upgradeFirmwareDownloading();
            } else {
                _upgradeDownloadFailed();
            }
            break;
            
        default:
            Q_ASSERT(false);
            break;
    }
}

bool PX4FirmwareUpgrade::_findBoard(void)
{
    foreach (QSerialPortInfo info, QSerialPortInfo::availablePorts()) {
        
        // Check for valid handles
        if (info.portName().isEmpty())
            continue;

        // FIXME: Won't this OR find a 3dr radio as well?
        // FIXME: Magic number
        if (info.description().contains("PX4") || info.vendorIdentifier() == 9900 /* 3DR */) {
            
            qDebug() << "Found Board:";
            qDebug() << "\tport name:" << info.portName();
            qDebug() << "\tdescription:" << info.description();
            qDebug() << "\tsystem location:" << info.systemLocation();
            qDebug() << "\tvendor ID:" << info.vendorIdentifier();
            qDebug() << "\tproduct ID:" << info.productIdentifier();
            
            _portName = info.portName();
            _portDescription = info.description();
            
#ifdef Q_OS_WIN
            // Stupid windows fixes
            _portName.prepend("\\\\.\\");
#endif
            
            return true;
        }
    }
    
    return false;
}

bool PX4FirmwareUpgrade::_bootloaderWrite(const char* data, qint64 maxSize, const QString errorPrefix)
{
    qint64 bytesWritten = _bootloaderPort.write(data, maxSize);
    if (bytesWritten == -1) {
        _portErrorString = tr("Could not write %1, error: %2").arg(errorPrefix).arg(_bootloaderPort.errorString());
        return false;
    }
    if (bytesWritten != maxSize) {
        _portErrorString = tr("In correct number of bytes returned for %1 write: %2").arg(errorPrefix).arg(bytesWritten);
        return false;
    }
    if (!_bootloaderPort.waitForBytesWritten(1000)) {
        _portErrorString = tr("Timeout waiting for %1 write").arg(errorPrefix);
        return false;
    }
    
    return true;
}

bool PX4FirmwareUpgrade::_bootloaderWrite(const char data, const QString errorPrefix)
{
    char buf[1] = { data };
    return _bootloaderWrite(buf, 1, errorPrefix);
}

bool PX4FirmwareUpgrade::_bootloaderRead(char* data, qint64 maxSize, const QString errorPrefix, int readTimeout)
{
    qint64 bytesRead;
    
    if (_bootloaderPort.bytesAvailable() < maxSize) {
        if (!_bootloaderPort.waitForReadyRead(readTimeout)) {
            _portErrorString = tr("Timeout waiting for %1 response: %2").arg(errorPrefix).arg(_bootloaderPort.errorString());
            return false;
        }
    }
    
    bytesRead = _bootloaderPort.read(data, maxSize);
    if (bytesRead == -1) {
        _portErrorString = tr("Could not read %1 resonse, error: %2").arg(errorPrefix).arg(_bootloaderPort.errorString());
        return false;
    }
    if (bytesRead != maxSize) {
        _portErrorString = tr("In correct number of bytes returned for %1 response: %2").arg(errorPrefix).arg(bytesRead);
        return false;
    }
    
    return true;
}

bool PX4FirmwareUpgrade::_bootloaderGetCommandResponse(const QString& errorPrefix, int responseTimeout)
{
    char response[2];
    
    // Read command response
    if (!_bootloaderRead(response, 2, errorPrefix, responseTimeout)) {
        return false;
    }
    
    // Make sure the correct sync byte
    if (response[0] != PROTO_INSYNC) {
        _portErrorString = tr("Invalid sync response from %1 command: %2").arg(errorPrefix).arg((int)response[0], 0, 16);
        return false;
    }
    
    switch (response[1]) {
        case PROTO_OK:
            return true;
            
        case PROTO_INVALID:
            _portErrorString = tr("Invalid command sent to bootloader: %1").arg(errorPrefix);
            return false;
            
        case PROTO_FAILED:
            _portErrorString = tr("Bootloader command %1 failed").arg(errorPrefix);
            return false;
            
        default:
            _portErrorString = tr("Unknown failure response from bootloader command %1: %2").arg((int)response[1], 0, 16).arg(errorPrefix);
            return false;
    }
    
    return true;
}

bool PX4FirmwareUpgrade::_bootloaderGetBoardInfo(char param, uint32_t& value)
{
    char buf[3] = { PROTO_GET_DEVICE, param, PROTO_EOC };
    
    if (!_bootloaderWrite(buf, sizeof(buf), "GET_DEVICE")) {
        return false;
    }
    if (!_bootloaderRead((char*)&value, sizeof(value), tr("GET_DEVICE read return value"))) {
        return false;
    }
    return _bootloaderGetCommandResponse("GET_DEVICE");
}

bool PX4FirmwareUpgrade::_bootloaderCommand(const char cmd, const QString& errorPrefix, int responseTimeout)
{
    char buf[2] = { cmd, PROTO_EOC };
    
    if (!_bootloaderWrite(buf, 2, errorPrefix)) {
        return false;
    }
    return _bootloaderGetCommandResponse(errorPrefix, responseTimeout);
}

bool PX4FirmwareUpgrade::_bootloaderProgram(void)
{
    Q_ASSERT(!_bootloaderPort.isOpen());
    
    _ui.progressBar->setMinimum(0);
    _ui.progressBar->setMinimum(_imageSize);
    
    if (!_bootloaderPort.open(QIODevice::ReadWrite)) {
        _ui.statusLabel->setText(tr("Could not open port %1, error: %2").arg(_portName).arg(_bootloaderPort.errorString()));
        return false;
    }
    
    // Erase is slow, need larger timeout
    if (!_bootloaderCommand(PROTO_CHIP_ERASE, "CHIP_ERASE", 20000)) {
        _ui.statusLabel->setText(tr("ERROR: Unable to erase firmware, %1").arg(_portErrorString));
        return false;
    }
    
    QFile firmwareFile(_firmwareFilename);
    if (!firmwareFile.open(QIODevice::ReadOnly)) {
        _ui.statusLabel->setText(tr("ERROR: Unabled to open firmware file %1, %2").arg(firmwareFile.errorString()));
        return false;
    }
    
    Q_ASSERT(_imageSize == (uint32_t)firmwareFile.size());
    
    char imageBuf[PROG_MULTI_MAX];
    int bytesSent = 0;
    
    Q_ASSERT(PROG_MULTI_MAX <= 0x8F);
    
    while (bytesSent < (int)_imageSize) {
        int bytesToSend = _imageSize - bytesSent;
        if (bytesToSend > (int)sizeof(imageBuf)) {
            bytesToSend = (int)sizeof(imageBuf);
        }
        
        Q_ASSERT((bytesToSend % 4) == 0);
        
        int bytesWritten = firmwareFile.read(imageBuf, bytesToSend);
        if (bytesWritten == -1 || bytesWritten != bytesToSend) {
            // FIXME: Better error handling
            _ui.statusLabel->setText(tr("ERROR: read, %1").arg(firmwareFile.errorString()));
            return false;
        }
        
        Q_ASSERT(bytesToSend <= 0x8F);
        
        // FIXME: Error handling
        if (!_bootloaderWrite(PROTO_PROG_MULTI, "PROG_MULTI") ||
                !_bootloaderWrite((char)bytesToSend, "PROG_MULTI") ||
                !_bootloaderWrite(imageBuf, bytesToSend, "PROG_MULTI") ||
                !_bootloaderWrite(PROTO_EOC, "PROG_MULTI") ||
                !_bootloaderGetCommandResponse("PROG_MULTI")) {
            _ui.statusLabel->setText(tr("ERROR: Unable to program flash %1").arg(_portErrorString));
            return false;
        }
        
        bytesSent += bytesToSend;
        
        _ui.progressBar->setValue(bytesSent);
        // FIXME: Not working
        _ui.progressBar->repaint();
    }
    
    firmwareFile.close();
    
    return true;
}

bool PX4FirmwareUpgrade::_findBootloader(void)
{
    QString errorString;
    
    Q_ASSERT(!_bootloaderPort.isOpen());
    
    _bootloaderPort.setPortName(_portName);
    _bootloaderPort.setBaudRate(QSerialPort::Baud115200);
    _bootloaderPort.setDataBits(QSerialPort::Data8);
    _bootloaderPort.setParity(QSerialPort::NoParity);
    _bootloaderPort.setStopBits(QSerialPort::OneStop);
    _bootloaderPort.setFlowControl(QSerialPort::NoFlowControl);
    
    if (!_bootloaderPort.open(QIODevice::ReadWrite)) {
        _portErrorString = tr("Could not open port %1, error: %2").arg(_portName).arg(_serialPortErrorString(_bootloaderPort.error()));
        goto Error;
    }
    
    // Drain out any remaining input or output from the port
    if (!_bootloaderPort.clear()) {
        _portErrorString = tr("Unable to clear port");
        goto Error;
    }
    
    // Send sync command
    if (!_bootloaderCommand(PROTO_GET_SYNC, "GET_SYNC")) {
        goto Error;
    }
    
    // Now get the various bits of board info from the bootloader
    
    if (!_bootloaderGetBoardInfo(INFO_BL_REV, _bootloaderVersion)) {
        goto Error;
    }
    if (_bootloaderVersion < BL_REV_MIN || _bootloaderVersion > BL_REV_MAX) {
        _portErrorString = tr("Found unsupported bootloader version: %1").arg(_bootloaderVersion);
        goto Error;
    }
    
    if (!_bootloaderGetBoardInfo(INFO_BOARD_ID, _boardID)) {
        goto Error;
    }
    if (_boardID != _boardIDPX4Flow && _boardID != _boardIDPX4FMUV1 && _boardID != _boardIDPX4FMUV2) {
        _portErrorString = tr("This board is not support by upgrade: %1").arg(_boardID);
        goto Error;
    }
    
    if (!_bootloaderGetBoardInfo(INFO_FLASH_SIZE, _boardFlashSize)) {
        goto Error;
    }
    
    _bootloaderPort.close();
    
    return true;
    
Error:
    _bootloaderPort.close();
    _ui.statusLabel->setText(_portErrorString);
    return false;
}

/// @brief Enables the specified set of wizard buttons. Buttons which are not
/// specified are disabled.
void PX4FirmwareUpgrade::_enableWizardButtons(unsigned int buttons)
{
    _ui.tryAgain->setEnabled(buttons & wizardButtonTryAgain);
    _ui.skip->setEnabled(buttons & wizardButtonSkip);
    _ui.cancel->setEnabled(buttons & wizardButtonCancel);
    _ui.next->setEnabled(buttons & wizardButtonNext);
}


/// @brief Set up for the upgradeStateBegin state. This state tells the users to start the process
/// by connecting and clicking the Scan button.
void PX4FirmwareUpgrade::_upgradeBegin(void)
{
    // FIXME: What about disconnect all?
    
    _upgradeState = upgradeStateBegin;

    // Reset internal state
    _portName.clear();
    _portDescription.clear();
    _firmwareFilename.clear();
    _bootloaderVersion = 0;
    _boardID = 0;
    Q_ASSERT(!_bootloaderPort.isOpen());
    
    _ui.boardFoundCheck->setCheckState(Qt::Unchecked);
    _ui.port->setVisible(false);
    _ui.description->setVisible(false);
    
    _ui.bootloaderFoundCheck->setCheckState(Qt::Unchecked);
    _ui.bootloaderVersion->setVisible(false);
    _ui.boardID->setVisible(false);
    _ui.icon->setVisible(false);
    
    _ui.selectFirmwareCheck->setCheckState(Qt::Unchecked);
    _ui.firmwareCombo->setVisible(false);
    _ui.firmwareCombo->setEnabled(true);
    
    _ui.firmwareDownloadedCheck->setCheckState(Qt::Unchecked);
    _ui.boardUpgradedCheck->setCheckState(Qt::Unchecked);
    
    _ui.progressBar->setVisible(false);
    
    _enableWizardButtons(wizardButtonNext);
    
    _ui.next->setText(tr("Scan"));
    
    _ui.statusLog->setText(tr("Connect your Pixhawk or PX4Flow board via USB. "
                              "Then click the 'Scan' button to find the board and begin the firmware upgrade process."));
    _ui.statusLabel->clear();
}

/// @brief Set up for the upgradeStateBoardNotFound state. This state tells the users to disconnect and Try Again.
void PX4FirmwareUpgrade::_upgradeBoardNotFound(void)
{
    _upgradeState = upgradeStateBoardNotFound;
    
    _enableWizardButtons(wizardButtonTryAgain | wizardButtonCancel);
    
    _ui.statusLog->setText(tr("Unable to detect your board. If the board is currently connected via USB. "
                              "Disconnect it, reconnect it, wait a few seconds and click the 'Try Again' button."));
}

/// @brief Set up for the upgradeStateBoardFound state. This state tells the users to disconnect and Try Again.
void PX4FirmwareUpgrade::_upgradeBoardFound(void)
{
    _upgradeState = upgradeStateBoardFound;

    _ui.boardFoundCheck->setCheckState(Qt::Checked);
    
    _ui.port->setText(tr("Port: %1").arg(_portName));
    _ui.port->setVisible(true);
    
    _ui.description->setText(tr("Name: %1").arg(_portDescription));
    _ui.description->setVisible(true);
    
    _ui.next->setText(tr("Next"));
    
    _enableWizardButtons(wizardButtonTryAgain | wizardButtonCancel | wizardButtonNext);
    
    if (_findBootloader()) {
        _upgradeBootloaderFound();
    } else {
        _upgradeBootloaderNotFound();
    }
}

/// @brief Set up for the upgradeStateBootloaderFound state.
void PX4FirmwareUpgrade::_upgradeBootloaderFound(void)
{
    _upgradeState = upgradeStateBootloaderFound;

    _ui.bootloaderFoundCheck->setCheckState(Qt::Checked);
    
    _ui.bootloaderVersion->setText(QString("Version %1").arg(_bootloaderVersion));
    _ui.bootloaderVersion->setVisible(true);
    
    _ui.boardID->setText(QString("Board ID %1").arg(_boardID));
    _ui.boardID->setVisible(true);
    
    _setBoardIcon(_boardID);
    _ui.icon->setVisible(true);
    
    _setFirmwareCombo(_boardID);
    _ui.firmwareCombo->setVisible(true);
    
    _enableWizardButtons(wizardButtonCancel);
    
    _ui.statusLog->setText(tr("Please select the firmware you would like to upload to the board from the dropdown to the right."));
    _ui.statusLabel->clear();
}

void PX4FirmwareUpgrade::_upgradeFirmwareSelected(void)
{
    _upgradeState = upgradeStateFirmwareSelected;
    
    _ui.selectFirmwareCheck->setCheckState(Qt::Checked);
    
    _ui.progressBar->setVisible(true);
    _ui.progressBar->setValue(0);
    
    _ui.firmwareCombo->setEnabled(false);
    
    _enableWizardButtons(wizardButtonCancel | wizardButtonNext);
    
    _ui.statusLog->setText(tr("Click Next to download your selected firmware."));
    _ui.statusLabel->clear();
}

/// @brief Set up for the upgradeStateBootloaderNotFound state.
void PX4FirmwareUpgrade::_upgradeBootloaderNotFound(void)
{
    _upgradeState = upgradeStateBootloaderNotFound;
    
    _ui.statusLog->setText(tr("Unable to detect bootloader. "
                              "Disconnect the board, reconnect it, wait a few seconds and click the 'Try Again' button."));
    
    _enableWizardButtons(wizardButtonTryAgain | wizardButtonCancel);
}

void PX4FirmwareUpgrade::_upgradeFirmwareDownloading(void)
{
    _upgradeState = upgradeStateFirmwareDownloading;
    
    _enableWizardButtons(wizardButtonCancel);
    
    _ui.statusLog->setText(tr("Your firmware is currently downloading."));
}

void PX4FirmwareUpgrade::_upgradeFirmwareDownloaded(void)
{
    _upgradeState = upgradeStateFirmwareDownloaded;
    
    _ui.firmwareDownloadedCheck->setCheckState(Qt::Checked);
    
    _enableWizardButtons(wizardButtonCancel | wizardButtonNext);
    
    _ui.statusLog->setText(tr("Click Next to upload the selected firmware to your board."));
    _ui.statusLabel->clear();
}

void PX4FirmwareUpgrade::_upgradeDownloadFailed(void)
{
    _upgradeState = upgradeStateDownloadFailed;
    
    _enableWizardButtons(wizardButtonCancel | wizardButtonTryAgain);
    
    _ui.statusLog->setText(tr("The download of the firmware you selected failed. Click 'Try Again' to attempt the download again, or 'Cancel' to abort the upgrade."));
}

void PX4FirmwareUpgrade::_upgradeBoardUpgraded(void)
{
    _upgradeState = upgradeStateBoardUpgraded;
    
    _ui.boardUpgradedCheck->setCheckState(Qt::Checked);
    
    _enableWizardButtons(wizardButtonTryAgain);
    
    _ui.statusLog->setText(tr("Board upgraded."));
    _ui.statusLabel->clear();
}

void PX4FirmwareUpgrade::_upgradeBoardUpgradeFailed(void)
{
    _upgradeState = upgradeStateBoardUpgradeFailed;
    
    _enableWizardButtons(wizardButtonCancel | wizardButtonTryAgain);
    
    _ui.statusLog->setText(tr("Flasing the firmware to the board failed. Click 'Try Again' to attempt again, or 'Cancel' to abort the upgrade."));
}

/// @brief Begins the process of downloading the firmware file.
/// @return true: Download process has begin, false: Could not start download process
bool PX4FirmwareUpgrade::_downloadFirmware(void)
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
            _ui.statusLabel->setText(tr("Unabled to find writable download location. Tried downloads and temp directory."));
            return false;
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
    
    return true;
}

void PX4FirmwareUpgrade::_downloadProgress(qint64 curr, qint64 total)
{
    // Take care of cases where 0 / 0 is emitted as error return value
    if (total > 0) {
        _ui.progressBar->setValue((curr*100) / total);
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
        _ui.statusLabel->setText(tr("Could not save downloaded file to %1. Error: %2").arg(downloadFilename).arg(file.errorString()));
        _upgradeDownloadFailed();
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
            _ui.statusLabel->setText(tr("Unable to open firmware file %1, error: %2").arg(downloadFilename).arg(px4File.errorString()));
            _upgradeDownloadFailed();
            return;
        }
        
        QByteArray bytes = px4File.readAll();
        px4File.close();
        QJsonDocument doc = QJsonDocument::fromJson(bytes);
        
        if (doc.isNull()) {
            // FIXME: Better error message
            _ui.statusLabel->setText(tr("supplied file is not a valid JSON document"));
            _upgradeDownloadFailed();
            return;
        }
        
        QJsonObject px4Json = doc.object();
        
        // FIXME: Magic strings
        static const char* rgJsonKeys[] = { "board_id", "image_size", "description", "git_identity" };
        for (size_t i=0; i<sizeof(rgJsonKeys)/sizeof(rgJsonKeys[0]); i++) {
            if (!px4Json.contains(rgJsonKeys[i])) {
                _ui.statusLabel->setText(tr("Incorrectly formatted firmware file. No %1 key.").arg(rgJsonKeys[i]));
                _upgradeDownloadFailed();
                return;
            }
        }
        
        uint32_t firmwareBoardID = (uint32_t)px4Json.value(QString("board_id")).toInt();
        if (firmwareBoardID != _boardID) {
            _ui.statusLabel->setText(tr("Downloaded firmware board id does not match hardware board id: %1 != %2").arg(firmwareBoardID).arg(_boardID));
            _upgradeDownloadFailed();
            return;
        }
        
        _imageSize = px4Json.value(QString("image_size")).toInt();
        if (_imageSize == 0) {
            _ui.statusLabel->setText(tr("Image size of 0 in .px4 file %1").arg(downloadFilename));
            _upgradeDownloadFailed();
            return;
        }

#if 0
        // FIXME: Not finished
        QString str = px4.value(QString("description")).toString();
        log("description: %s", str.toStdString().c_str());
        QString identity = px4.value(QString("git_identity")).toString();
        log("GIT identity: %s", identity.toStdString().c_str());
#endif
        
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
            _ui.statusLabel->setText(tr("Firmware file has 0 length image"));
            _upgradeDownloadFailed();
            return;
        }
        if (b.count() != (int)_imageSize) {
            _ui.statusLabel->setText(tr("Image size for decompressed image does not match stored image size: Expected(%1) Actual(%2)").arg(_imageSize).arg(b.count()));
            _upgradeDownloadFailed();
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
            _ui.statusLabel->setText(tr("Unable to open decompressed file %1 for writing, error: %2").arg(decompressFilename).arg(decompressFile.errorString()));
            _upgradeDownloadFailed();
            return;
        }
        
        qint64 bytesWritten = decompressFile.write(b);
        if (bytesWritten != b.count()) {
            _ui.statusLabel->setText(tr("Write failed for decompressed image file, error: %1").arg(decompressFile.errorString()));
            _upgradeDownloadFailed();
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
            _ui.statusLabel->setText(tr("Downloaded firmware board id does not match hardware board id: %1 != %2").arg(firmwareBoardID).arg(_boardID));
            _upgradeDownloadFailed();
            return;
        }
        
        _firmwareFilename = downloadFilename;
        
        QFile binFile(_firmwareFilename);
        if (!binFile.open(QIODevice::ReadOnly)) {
            _ui.statusLabel->setText(tr("Unabled to open firmware file %1, %2").arg(_firmwareFilename).arg(binFile.errorString()));
            _upgradeDownloadFailed();
        }
        _imageSize = (uint32_t)binFile.size();
        binFile.close();
    } else {
        // Standard firmware builds (stable/continuous/...) are always .bin or .px4. Select file dialog for custom
        // firmware filters to .bin and .px4. So we should ever get a file that ends in anything else.
        Q_ASSERT(false);
    }
    
    if (_imageSize > _boardFlashSize) {
        _ui.statusLabel->setText(tr("Image size of %1 is too large for board flash size %2").arg(_imageSize).arg(_boardFlashSize));
        _upgradeDownloadFailed();
        return;
    }

    _upgradeFirmwareDownloaded();
}

void PX4FirmwareUpgrade::_downloadError(QNetworkReply::NetworkError code)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    Q_ASSERT(reply);
    
    reply->abort();
    _downloadManager->deleteLater();
    _downloadManager = NULL;
    
    // FIXME: Text for error code
    _ui.statusLabel->setText(tr("Error during download. Error: %1").arg(code));
    
    _upgradeDownloadFailed();
}

