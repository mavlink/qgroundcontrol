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
///     @brief PX4 Firmware Upgrade operations which occur on a seperate thread.
///     @author Don Gagne <don@thegagnes.com>

#include "PX4FirmwareUpgradeThread.h"
#include "Bootloader.h"
#include "QGCLoggingCategory.h"
#include "QGC.h"
#include "SerialPortIds.h"

#include <QTimer>
#include <QSerialPortInfo>
#include <QDebug>
#include <QSerialPort>

PX4FirmwareUpgradeThreadWorker::PX4FirmwareUpgradeThreadWorker(PX4FirmwareUpgradeThreadController* controller) :
    _controller(controller),
    _bootloader(NULL),
    _bootloaderPort(NULL),
    _timerRetry(NULL),
    _foundBoard(false),
    _findBoardFirstAttempt(true)
{
    Q_ASSERT(_controller);
    
    connect(_controller, &PX4FirmwareUpgradeThreadController::_initThreadWorker,            this, &PX4FirmwareUpgradeThreadWorker::_init);
    connect(_controller, &PX4FirmwareUpgradeThreadController::_startFindBoardLoopOnThread,  this, &PX4FirmwareUpgradeThreadWorker::_startFindBoardLoop);
    connect(_controller, &PX4FirmwareUpgradeThreadController::_flashOnThread,               this, &PX4FirmwareUpgradeThreadWorker::_flash);
    connect(_controller, &PX4FirmwareUpgradeThreadController::_rebootOnThread,              this, &PX4FirmwareUpgradeThreadWorker::_reboot);
    connect(_controller, &PX4FirmwareUpgradeThreadController::_cancel,                      this, &PX4FirmwareUpgradeThreadWorker::_cancel);
}

PX4FirmwareUpgradeThreadWorker::~PX4FirmwareUpgradeThreadWorker()
{
    if (_bootloaderPort) {
        // deleteLater so delete happens on correct thread
        _bootloaderPort->deleteLater();
    }
}

/// @brief Initializes the PX4FirmwareUpgradeThreadWorker with the various child objects which must be created
///         on the worker thread.
void PX4FirmwareUpgradeThreadWorker::_init(void)
{
    // We create the timers here so that they are on the right thread
    
    Q_ASSERT(_timerRetry == NULL);
    _timerRetry = new QTimer(this);
    Q_CHECK_PTR(_timerRetry);
    _timerRetry->setSingleShot(true);
    _timerRetry->setInterval(_retryTimeout);
    connect(_timerRetry, &QTimer::timeout, this, &PX4FirmwareUpgradeThreadWorker::_findBoardOnce);
    
    Q_ASSERT(_bootloader == NULL);
    _bootloader = new Bootloader(this);
    connect(_bootloader, &Bootloader::updateProgress, this, &PX4FirmwareUpgradeThreadWorker::_updateProgress);
}

void PX4FirmwareUpgradeThreadWorker::_cancel(void)
{
    if (_bootloaderPort) {
        _bootloaderPort->close();
        _bootloaderPort->deleteLater();
        _bootloaderPort = NULL;
    }
}

void PX4FirmwareUpgradeThreadWorker::_startFindBoardLoop(void)
{
    _foundBoard = false;
    _findBoardFirstAttempt = true;
    _findBoardOnce();
}

void PX4FirmwareUpgradeThreadWorker::_findBoardOnce(void)
{
    qCDebug(FirmwareUpgradeLog) << "_findBoardOnce";
    
    QSerialPortInfo                     portInfo;
    PX4FirmwareUpgradeFoundBoardType_t  boardType;
    
    if (_findBoardFromPorts(portInfo, boardType)) {
        if (!_foundBoard) {
            _foundBoard = true;
            _foundBoardPortInfo = portInfo;
            emit foundBoard(_findBoardFirstAttempt, portInfo, boardType);
            if (!_findBoardFirstAttempt) {
                if (boardType == FoundBoard3drRadio) {
                    _3drRadioForceBootloader(portInfo);
                    return;
                } else {
                    _findBootloader(portInfo, false /* radio mode */, true /* errorOnNotFound */);
                    return;
                }
            }
        }
    } else {
        if (_foundBoard) {
            _foundBoard = false;
            qCDebug(FirmwareUpgradeLog) << "Board gone";
            emit boardGone();
        } else if (_findBoardFirstAttempt) {
            emit noBoardFound();
        }
    }
    
    _findBoardFirstAttempt = false;
    _timerRetry->start();
}

bool PX4FirmwareUpgradeThreadWorker::_findBoardFromPorts(QSerialPortInfo& portInfo, PX4FirmwareUpgradeFoundBoardType_t& type)
{
    bool found = false;
    
    foreach (QSerialPortInfo info, QSerialPortInfo::availablePorts()) {
        qCDebug(FirmwareUpgradeLog) << "Serial Port --------------";
        qCDebug(FirmwareUpgradeLog) << "\tport name:" << info.portName();
        qCDebug(FirmwareUpgradeLog) << "\tdescription:" << info.description();
        qCDebug(FirmwareUpgradeLog) << "\tsystem location:" << info.systemLocation();
        qCDebug(FirmwareUpgradeLog) << "\tvendor ID:" << info.vendorIdentifier();
        qCDebug(FirmwareUpgradeLog) << "\tproduct ID:" << info.productIdentifier();
        
        if (!info.portName().isEmpty()) {
            switch (info.vendorIdentifier()) {
                case SerialPortIds::px4VendorId:
                    if (info.productIdentifier() == SerialPortIds::pixhawkFMUV2ProductId || info.productIdentifier() == SerialPortIds::pixhawkFMUV2OldBootloaderProductId) {
                        qCDebug(FirmwareUpgradeLog) << "Found PX4 FMU V2";
                        type = FoundBoardPX4FMUV2;
                        found = true;
                    } else if (info.productIdentifier() == SerialPortIds::pixhawkFMUV1ProductId) {
                        qCDebug(FirmwareUpgradeLog) << "Found PX4 FMU V1";
                        type = FoundBoardPX4FMUV1;
                        found = true;
                    } else if (info.productIdentifier() == SerialPortIds::px4FlowProductId) {
                        qCDebug(FirmwareUpgradeLog) << "Found PX4 Flow";
                        type = FoundBoardPX4Flow;
                        found = true;
                    } else if (info.productIdentifier() == SerialPortIds::AeroCoreProductId) {
                        qCDebug(FirmwareUpgradeLog) << "Found AeroCore";
                        type = FoundBoardAeroCore;
                        found = true;
                    }
                    break;
                case SerialPortIds::threeDRRadioVendorId:
                    if (info.productIdentifier() == SerialPortIds::threeDRRadioProductId) {
                        qCDebug(FirmwareUpgradeLog) << "Found 3DR Radio";
                        type = FoundBoard3drRadio;
                        found = true;
                    }
                    break;
            }
            if (!found) {
                // Fall back to port name matching which could lead to incorrect board mapping. But in some cases the
                // vendor and product id do not come through correctly so this is used as a last chance detection method.
                if (info.description() == "PX4 FMU v2.x" || info.description() == "PX4 BL FMU v2.x") {
                    qCDebug(FirmwareUpgradeLog) << "Found PX4 FMU V2 (by name matching fallback)";
                    type = FoundBoardPX4FMUV2;
                    found = true;
                } else if (info.description() == "PX4 FMU v1.x" || info.description() == "PX4 BL FMU v1.x") {
                    qCDebug(FirmwareUpgradeLog) << "Found PX4 FMU V1 (by name matching fallback)";
                    type = FoundBoardPX4FMUV1;
                    found = true;
                }
            }
        }
        
        if (found) {
            portInfo = info;
            return true;
        }
    }
    
    return false;
}

void PX4FirmwareUpgradeThreadWorker::_3drRadioForceBootloader(const QSerialPortInfo& portInfo)
{
    // First make sure we can't get the bootloader
    
    if (_findBootloader(portInfo, true /* radio Mode */, false /* errorOnNotFound */)) {
        return;
    }

    // Couldn't find the bootloader. We'll need to reboot the radio into bootloader.
    
    QSerialPort port(portInfo);
    
    port.setBaudRate(QSerialPort::Baud57600);
    
    emit status("Putting radio into command mode");
    
    // Wait a little while for the USB port to initialize. 3DR Radio boot is really slow.
    for (int i=0; i<12; i++) {
        if (port.open(QIODevice::ReadWrite)) {
            break;
        } else {
            QGC::SLEEP::msleep(250);
        }
    }
    
    if (!port.isOpen()) {
        emit error(QString("Unable to open port: %1 error: %2").arg(portInfo.systemLocation()).arg(port.errorString()));
        return;
    }

    // Put radio into command mode
    port.write("+++", 3);
    if (!port.waitForReadyRead(1500)) {
        emit error("Unable to put radio into command mode");
        return;
    }
    QByteArray bytes = port.readAll();
    if (!bytes.contains("OK")) {
        emit error("Unable to put radio into command mode");
        return;
    }

    emit status("Rebooting radio to bootloader");
    
    port.write("AT&UPDATE\r\n");
    if (!port.waitForBytesWritten(1500)) {
        emit error("Unable to reboot radio");
        return;
    }
    QGC::SLEEP::msleep(2000);
    port.close();
    
    // The bootloader should be waiting for us now
    
    _findBootloader(portInfo, true /* radio mode */, true /* errorOnNotFound */);
}

bool PX4FirmwareUpgradeThreadWorker::_findBootloader(const QSerialPortInfo& portInfo, bool radioMode, bool errorOnNotFound)
{
    qCDebug(FirmwareUpgradeLog) << "_findBootloader";
    
    uint32_t bootloaderVersion = 0;
    uint32_t boardID;
    uint32_t flashSize = 0;

    
    _bootloaderPort = new QextSerialPort(QextSerialPort::Polling);
    Q_CHECK_PTR(_bootloaderPort);
    
    // Wait a little while for the USB port to initialize.
    for (int i=0; i<10; i++) {
        if (_bootloader->open(_bootloaderPort, portInfo.systemLocation())) {
            break;
        } else {
            QGC::SLEEP::msleep(100);
        }
    }
    
    if (_bootloader->sync(_bootloaderPort)) {
        bool success;
        
        if (radioMode) {
            success = _bootloader->get3DRRadioBoardId(_bootloaderPort, boardID);
        } else {
            success = _bootloader->getPX4BoardInfo(_bootloaderPort, bootloaderVersion, boardID, flashSize);
        }
        if (success) {
            qCDebug(FirmwareUpgradeLog) << "Found bootloader";
            emit foundBootloader(bootloaderVersion, boardID, flashSize);
            return true;
        }
    }
    
    _bootloaderPort->close();
    _bootloaderPort->deleteLater();
    _bootloaderPort = NULL;
    qCDebug(FirmwareUpgradeLog) << "Bootloader error:" << _bootloader->errorString();
    if (errorOnNotFound) {
        emit error(_bootloader->errorString());
    }
    
    return false;
}

void PX4FirmwareUpgradeThreadWorker::_reboot(void)
{
    if (_bootloaderPort) {
        if (_bootloaderPort->isOpen()) {
            _bootloader->reboot(_bootloaderPort);
        }
        _bootloaderPort->deleteLater();
        _bootloaderPort = NULL;
    }
}

void PX4FirmwareUpgradeThreadWorker::_flash(void)
{
    qCDebug(FirmwareUpgradeLog) << "PX4FirmwareUpgradeThreadWorker::_flash";
    
    if (_erase()) {
        emit status("Programming new version...");
        
        if (_bootloader->program(_bootloaderPort, _controller->image())) {
            qCDebug(FirmwareUpgradeLog) << "Program complete";
            emit status("Program complete");
        } else {
            _bootloaderPort->deleteLater();
            _bootloaderPort = NULL;
            qCDebug(FirmwareUpgradeLog) << "Program failed:" << _bootloader->errorString();
            emit error(_bootloader->errorString());
        }
        
        emit status("Verifying program...");
        
        if (_bootloader->verify(_bootloaderPort, _controller->image())) {
            qCDebug(FirmwareUpgradeLog) << "Verify complete";
            emit status("Verify complete");
        } else {
            qCDebug(FirmwareUpgradeLog) << "Verify failed:" << _bootloader->errorString();
            emit error(_bootloader->errorString());
        }
    }
    
    emit _reboot();
    
    emit flashComplete();
}

bool PX4FirmwareUpgradeThreadWorker::_erase(void)
{
    qCDebug(FirmwareUpgradeLog) << "PX4FirmwareUpgradeThreadWorker::_erase";
    
    emit eraseStarted();
    emit status("Erasing previous program...");
    
    if (_bootloader->erase(_bootloaderPort)) {
        qCDebug(FirmwareUpgradeLog) << "Erase complete";
        emit status("Erase complete");
        emit eraseComplete();
        return true;
    } else {
        qCDebug(FirmwareUpgradeLog) << "Erase failed:" << _bootloader->errorString();
        emit error(_bootloader->errorString());
        return false;
    }
}

PX4FirmwareUpgradeThreadController::PX4FirmwareUpgradeThreadController(QObject* parent) :
    QObject(parent)
{
    _worker = new PX4FirmwareUpgradeThreadWorker(this);
    Q_CHECK_PTR(_worker);
    
    _workerThread = new QThread(this);
    Q_CHECK_PTR(_workerThread);
    _worker->moveToThread(_workerThread);
    
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::updateProgress,       this, &PX4FirmwareUpgradeThreadController::_updateProgress);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::foundBoard,           this, &PX4FirmwareUpgradeThreadController::_foundBoard);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::noBoardFound,         this, &PX4FirmwareUpgradeThreadController::_noBoardFound);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::boardGone,            this, &PX4FirmwareUpgradeThreadController::_boardGone);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::foundBootloader,      this, &PX4FirmwareUpgradeThreadController::_foundBootloader);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::bootloaderSyncFailed, this, &PX4FirmwareUpgradeThreadController::_bootloaderSyncFailed);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::error,                this, &PX4FirmwareUpgradeThreadController::_error);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::status,               this, &PX4FirmwareUpgradeThreadController::_status);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::eraseStarted,         this, &PX4FirmwareUpgradeThreadController::_eraseStarted);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::eraseComplete,        this, &PX4FirmwareUpgradeThreadController::_eraseComplete);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::flashComplete,        this, &PX4FirmwareUpgradeThreadController::_flashComplete);

    _workerThread->start();
    
    emit _initThreadWorker();
}

PX4FirmwareUpgradeThreadController::~PX4FirmwareUpgradeThreadController()
{
    _workerThread->quit();
    _workerThread->wait();
    
    delete _workerThread;
}

void PX4FirmwareUpgradeThreadController::startFindBoardLoop(void)
{
    qCDebug(FirmwareUpgradeLog) << "PX4FirmwareUpgradeThreadController::findBoard";
    emit _startFindBoardLoopOnThread();
}

void PX4FirmwareUpgradeThreadController::cancel(void)
{
    qCDebug(FirmwareUpgradeLog) << "PX4FirmwareUpgradeThreadController::cancel";
    emit _cancel();
}

void PX4FirmwareUpgradeThreadController::flash(const FirmwareImage* image)
{
    _image = image;
    emit _flashOnThread();
}
