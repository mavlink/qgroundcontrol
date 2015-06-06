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
    _statusLog(NULL)
{
    _threadController = new PX4FirmwareUpgradeThreadController(this);
    Q_CHECK_PTR(_threadController);

    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBoard, this, &FirmwareUpgradeController::_foundBoard);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::noBoardFound, this, &FirmwareUpgradeController::_noBoardFound);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::boardGone, this, &FirmwareUpgradeController::_boardGone);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBootloader, this, &FirmwareUpgradeController::_foundBootloader);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::bootloaderSyncFailed, this, &FirmwareUpgradeController::_bootloaderSyncFailed);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::error, this, &FirmwareUpgradeController::_error);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::status, this, &FirmwareUpgradeController::_status);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::flashComplete, this, &FirmwareUpgradeController::_flashComplete);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::updateProgress, this, &FirmwareUpgradeController::_updateProgress);
    
    connect(LinkManager::instance(), &LinkManager::linkDisconnected, this, &FirmwareUpgradeController::_linkDisconnected);
    
    connect(&_eraseTimer, &QTimer::timeout, this, &FirmwareUpgradeController::_eraseProgressTick);
}

void FirmwareUpgradeController::startBoardSearch(void)
{
    _bootloaderFound = true;
    _startFlashWhenBootloaderFound = false;
    _threadController->startFindBoardLoop();
}

void FirmwareUpgradeController::flash(FirmwareType_t firmwareType)
{
    _getFirmwareFile(firmwareType);
}

void FirmwareUpgradeController::cancel(void)
{
    // FIXME: Needs to cancel any controller operations
    _appendStatusLog("Firmware upgrade cancelled");
    _appendStatusLog("***CANCEL NYI***");
    _threadController->cancel();
}

void FirmwareUpgradeController::_foundBoard(bool firstAttempt, const QSerialPortInfo& info, int type)
{
    _foundBoardInfo = info;
    switch (type) {
        case FoundBoardPX4FMUV1:
            _foundBoardType = "PX4 FMU V1";
            _startFlashWhenBootloaderFound = false;
            break;
        case FoundBoardPX4FMUV2:
            _foundBoardType = "Pixhawk";
            _startFlashWhenBootloaderFound = false;
            break;
        case FoundBoardPX4Flow:
        case FoundBoard3drRadio:
            _foundBoardType = type == FoundBoardPX4Flow ? "PX4 Flow" : "3DR Radio";
            if (!firstAttempt) {
                // PX4 Flow and Radio always flash stable firmware, so we can start right away without
                // any further user input.
                _startFlashWhenBootloaderFound = true;
                _startFlashWhenBootloaderFoundFirmwareType = PX4StableFirmware;
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
    
    _appendStatusLog(tr("Connected to bootloader:"));
    _appendStatusLog(tr("  Version: %1").arg(_bootloaderVersion));
    _appendStatusLog(tr("  Board ID: %1").arg(_bootloaderBoardID));
    _appendStatusLog(tr("  Flash size: %1").arg(_bootloaderBoardFlashSize));
    
    if (_startFlashWhenBootloaderFound) {
        flash(_startFlashWhenBootloaderFoundFirmwareType);
    }
}

/// @brief Called when the findBootloader process is unable to sync to the bootloader. Moves the state
///         machine to the appropriate error state.
void FirmwareUpgradeController::_bootloaderSyncFailed(void)
{
    _appendStatusLog(tr("Unable to sync with bootloader."), true);
    cancel();
}

/// @brief Prompts the user to select a firmware file if needed and moves the state machine to the next state.
void FirmwareUpgradeController::_getFirmwareFile(FirmwareType_t firmwareType)
{
    static DownloadLocationByFirmwareType_t rgPX4FMUV2Firmware[] = {
        { PX4StableFirmware,            "http://px4-travis.s3.amazonaws.com/Firmware/stable/px4fmu-v2_default.px4" },
        { PX4BetaFirmware,              "http://px4-travis.s3.amazonaws.com/Firmware/beta/px4fmu-v2_default.px4" },
        { PX4DeveloperFirmware,         "http://px4-travis.s3.amazonaws.com/Firmware/master/px4fmu-v2_default.px4"},
        { ApmArduCopterQuadFirmware,    "http://firmware.diydrones.com/Copter/stable/PX4-quad/ArduCopter-v2.px4" },
        { ApmArduCopterX8Firmware,      "http://firmware.diydrones.com/Copter/stable/PX4-octa-quad/ArduCopter-v2.px4" },
        { ApmArduCopterHexaFirmware,    "http://firmware.diydrones.com/Copter/stable/PX4-hexa/ArduCopter-v2.px4" },
        { ApmArduCopterOctoFirmware,    "http://firmware.diydrones.com/Copter/stable/PX4-octa/ArduCopter-v2.px4" },
        { ApmArduCopterYFirmware,       "http://firmware.diydrones.com/Copter/stable/PX4-tri/ArduCopter-v2.px4" },
        { ApmArduCopterY6Firmware,      "http://firmware.diydrones.com/Copter/stable/PX4-y6/ArduCopter-v2.px4" },
        { ApmArduCopterHeliFirmware,    "http://firmware.diydrones.com/Copter/stable/PX4-heli/ArduCopter-v2.px4" },
        { ApmArduPlaneFirmware,         "http://firmware.diydrones.com/Plane/stable/PX4/ArduPlane-v2.px4" },
        { ApmRoverFirmware,             "http://firmware.diydrones.com/Plane/stable/PX4/APMrover2-v2.px4" },
    };
    static const size_t crgPX4FMUV2Firmware = sizeof(rgPX4FMUV2Firmware) / sizeof(rgPX4FMUV2Firmware[0]);

    static const DownloadLocationByFirmwareType_t rgAeroCoreFirmware[] = {
        { PX4StableFirmware,    "http://s3-us-west-2.amazonaws.com/gumstix-aerocore/PX4/stable/aerocore_default.px4" },
        { PX4BetaFirmware,      "http://s3-us-west-2.amazonaws.com/gumstix-aerocore/PX4/beta/aerocore_default.px4" },
        { PX4DeveloperFirmware, "http://s3-us-west-2.amazonaws.com/gumstix-aerocore/PX4/master/aerocore_default.px4" },
    };
    static const size_t crgAeroCoreFirmware = sizeof(rgAeroCoreFirmware) / sizeof(rgAeroCoreFirmware[0]);

    static const DownloadLocationByFirmwareType_t rgPX4FMUV1Firmware[] = {
        { PX4StableFirmware,    "http://px4-travis.s3.amazonaws.com/Firmware/stable/px4fmu-v1_default.px4" },
        { PX4BetaFirmware,      "http://px4-travis.s3.amazonaws.com/Firmware/beta/px4fmu-v1_default.px4" },
        { PX4DeveloperFirmware, "http://px4-travis.s3.amazonaws.com/Firmware/master/px4fmu-v1_default.px4" },
    };
    static const size_t crgPX4FMUV1Firmware = sizeof(rgPX4FMUV1Firmware) / sizeof(rgPX4FMUV1Firmware[0]);
    
    static const DownloadLocationByFirmwareType_t rgPX4FlowFirmware[] = {
        { PX4StableFirmware, "http://px4-travis.s3.amazonaws.com/Flow/master/px4flow.px4" },
    };
    static const size_t crgPX4FlowFirmware = sizeof(rgPX4FlowFirmware) / sizeof(rgPX4FlowFirmware[0]);
    
    static const DownloadLocationByFirmwareType_t rg3DRRadioFirmware[] = {
        { PX4StableFirmware, "http://firmware.diydrones.com/SiK/stable/radio~hm_trp.ihx" },
    };
    static const size_t crg3DRRadioFirmware = sizeof(rg3DRRadioFirmware) / sizeof(rg3DRRadioFirmware[0]);
    
    // Select the firmware set based on board type
    
    const DownloadLocationByFirmwareType_t* prgFirmware;
    size_t crgFirmware;
    
    switch (_bootloaderBoardID) {
        case _boardIDPX4FMUV1:
            prgFirmware = rgPX4FMUV1Firmware;
            crgFirmware = crgPX4FMUV1Firmware;
            break;
            
        case _boardIDPX4Flow:
            prgFirmware = rgPX4FlowFirmware;
            crgFirmware = crgPX4FlowFirmware;
            break;
            
        case _boardIDPX4FMUV2:
            prgFirmware = rgPX4FMUV2Firmware;
            crgFirmware = crgPX4FMUV2Firmware;
            break;

        case _boardIDAeroCore:
            prgFirmware = rgAeroCoreFirmware;
            crgFirmware = crgAeroCoreFirmware;
            break;

        case _boardID3DRRadio:
            prgFirmware = rg3DRRadioFirmware;
            crgFirmware = crg3DRRadioFirmware;
            break;

        default:
            prgFirmware = NULL;
            break;
    }

    if (prgFirmware == NULL && firmwareType != PX4CustomFirmware) {
            _appendStatusLog("Attempting to flash an unknown board type, you must select 'Custom firmware file'", true);
            cancel();
            return;
    }
    
    if (firmwareType == PX4CustomFirmware) {
        _firmwareFilename = QGCFileDialog::getOpenFileName(NULL,                                                                // Parent to main window
                                                           tr("Select Firmware File"),                                          // Dialog Caption
                                                           QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), // Initial directory
                                                           tr("Firmware Files (*.px4 *.bin)"));                                 // File filter
    } else {
        bool found = false;
        
        for (size_t i=0; i<crgFirmware; i++) {
            if (prgFirmware->firmwareType == firmwareType) {
                found = true;
                break;
            }
            prgFirmware++;
        }
        
        if (found) {
            _firmwareFilename = prgFirmware->downloadLocation;
        } else {
            _appendStatusLog("Unable to find specified firmware download location", true);
            cancel();
            return;
        }
    }
    
    if (_firmwareFilename.isEmpty()) {
        _appendStatusLog("No firmware file selected");
        cancel();
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
            cancel();
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
        cancel();
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
            cancel();
            return;
        }
        
        QJsonObject px4Json = doc.object();

        // Make sure the keys we need are available
        static const char* rgJsonKeys[] = { "board_id", "image_size", "description", "git_identity" };
        for (size_t i=0; i<sizeof(rgJsonKeys)/sizeof(rgJsonKeys[0]); i++) {
            if (!px4Json.contains(rgJsonKeys[i])) {
                _appendStatusLog(tr("Incorrectly formatted firmware file. No %1 key.").arg(rgJsonKeys[i]));
                cancel();
                return;
            }
        }
        
        uint32_t firmwareBoardID = (uint32_t)px4Json.value(QString("board_id")).toInt();
        if (firmwareBoardID != _bootloaderBoardID) {
            _appendStatusLog(tr("Downloaded firmware board id does not match hardware board id: %1 != %2").arg(firmwareBoardID).arg(_bootloaderBoardID));
            cancel();
            return;
        }
        
        // Decompress the parameter xml and save to file
        QByteArray decompressedBytes;
        bool success = _decompressJsonValue(px4Json,               // JSON object
                                            bytes,                 // Raw bytes of JSON document
                                            "parameter_xml_size",  // key which holds byte size
                                            "parameter_xml",       // key which holds compress bytes
                                            decompressedBytes);    // Returned decompressed bytes
        if (success) {
            QSettings settings;
            QDir parameterDir = QFileInfo(settings.fileName()).dir();
            QString parameterFilename = parameterDir.filePath("PX4ParameterFactMetaData.xml");
			qDebug() << parameterFilename;
            QFile parameterFile(parameterFilename);
            if (parameterFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                qint64 bytesWritten = parameterFile.write(decompressedBytes);
                if (bytesWritten != decompressedBytes.count()) {
                    _appendStatusLog(tr("Write failed for parameter meta data file, error: %1").arg(parameterFile.errorString()));
                    parameterFile.close();
                    QFile::remove(parameterFilename);
                } else {
                    parameterFile.close();
                }
            } else {
                _appendStatusLog(tr("Unable to open parameter meta data file %1 for writing, error: %2").arg(parameterFilename).arg(parameterFile.errorString()));
            }
            
        }
        
        // FIXME: Save NYI
        
        // Decompress the image and save to file
        _imageSize = px4Json.value(QString("image_size")).toInt();
		success = _decompressJsonValue(px4Json,               // JSON object
                                       bytes,                 // Raw bytes of JSON document
                                       "image_size",          // key which holds byte size
                                       "image",               // key which holds compress bytes
                                       decompressedBytes);    // Returned decompressed bytes
        if (!success) {
            cancel();
            return;
        }

        // Pad image to 4-byte boundary
        while ((decompressedBytes.count() % 4) != 0) {
            decompressedBytes.append(static_cast<char>(static_cast<unsigned char>(0xFF)));
        }
        
        // Store decompressed image file in same location as original download file
        QDir downloadDir = QFileInfo(downloadFilename).dir();
        QString decompressFilename = downloadDir.filePath("PX4FlashUpgrade.bin");
        
        QFile decompressFile(decompressFilename);
        if (!decompressFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            _appendStatusLog(tr("Unable to open decompressed file %1 for writing, error: %2").arg(decompressFilename).arg(decompressFile.errorString()));
            cancel();
            return;
        }
        
        qint64 bytesWritten = decompressFile.write(decompressedBytes);
        if (bytesWritten != decompressedBytes.count()) {
            _appendStatusLog(tr("Write failed for decompressed image file, error: %1").arg(decompressFile.errorString()));
            cancel();
            return;
        }
        decompressFile.close();
        
        _firmwareFilename = decompressFilename;
    } else {
        uint32_t firmwareBoardID = 0;
        
        // Take some educated guesses on board id based on firmware build system file name conventions
        
        if (downloadFilename.toLower().contains("px4fmu-v1")) {
            firmwareBoardID = _boardIDPX4FMUV2;
        } else if (downloadFilename.toLower().contains("px4flow")) {
            firmwareBoardID = _boardIDPX4Flow;
        } else if (downloadFilename.toLower().contains("px4fmu-v1")) {
            firmwareBoardID = _boardIDPX4FMUV1;
        } else if (downloadFilename.toLower().contains("aerocore")) {
            firmwareBoardID = _boardIDAeroCore;
        } else if (downloadFilename.toLower().contains("radio")) {
            firmwareBoardID = _boardID3DRRadio;
        }
        
        if (firmwareBoardID != 0 &&  firmwareBoardID != _bootloaderBoardID) {
            _appendStatusLog(tr("Downloaded firmware board id does not match hardware board id: %1 != %2").arg(firmwareBoardID).arg(_bootloaderBoardID));
            cancel();
            return;
        }
        
        _firmwareFilename = downloadFilename;
        
        QFile binFile(_firmwareFilename);
        if (!binFile.open(QIODevice::ReadOnly)) {
            _appendStatusLog(tr("Unabled to open firmware file %1, %2").arg(_firmwareFilename).arg(binFile.errorString()));
            cancel();
            return;
        }
        _imageSize = (uint32_t)binFile.size();
        binFile.close();
    }
    
    // We can't proceed unless we have the bootloader
    if (!_bootloaderFound) {
        _appendStatusLog(tr("Bootloader not found").arg(_imageSize).arg(_bootloaderBoardFlashSize), true);
        cancel();
        return;
    }
    
    if (_bootloaderBoardFlashSize != 0 && _imageSize > _bootloaderBoardFlashSize) {
        _appendStatusLog(tr("Image size of %1 is too large for board flash size %2").arg(_imageSize).arg(_bootloaderBoardFlashSize), true);
        cancel();
        return;
    }

    _threadController->flash(_firmwareFilename);
}

/// Decompress a set of bytes stored in a Json document.
bool FirmwareUpgradeController::_decompressJsonValue(const QJsonObject&	jsonObject,			///< JSON object
													 const QByteArray&	jsonDocBytes,		///< Raw bytes of JSON document
													 const QString&		sizeKey,			///< key which holds byte size
													 const QString&		bytesKey,			///< key which holds compress bytes
													 QByteArray&		decompressedBytes)	///< Returned decompressed bytes
{
    // Validate decompressed size key
    if (!jsonObject.contains(sizeKey)) {
        _appendStatusLog(QString("Firmware file missing %1 key").arg(sizeKey), true);
        return false;
    }
    int decompressedSize = jsonObject.value(QString(sizeKey)).toInt();
    if (decompressedSize == 0) {
        _appendStatusLog(QString("Firmware file has invalid decompressed size for %1").arg(sizeKey), true);
        return false;
    }
    
	// XXX Qt's JSON string handling is terribly broken, strings
	// with some length (18K / 25K) are just weirdly cut.
	// The code below works around this by manually 'parsing'
	// for the image string. Since its compressed / checksummed
	// this should be fine.
	
	QStringList parts = QString(jsonDocBytes).split(QString("\"%1\": \"").arg(bytesKey));
    if (parts.count() == 1) {
        _appendStatusLog(QString("Could not find compressed bytes for %1 in Firmware file").arg(bytesKey), true);
        return false;
    }
	parts = parts.last().split("\"");
    if (parts.count() == 1) {
        _appendStatusLog(QString("Incorrectly formed compressed bytes section for %1 in Firmware file").arg(bytesKey), true);
        return false;
    }
	
    // Store decompressed size as first four bytes. This is required by qUncompress routine.
	QByteArray raw;
	raw.append((unsigned char)((decompressedSize >> 24) & 0xFF));
	raw.append((unsigned char)((decompressedSize >> 16) & 0xFF));
	raw.append((unsigned char)((decompressedSize >> 8) & 0xFF));
	raw.append((unsigned char)((decompressedSize >> 0) & 0xFF));
	
	QByteArray raw64 = parts.first().toUtf8();
	raw.append(QByteArray::fromBase64(raw64));
	decompressedBytes = qUncompress(raw);
    
    if (decompressedBytes.count() == 0) {
        _appendStatusLog(QString("Firmware file has 0 length %1").arg(bytesKey), true);
        return false;
    }
    if (decompressedBytes.count() != decompressedSize) {
        _appendStatusLog(QString("Size for decompressed %1 does not match stored size: Expected(%1) Actual(%2)").arg(decompressedSize).arg(decompressedBytes.count()), true);
        return false;
    }
    
    _appendStatusLog(QString("Succesfully decompressed %1").arg(bytesKey));
    return true;
}

/// @brief Called when an error occurs during download
void FirmwareUpgradeController::_downloadError(QNetworkReply::NetworkError code)
{
    if (code == QNetworkReply::OperationCanceledError) {
        _appendStatusLog(tr("Download cancelled"));
    } else {
        _appendStatusLog(tr("Error during download. Error: %1").arg(code), true);
    }
    cancel();
}

/// @brief Signals completion of one of the specified bootloader commands. Moves the state machine to the
///         appropriate next step.
void FirmwareUpgradeController::_flashComplete(void)
{
    _appendStatusLog("Upgrade complete", true);
    QGCMessageBox::information("Firmware Upgrade", "Upgrade completed succesfully");
}

void FirmwareUpgradeController::_error(const QString& errorString)
{
    _appendStatusLog(tr("Error: %1").arg(errorString), true);
    emit error();
    cancel();
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
    return LinkManager::instance()->anyConnectedLinks();
}

void FirmwareUpgradeController::_linkDisconnected(LinkInterface* link)
{
    Q_UNUSED(link);
    emit qgcConnectionsChanged(qgcConnections());
}

