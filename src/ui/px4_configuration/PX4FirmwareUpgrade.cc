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
#include <QDebug>

#include "QGCFileDialog.h"
#include "QGCMessageBox.h"

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
    connect(_ui->firmwareCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(_firmwareSelected(int)));
    
    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBoard, this, &PX4FirmwareUpgrade::_foundBoard);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBootloader, this, &PX4FirmwareUpgrade::_foundBootloader);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::bootloaderSyncFailed, this, &PX4FirmwareUpgrade::_bootloaderSyncFailed);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::error, this, &PX4FirmwareUpgrade::_error);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::complete, this, &PX4FirmwareUpgrade::_complete);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::findTimeout, this, &PX4FirmwareUpgrade::_findTimeout);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::updateProgress, this, &PX4FirmwareUpgrade::_updateProgress);
    
    connect(&_eraseTimer, &QTimer::timeout, this, &PX4FirmwareUpgrade::_eraseProgressTick);
    
    _setupState(upgradeStateBegin);
}

PX4FirmwareUpgrade::~PX4FirmwareUpgrade()
{
}

/// @brief Returns the state machine entry for the specified state.
const PX4FirmwareUpgrade::stateMachineEntry* PX4FirmwareUpgrade::_getStateMachineEntry(enum PX4FirmwareUpgrade::upgradeStates state)
{
    static const char* msgBegin = "If you are currently connected to your Pixhawk board via QGroundControl, you must 'Disconnect' from the board. "
                                    "If your board is connected via USB, you must unplug the USB cable.\n\n"
                                    "Click 'Next' when these two steps are complete to begin upgrading.";
    static const char* msgBoardSearch = "Plug in your board via USB now...";
    static const char* msgBoardNotFound = "Unable to detect your board. If the board is currently connected via USB. Disconnect it, and click 'Try Again'.";
    static const char* msgBootloaderSearch = "Searching for Bootloader...";
    static const char* msgBootloaderNotFound = "Unable to connect to Bootloader. If the board is currently connected via USB. Disconnect it, and click 'Try Again'.";
    static const char* msgBootloaderError = "An error occured while communicating with the Bootloader.";
    static const char* msgFirmwareSelect = "Please select the firmware you would like to upload to the board from the dropdown to the right.";
    static const char* msgFirmwareDownloading = "Firmware downloading...";
    static const char* msgFirmwareDownloadFailed = "Firmware download failed";
    static const char* msgFirmwareBoardErasing = "Erasing old firmware from board...";
    static const char* msgFirmwareBoardEraseFailed = "Board erase failed.";
    static const char* msgFirmwareBoardFlashing = "Flashing new firmware onto board...";
    static const char* msgFirmwareBoardFlashError = "A failure has occured while flashing the new firmware to your board. "
                                                        "This has left the board in an inconsistent state.\n\n"
                                                        "There currently is an known issue which does not yet have a fix which may cause this.\n\n"
                                                        "You should click 'Try Again' to attempt the upgrade process again.";
    static const char* msgFirmwareBoardVerifying = "Verifying firmware on board...";
    static const char* msgFirmwareBoardVerifyError = "Verification of flash memory on board failed. "
                                                    "This has left the board in an inconsistent state. "
                                                    "You should click 'Try Again' to attempt the upgrade process again.";
    static const char* msgFirmwareBoardUpgraded = "Your board has been upgraded successfully.\n\nYou can now connect to your board via QGroundControl\n\nClick 'Try Again' to do another upgrade.";

    static const stateMachineEntry rgStateMachine[] = {
        //State                                     Next command                            Cancel command                              Try Again command           State Text
        { upgradeStateBegin,                        &PX4FirmwareUpgrade::_findBoard,        NULL,                                       NULL,                               msgBegin },
        { upgradeStateBoardSearch,                  NULL,                                   &PX4FirmwareUpgrade::_cancelFind,           NULL,                               msgBoardSearch },
        { upgradeStateBoardNotFound,                NULL,                                   &PX4FirmwareUpgrade::_cancel,               &PX4FirmwareUpgrade::_findBoard,    msgBoardNotFound },
        { upgradeStateBootloaderSearch,             NULL,                                   &PX4FirmwareUpgrade::_cancelFind,           NULL,                               msgBootloaderSearch },
        { upgradeStateBootloaderNotFound,           NULL,                                   &PX4FirmwareUpgrade::_cancel,               &PX4FirmwareUpgrade::_restart,      msgBootloaderNotFound },
        { upgradeStateBootloaderError,              NULL,                                   &PX4FirmwareUpgrade::_cancel,               NULL,                               msgBootloaderError },
        { upgradeStateFirmwareSelect,               &PX4FirmwareUpgrade::_getFirmwareFile,  &PX4FirmwareUpgrade::_cancel,               NULL,                               msgFirmwareSelect },
        { upgradeStateFirmwareDownloading,          NULL,                                   &PX4FirmwareUpgrade::_cancelDownload,       NULL,                               msgFirmwareDownloading },
        { upgradeStateDownloadFailed,               NULL,                                   NULL,                                       &PX4FirmwareUpgrade::_restart,      msgFirmwareDownloadFailed },
        { upgradeStateErasing,                      NULL,                                   NULL,                                       NULL,                               msgFirmwareBoardErasing },
        { upgradeStateEraseError,                   NULL,                                   &PX4FirmwareUpgrade::_cancel,               NULL,                               msgFirmwareBoardEraseFailed },
        { upgradeStateFlashing,                     NULL,                                   NULL,                                       NULL,                               msgFirmwareBoardFlashing },
        { upgradeStateFlashError,                   NULL,                                   NULL,                                       &PX4FirmwareUpgrade::_restart,      msgFirmwareBoardFlashError },
        { upgradeStateVerifying,                    NULL,                                   NULL,                                       NULL,                               msgFirmwareBoardVerifying },
        { upgradeStateVerifyError,                  NULL,                                   NULL,                                       &PX4FirmwareUpgrade::_restart,      msgFirmwareBoardVerifyError },
        { upgradeStateBoardUpgraded,                NULL,                                   NULL,                                       &PX4FirmwareUpgrade::_restart,      msgFirmwareBoardUpgraded },
    };
    
    const stateMachineEntry* entry = &rgStateMachine[state];
    
    // Validate that our state array has not gotten out of sync
    for (size_t i=0; i<upgradeStateMax; i++) {
        Q_ASSERT(rgStateMachine[i].state == i);
    }

    return entry;
}

/// @brief Sets up the Wizard according to the specified state
void PX4FirmwareUpgrade::_setupState(enum PX4FirmwareUpgrade::upgradeStates state)
{
    _upgradeState = state;
    
    const stateMachineEntry* stateMachine = _getStateMachineEntry(state);
    
    _ui->tryAgain->setEnabled(stateMachine->tryAgain != NULL);
    _ui->skip->setEnabled(false);
    _ui->cancel->setEnabled(stateMachine->cancel != NULL);
    _ui->next->setEnabled(stateMachine->next != NULL);
    
    _ui->statusLog->setText(stateMachine->msg);
    
    if (_upgradeState == upgradeStateDownloadFailed) {
        // Bootloader is still open, reboot to close and heopfully get back to FMU
        _threadController->sendBootloaderReboot();
    }
    
    _updateIndicatorUI();
}

/// @brief Updates the Indicator UI which is to the right of the Wizard area to match the current
///         upgrade state.
void PX4FirmwareUpgrade::_updateIndicatorUI(void)
{
    if (_upgradeState == upgradeStateBegin) {
        // Reset to intial state. All check boxes unchecked, all additional information hidden.
        
        _ui->statusLabel->clear();
        
        _ui->progressBar->setValue(0);
        _ui->progressBar->setTextVisible(false);
        
        _ui->boardFoundCheck->setCheckState(Qt::Unchecked);
        _ui->port->setVisible(false);
        _ui->description->setVisible(false);
        
        _ui->bootloaderFoundCheck->setCheckState(Qt::Unchecked);
        _ui->bootloaderVersion->setVisible(false);
        _ui->boardID->setVisible(false);
        _ui->icon->setVisible(false);
        
        _ui->firmwareCombo->setVisible(false);
        _ui->firmwareCombo->setEnabled(true);
        
        _ui->selectFirmwareCheck->setCheckState(Qt::Unchecked);
        _ui->firmwareDownloadedCheck->setCheckState(Qt::Unchecked);
        _ui->boardUpgradedCheck->setCheckState(Qt::Unchecked);
        
    } else if (_upgradeState == upgradeStateBootloaderSearch){
        // We have found the board
        
        _ui->statusLabel->clear();
        
        _ui->progressBar->setValue(0);
        
        _ui->boardFoundCheck->setCheckState(Qt::Checked);
        
        _ui->port->setText("Port: " + _portName);
        _ui->description->setText("Name: " +_portDescription);
        
        _ui->port->setVisible(true);
        _ui->description->setVisible(true);
        
    } else if (_upgradeState == upgradeStateFirmwareSelect) {
        // We've found the bootloader and need to set up firmware selection
        
        _ui->statusLabel->clear();

        _ui->progressBar->setValue(0);
        
        _ui->bootloaderFoundCheck->setCheckState(Qt::Checked);
        
        
        _ui->bootloaderVersion->setText(QString("Version: %1").arg(_bootloaderVersion));
        _ui->boardID->setText(QString("Board ID: %1").arg(_boardID));
        _setBoardIcon(_boardID);
        _setFirmwareCombo(_boardID);
        
        _ui->bootloaderVersion->setVisible(true);
        _ui->boardID->setVisible(true);
        _ui->icon->setVisible(true);
        _ui->firmwareCombo->setVisible(true);
        _ui->firmwareCombo->setEnabled(true);
        _ui->firmwareCombo->setCurrentIndex(0);
        
    } else if (_upgradeState == upgradeStateFirmwareDownloading) {
        
        _ui->statusLabel->clear();
        _ui->selectFirmwareCheck->setCheckState(Qt::Checked);
        _ui->firmwareCombo->setEnabled(false);
        
    } else if (_upgradeState == upgradeStateFlashing) {
        
        _ui->statusLabel->clear();
        _ui->progressBar->setValue(0);
        _ui->firmwareDownloadedCheck->setCheckState(Qt::Checked);
        
    } else if (_upgradeState == upgradeStateBoardUpgraded) {
        
        _ui->statusLabel->clear();
        _ui->progressBar->setValue(0);
        _ui->boardUpgradedCheck->setCheckState((_upgradeState >= upgradeStateBoardUpgraded) ? Qt::Checked : Qt::Unchecked);
        
    }
}

/// @brief Responds to a click on the Next Button calling the appropriate method as specified by the state machine.
void PX4FirmwareUpgrade::_nextButton(void)
{
    const stateMachineEntry* stateMachine = _getStateMachineEntry(_upgradeState);
    
    Q_ASSERT(stateMachine->next != NULL);
    
    (this->*stateMachine->next)();
}


/// @brief Responds to a click on the Cancel Button calling the appropriate method as specified by the state machine.
void PX4FirmwareUpgrade::_cancelButton(void)
{
    const stateMachineEntry* stateMachine = _getStateMachineEntry(_upgradeState);
    
    Q_ASSERT(stateMachine->cancel != NULL);
    
    (this->*stateMachine->cancel)();
}

/// @brief Responds to a click on the Try Again Button calling the appropriate method as specified by the state machine.
void PX4FirmwareUpgrade::_tryAgainButton(void)
{
    const stateMachineEntry* stateMachine = _getStateMachineEntry(_upgradeState);
    
    Q_ASSERT(stateMachine->tryAgain != NULL);
    
    (this->*stateMachine->tryAgain)();
}

/// @brief Cancels a findBoard or findBootloader operation.
void PX4FirmwareUpgrade::_cancelFind(void)
{
    _threadController->cancelFind();
}

/// @brief Cancels the current state and returns to the begin start
void PX4FirmwareUpgrade::_cancel(void)
{
    _setupState(upgradeStateBegin);
}

/// @brief Begins the process or searching for the board
void PX4FirmwareUpgrade::_findBoard(void)
{
    _setupState(upgradeStateBoardSearch);
    _threadController->findBoard(_findBoardTimeoutMsec);
}

/// @brief Called when board has been found by the findBoard process
void PX4FirmwareUpgrade::_foundBoard(bool firstTry, const QString portName, QString portDescription)
{
    if (firstTry) {
        // Board is still plugged
        QGCMessageBox::critical(tr("Firmware Upgrade"), tr("You must unplug you board before beginning the Firmware Upgrade process."));
        _cancel();
    } else {
        _portName = portName;
        _portDescription = portDescription;
        _setupState(upgradeStateBootloaderSearch);
        _findBootloader();
    }
}

/// @brief Begins the findBootloader process to connect to the bootloader
void PX4FirmwareUpgrade::_findBootloader(void)
{
    _setupState(upgradeStateBootloaderSearch);
    _threadController->findBootloader(_portName, _findBootloaderTimeoutMsec);
}

/// @brief Called when the bootloader is connected to by the findBootloader process. Moves the state machine
///         to the next step.
void PX4FirmwareUpgrade::_foundBootloader(int bootloaderVersion, int boardID, int flashSize)
{
    _bootloaderVersion = bootloaderVersion;
    _boardID = boardID;
    _boardFlashSize = flashSize;
    _setupState(upgradeStateFirmwareSelect);
}

/// @brief Called when the findBootloader process is unable to sync to the bootloader. Moves the state
///         machine to the appropriate error state.
void PX4FirmwareUpgrade::_bootloaderSyncFailed(void)
{
    if (_upgradeState == upgradeStateBootloaderSearch) {
        // We can connect to the board, but we still can't talk to the bootloader.
        qDebug() << "Bootloader sync failed";
        _setupState(upgradeStateBootloaderNotFound);
    } else {
        Q_ASSERT(false);
    }
    
}

/// @brief Called when the findBoard or findBootloader process times out. Moves the state machine to the
///         appropriate error state.
void PX4FirmwareUpgrade::_findTimeout(void)
{
    if (_upgradeState == upgradeStateBoardSearch) {
        qDebug() << "Timeout on board search";
        _setupState(upgradeStateBoardNotFound);
    } else if (_upgradeState == upgradeStateBootloaderSearch) {
        qDebug() << "Timeout on bootloader search";
        _setupState(upgradeStateBoardNotFound);
    } else {
        Q_ASSERT(false);
    }
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
    }
    
    if (!imageFile.isEmpty()) {
        bool success = _boardIcon.load(imageFile);
        Q_ASSERT(success);
        Q_UNUSED(success);
        
        int w = _ui->icon->width();
        int h = _ui->icon->height();
        
        _ui->icon->setPixmap(_boardIcon.scaled(w, h, Qt::KeepAspectRatio));
    }
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
            prgFirmware = NULL;
            break;
    }

    if (prgFirmware) {
        _ui->firmwareCombo->addItem(tr("Standard Version (stable)"), prgFirmware[0]);
        _ui->firmwareCombo->addItem(tr("Beta Testing (beta)"), prgFirmware[1]);
        _ui->firmwareCombo->addItem(tr("Developer Build (master)"), prgFirmware[2]);
    }
    _ui->firmwareCombo->addItem(tr("Custom firmware file..."), "selectfile");
}

/// @brief Called when the selection in the firmware combo box changes. Updates the wizard
///         text appropriately with licensing and possibly warning information.
void PX4FirmwareUpgrade::_firmwareSelected(int index)
{
#define SELECT_FIRMWARE_LICENSE "By clicking Next you agree to the terms and disclaimer of the BSD open source license, as redistributed with the source code."
    
    if (_upgradeState == upgradeStateFirmwareSelect) {
        switch (index) {
            case 0:
            case 3:
                _ui->statusLog->setText(tr(SELECT_FIRMWARE_LICENSE));
                break;
                
            case 1:
                _ui->statusLog->setText(tr("WARNING: BETA FIRMWARE\n"
                                           "This firmware version is ONLY intended for beta testers. "
                                           "Although it has received FLIGHT TESTING, it represents actively changed code. Do NOT use for normal operation.\n\n"
                                           SELECT_FIRMWARE_LICENSE));
                break;
                
            case 2:
                _ui->statusLog->setText(tr("WARNING: CONTINUOUS BUILD FIRMWARE\n"
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

/// @brief Prompts the user to select a firmware file if needed and moves the state machine to the
///         download firmware state.
void PX4FirmwareUpgrade::_getFirmwareFile(void)
{
    int index = _ui->firmwareCombo->currentIndex();
    _firmwareFilename = _ui->firmwareCombo->itemData(index).toString();
    Q_ASSERT(!_firmwareFilename.isEmpty());
    if (_firmwareFilename == "selectfile") {
        _firmwareFilename = QGCFileDialog::getOpenFileName(
            this,
            tr("Load Firmware File"),                                              // Dialog Caption
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),   // Initial directory
            tr("Firmware Files (*.px4 *.bin)"));                                   // File filter
    }
    if (!_firmwareFilename.isEmpty()) {
        _downloadFirmware();
    }
}

/// @brief Begins the process of downloading the selected firmware file.
void PX4FirmwareUpgrade::_downloadFirmware(void)
{
    // Split out filename from path
    Q_ASSERT(!_firmwareFilename.isEmpty());
    QString firmwareFilename = QFileInfo(_firmwareFilename).fileName();
    Q_ASSERT(!firmwareFilename.isEmpty());
    
    // Determine location to download file to
    QString downloadFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (downloadFile.isEmpty()) {
        downloadFile = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (downloadFile.isEmpty()) {
            _setupState(upgradeStateDownloadFailed);
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

/// @brief Cancels a download which is in progress.
void PX4FirmwareUpgrade::_cancelDownload(void)
{
    _downloadNetworkReply->abort();
}

/// @brief Updates the progress indicator while downloading
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
        _ui->statusLabel->setText(tr("Could not save downloaded file to %1. Error: %2").arg(downloadFilename).arg(file.errorString()));
        _setupState(upgradeStateDownloadFailed);
        return;
    }
    
    file.write(reply->readAll());
    file.close();
    
    
    if (downloadFilename.endsWith(".px4")) {
        // We need to collect information from the .px4 file as well as pull the binary image out to a seperate file.
        
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
            _ui->statusLabel->setText(tr("supplied file is not a valid JSON document"));
            _setupState(upgradeStateDownloadFailed);
            return;
        }
        
        QJsonObject px4Json = doc.object();

        // Make sure the keys we need are available
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
        
        // Pad image to 4-byte boundary
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
        // firmware filters to .bin and .px4. So we should never get a file that ends in anything else.
        Q_ASSERT(false);
    }
    
    if (_imageSize > _boardFlashSize) {
        _ui->statusLabel->setText(tr("Image size of %1 is too large for board flash size %2").arg(_imageSize).arg(_boardFlashSize));
        _setupState(upgradeStateDownloadFailed);
        return;
    }

    _erase();
}

/// @brief Called when an error occurs during download
void PX4FirmwareUpgrade::_downloadError(QNetworkReply::NetworkError code)
{
    if (code == QNetworkReply::OperationCanceledError) {
        _ui->statusLabel->setText(tr("Download cancelled"));
    } else {
        _ui->statusLabel->setText(tr("Error during download. Error: %1").arg(code));
    }

    _setupState(upgradeStateDownloadFailed);
}

/// @brief Erase the board
void PX4FirmwareUpgrade::_erase(void)
{
    // We set up our own progress bar for erase since the erase command does not provide one
    _eraseTickCount = 0;
    _eraseTimer.start(_eraseTickMsec);
    _setupState(upgradeStateErasing);
    
    // Erase command
    _threadController->erase();
}

/// @brief Signals completion of one of the specified bootloader commands. Moves the state machine to the
///         appropriate next step.
void PX4FirmwareUpgrade::_complete(const int command)
{
    if (command == PX4FirmwareUpgradeThreadWorker::commandProgram) {
        _setupState(upgradeStateVerifying);
        _threadController->verify(_firmwareFilename);
    } else if (command == PX4FirmwareUpgradeThreadWorker::commandVerify) {
        _setupState(upgradeStateBoardUpgraded);
    } else if (command == PX4FirmwareUpgradeThreadWorker::commandErase) {
        _eraseTimer.stop();
        _setupState(upgradeStateFlashing);
        _threadController->program(_firmwareFilename);
    } else if (command == PX4FirmwareUpgradeThreadWorker::commandCancel) {
        if (_upgradeState == upgradeStateBoardSearch) {
            _setupState(upgradeStateBoardNotFound);
        } else if (_upgradeState == upgradeStateBootloaderSearch) {
            _setupState(upgradeStateBootloaderNotFound);
        } else {
            Q_ASSERT(false);
        }
    } else {
        Q_ASSERT(false);
    }
}

/// @brief Signals that an error has occured with the specified bootloader commands. Moves the state machine
///         to the appropriate error state.
void PX4FirmwareUpgrade::_error(const int command, const QString errorString)
{
    _ui->statusLabel->setText(tr("Error: %1").arg(errorString));
    
    if (command == PX4FirmwareUpgradeThreadWorker::commandProgram) {
        _setupState(upgradeStateFlashError);
    } else if (command == PX4FirmwareUpgradeThreadWorker::commandErase) {
        _setupState(upgradeStateEraseError);
    } else if (command == PX4FirmwareUpgradeThreadWorker::commandBootloader) {
        _setupState(upgradeStateBootloaderError);
    } else if (command == PX4FirmwareUpgradeThreadWorker::commandVerify) {
        _setupState(upgradeStateVerifyError);
    } else {
        Q_ASSERT(false);
    }
}

/// @brief Updates the progress bar from long running bootloader commands
void PX4FirmwareUpgrade::_updateProgress(int curr, int total)
{
    _ui->progressBar->setValue((curr*100) / total);
}

/// @brief Resets the state machine back to the beginning
void PX4FirmwareUpgrade::_restart(void)
{
    _setupState(upgradeStateBegin);
}

/// @brief Moves the progress bar ahead on tick while erasing the board
void PX4FirmwareUpgrade::_eraseProgressTick(void)
{
    _eraseTickCount++;
    _ui->progressBar->setValue((_eraseTickCount*_eraseTickMsec*100) / _eraseTotalMsec);
}
