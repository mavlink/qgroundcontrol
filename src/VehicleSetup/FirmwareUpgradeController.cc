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

#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QQmlProperty>
#include <QDebug>

#include "QGCFileDialog.h"
#include "QGCMessageBox.h"

/// @Brief Constructs a new FirmwareUpgradeController Widget. This widget is used within the PX4VehicleConfig set of screens.
FirmwareUpgradeController::FirmwareUpgradeController(void) :
    _downloadManager(NULL),
    _downloadNetworkReply(NULL),
    _firmwareType(StableFirmware),
    _upgradeButton(NULL),
    _statusLog(NULL)
{
    _threadController = new PX4FirmwareUpgradeThreadController(this);
    Q_CHECK_PTR(_threadController);

    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBoard, this, &FirmwareUpgradeController::_foundBoard);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBootloader, this, &FirmwareUpgradeController::_foundBootloader);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::bootloaderSyncFailed, this, &FirmwareUpgradeController::_bootloaderSyncFailed);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::error, this, &FirmwareUpgradeController::_error);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::complete, this, &FirmwareUpgradeController::_complete);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::findTimeout, this, &FirmwareUpgradeController::_findTimeout);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::updateProgress, this, &FirmwareUpgradeController::_updateProgress);
    
    connect(&_eraseTimer, &QTimer::timeout, this, &FirmwareUpgradeController::_eraseProgressTick);
}

/// @brief Cancels the current state and returns to the begin start
void FirmwareUpgradeController::_cancel(void)
{
    // Bootloader may still still open, reboot to close and heopfully get back to FMU
    _threadController->sendBootloaderReboot();
    
    Q_ASSERT(_upgradeButton);
    _upgradeButton->setEnabled(true);
}

/// @brief Begins the process or searching for the board
void FirmwareUpgradeController::_findBoard(void)
{
    _appendStatusLog(tr("Plug your board into USB now..."));
    _searchingForBoard = true;
    _threadController->findBoard(_findBoardTimeoutMsec);
}

/// @brief Called when board has been found by the findBoard process
void FirmwareUpgradeController::_foundBoard(bool firstTry, const QString portName, QString portDescription)
{
    if (firstTry) {
        // Board is still plugged
        _appendStatusLog(tr("You must unplug your board before beginning the Firmware Upgrade process."));
        _cancel();
    } else {
        _portName = portName;
        _portDescription = portDescription;

        _appendStatusLog(tr("Board found:"));
        _appendStatusLog(tr("  Port: %1").arg(_portName));
        _appendStatusLog(tr("  Description: %1").arg(_portName));
        
        _findBootloader();
    }
}

/// @brief Begins the findBootloader process to connect to the bootloader
void FirmwareUpgradeController::_findBootloader(void)
{
    _appendStatusLog(tr("Attemping to communicate with bootloader..."));
    _searchingForBoard = false;
    _threadController->findBootloader(_portName, _findBootloaderTimeoutMsec);
}

/// @brief Called when the bootloader is connected to by the findBootloader process. Moves the state machine
///         to the next step.
void FirmwareUpgradeController::_foundBootloader(int bootloaderVersion, int boardID, int flashSize)
{
    _bootloaderVersion = bootloaderVersion;
    _boardID = boardID;
    _boardFlashSize = flashSize;
    
    _appendStatusLog(tr("Connected to bootloader:"));
    _appendStatusLog(tr("  Version: %1").arg(_bootloaderVersion));
    _appendStatusLog(tr("  Board ID: %1").arg(_boardID));
    _appendStatusLog(tr("  Flash size: %1").arg(_boardFlashSize));
    
    _getFirmwareFile();
}

/// @brief Called when the findBootloader process is unable to sync to the bootloader. Moves the state
///         machine to the appropriate error state.
void FirmwareUpgradeController::_bootloaderSyncFailed(void)
{
    _appendStatusLog(tr("Unable to sync with bootloader."));
    _cancel();
}

/// @brief Called when the findBoard or findBootloader process times out. Moves the state machine to the
///         appropriate error state.
void FirmwareUpgradeController::_findTimeout(void)
{
    QString msg;
    
    if (_searchingForBoard) {
        msg = tr("Unable to detect your board. If the board is currently connected via USB. Disconnect it and try Upgrade again.");
    } else {
        msg = tr("Unable to communicate with Bootloader. If the board is currently connected via USB. Disconnect it and try Upgrade again.");
    }
    _appendStatusLog(msg);
    _cancel();
}

/// @brief Sets the board image into the icon label according to the board id.
void FirmwareUpgradeController::_setBoardIcon(int boardID)
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
        /*
         // FIXME: NYI

        int w = _ui->icon->width();
        int h = _ui->icon->height();
        
        _ui->icon->setPixmap(_boardIcon.scaled(w, h, Qt::KeepAspectRatio));
         */
    }
}

/// @brief Prompts the user to select a firmware file if needed and moves the state machine to the next state.
void FirmwareUpgradeController::_getFirmwareFile(void)
{
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
    
    Q_ASSERT(sizeof(rgPX4FMUV1Firmware) == sizeof(rgPX4FMUV2Firmware) && sizeof(rgPX4FMUV1Firmware) == sizeof(rgPX4FlowFirmware));
    
    const char** prgFirmware;
    switch (_boardID) {
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

    if (prgFirmware == NULL && _firmwareType != CustomFirmware) {
            QGCMessageBox::critical(tr("Firmware Upgrade"), tr("Attemping to flash an unknown board type, you must select 'Custom firmware file'"));
            _cancel();
            return;
    }
    
    if (_firmwareType == CustomFirmware) {
        _firmwareFilename = QGCFileDialog::getOpenFileName(NULL,                                                                // Parent to main window
                                                           tr("Select Firmware File"),                                          // Dialog Caption
                                                           QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), // Initial directory
                                                           tr("Firmware Files (*.px4 *.bin)"));                                 // File filter
    } else {
        _firmwareFilename = prgFirmware[_firmwareType];
    }
    
    if (_firmwareFilename.isEmpty()) {
        _cancel();
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
    
    // Split out filename from path
    QString firmwareFilename = QFileInfo(_firmwareFilename).fileName();
    Q_ASSERT(!firmwareFilename.isEmpty());
    
    // Determine location to download file to
    QString downloadFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (downloadFile.isEmpty()) {
        downloadFile = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (downloadFile.isEmpty()) {
            _appendStatusLog(tr("Unabled to find writable download location. Tried downloads and temp directory."));
            _cancel();
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
    _appendStatusLog(tr("Download complete"));
    
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
        _appendStatusLog(tr("Could not save downloaded file to %1. Error: %2").arg(downloadFilename).arg(file.errorString()));
        _cancel();
        return;
    }
    
    file.write(reply->readAll());
    file.close();
    
    
    if (downloadFilename.endsWith(".px4")) {
        // We need to collect information from the .px4 file as well as pull the binary image out to a seperate file.
        
        QFile px4File(downloadFilename);
        if (!px4File.open(QIODevice::ReadOnly | QIODevice::Text)) {
            _appendStatusLog(tr("Unable to open firmware file %1, error: %2").arg(downloadFilename).arg(px4File.errorString()));
            return;
        }
        
        QByteArray bytes = px4File.readAll();
        px4File.close();
        QJsonDocument doc = QJsonDocument::fromJson(bytes);
        
        if (doc.isNull()) {
            _appendStatusLog(tr("Supplied file is not a valid JSON document"));
            _cancel();
            return;
        }
        
        QJsonObject px4Json = doc.object();

        // Make sure the keys we need are available
        static const char* rgJsonKeys[] = { "board_id", "image_size", "description", "git_identity" };
        for (size_t i=0; i<sizeof(rgJsonKeys)/sizeof(rgJsonKeys[0]); i++) {
            if (!px4Json.contains(rgJsonKeys[i])) {
                _appendStatusLog(tr("Incorrectly formatted firmware file. No %1 key.").arg(rgJsonKeys[i]));
                _cancel();
                return;
            }
        }
        
        uint32_t firmwareBoardID = (uint32_t)px4Json.value(QString("board_id")).toInt();
        if (firmwareBoardID != _boardID) {
            _appendStatusLog(tr("Downloaded firmware board id does not match hardware board id: %1 != %2").arg(firmwareBoardID).arg(_boardID));
            _cancel();
            return;
        }
        
        _imageSize = px4Json.value(QString("image_size")).toInt();
        if (_imageSize == 0) {
            _appendStatusLog(tr("Image size of 0 in .px4 file %1").arg(downloadFilename));
            _cancel();
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
            _appendStatusLog(tr("Firmware file has 0 length image"));
            _cancel();
            return;
        }
        if (b.count() != (int)_imageSize) {
            _appendStatusLog(tr("Image size for decompressed image does not match stored image size: Expected(%1) Actual(%2)").arg(_imageSize).arg(b.count()));
            _cancel();
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
            _appendStatusLog(tr("Unable to open decompressed file %1 for writing, error: %2").arg(decompressFilename).arg(decompressFile.errorString()));
            _cancel();
            return;
        }
        
        qint64 bytesWritten = decompressFile.write(b);
        if (bytesWritten != b.count()) {
            _appendStatusLog(tr("Write failed for decompressed image file, error: %1").arg(decompressFile.errorString()));
            _cancel();
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
            _appendStatusLog(tr("Downloaded firmware board id does not match hardware board id: %1 != %2").arg(firmwareBoardID).arg(_boardID));
            _cancel();
            return;
        }
        
        _firmwareFilename = downloadFilename;
        
        QFile binFile(_firmwareFilename);
        if (!binFile.open(QIODevice::ReadOnly)) {
            _appendStatusLog(tr("Unabled to open firmware file %1, %2").arg(_firmwareFilename).arg(binFile.errorString()));
            _cancel();
            return;
        }
        _imageSize = (uint32_t)binFile.size();
        binFile.close();
    } else {
        // Standard firmware builds (stable/continuous/...) are always .bin or .px4. Select file dialog for custom
        // firmware filters to .bin and .px4. So we should never get a file that ends in anything else.
        Q_ASSERT(false);
    }
    
    if (_imageSize > _boardFlashSize) {
        _appendStatusLog(tr("Image size of %1 is too large for board flash size %2").arg(_imageSize).arg(_boardFlashSize));
        _cancel();
        return;
    }

    _erase();
}

/// @brief Called when an error occurs during download
void FirmwareUpgradeController::_downloadError(QNetworkReply::NetworkError code)
{
    if (code == QNetworkReply::OperationCanceledError) {
        _appendStatusLog(tr("Download cancelled"));
    } else {
        _appendStatusLog(tr("Error during download. Error: %1").arg(code));
    }
    _cancel();
}

/// @brief Erase the board
void FirmwareUpgradeController::_erase(void)
{
    _appendStatusLog(tr("Erasing previous firmware..."));
    
    // We set up our own progress bar for erase since the erase command does not provide one
    _eraseTickCount = 0;
    _eraseTimer.start(_eraseTickMsec);
    
    // Erase command
    _threadController->erase();
}

/// @brief Signals completion of one of the specified bootloader commands. Moves the state machine to the
///         appropriate next step.
void FirmwareUpgradeController::_complete(const int command)
{
    if (command == PX4FirmwareUpgradeThreadWorker::commandProgram) {
        _appendStatusLog(tr("Verifying board programming..."));
        _threadController->verify(_firmwareFilename);
    } else if (command == PX4FirmwareUpgradeThreadWorker::commandVerify) {
        _appendStatusLog(tr("Upgrade complete"));
        QGCMessageBox::information(tr("Firmware Upgrade"), tr("Upgrade completed succesfully"));
        _cancel();
    } else if (command == PX4FirmwareUpgradeThreadWorker::commandErase) {
        _eraseTimer.stop();
        _appendStatusLog(tr("Flashing new firmware to board..."));
        _threadController->program(_firmwareFilename);
    } else if (command == PX4FirmwareUpgradeThreadWorker::commandCancel) {
        // FIXME: This is no longer needed, no Cancel
        if (_searchingForBoard) {
            _appendStatusLog(tr("Board not found"));
            _cancel();
        } else {
            _appendStatusLog(tr("Bootloader not found"));
            _cancel();
        }
    } else {
        Q_ASSERT(false);
    }
}

/// @brief Signals that an error has occured with the specified bootloader commands. Moves the state machine
///         to the appropriate error state.
void FirmwareUpgradeController::_error(const int command, const QString errorString)
{
    Q_UNUSED(command);
    
    _appendStatusLog(tr("Error: %1").arg(errorString));
    _cancel();
}

/// @brief Updates the progress bar from long running bootloader commands
void FirmwareUpgradeController::_updateProgress(int curr, int total)
{
    // Take care of cases where 0 / 0 is emitted as error return value
    if (total > 0) {
        _progressBar->setProperty("value", (float)curr / (float)total);
    }
}

/// @brief Resets the state machine back to the beginning
void FirmwareUpgradeController::_restart(void)
{
    // FIXME: NYI
    //_setupState(upgradeStateBegin);
}

/// @brief Moves the progress bar ahead on tick while erasing the board
void FirmwareUpgradeController::_eraseProgressTick(void)
{
    _eraseTickCount++;
    _progressBar->setProperty("value", (float)(_eraseTickCount*_eraseTickMsec) / (float)_eraseTotalMsec);
}

void FirmwareUpgradeController::doFirmwareUpgrade(void)
{
    Q_ASSERT(_upgradeButton);
    _upgradeButton->setEnabled(false);
    
    _findBoard();
}

/// Appends the specified text to the status log area in the ui
void FirmwareUpgradeController::_appendStatusLog(const QString& text)
{
    Q_ASSERT(_statusLog);
    
    QVariant returnedValue;
    QVariant varText = text;
    QMetaObject::invokeMethod(_statusLog,
                              "append",
                              Q_RETURN_ARG(QVariant, returnedValue),
                              Q_ARG(QVariant, varText));
}