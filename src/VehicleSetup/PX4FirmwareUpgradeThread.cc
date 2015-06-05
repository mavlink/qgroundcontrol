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
#include "PX4Bootloader.h"
#include "QGCLoggingCategory.h"

#include <QTimer>
#include <QSerialPortInfo>
#include <QDebug>

PX4FirmwareUpgradeThreadWorker::PX4FirmwareUpgradeThreadWorker(QObject* parent) :
    QObject(parent),
    _bootloader(NULL),
    _bootloaderPort(NULL),
    _timerTimeout(NULL),
    _timerRetry(NULL),
    _foundBoard(false),
    _findBoardFirstAttempt(true)
{
    
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
void PX4FirmwareUpgradeThreadWorker::init(void)
{
    // We create the timers here so that they are on the right thread
    
    Q_ASSERT(_timerTimeout == NULL);
    _timerTimeout = new QTimer(this);
    Q_CHECK_PTR(_timerTimeout);
    connect(_timerTimeout, &QTimer::timeout, this, &PX4FirmwareUpgradeThreadWorker::timeout);
    _timerTimeout->setSingleShot(true);
    
    Q_ASSERT(_timerRetry == NULL);
    _timerRetry = new QTimer(this);
    Q_CHECK_PTR(_timerRetry);
    _timerRetry->setSingleShot(true);
    _timerRetry->setInterval(_retryTimeout);
    
    Q_ASSERT(_bootloader == NULL);
    _bootloader = new PX4Bootloader(this);
    connect(_bootloader, &PX4Bootloader::updateProgramProgress, this, &PX4FirmwareUpgradeThreadWorker::_updateProgramProgress);
}

void PX4FirmwareUpgradeThreadWorker::startFindBoardLoop(void)
{
    _foundBoard = false;
    _findBoardFirstAttempt = true;
    connect(_timerRetry, &QTimer::timeout, this, &PX4FirmwareUpgradeThreadWorker::_findBoardOnce);
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

bool PX4FirmwareUpgradeThreadWorker::findBoardFromPorts(QSerialPortInfo& portInfo, PX4FirmwareUpgradeFoundBoardType_t& type)
{
    bool found = false;
    
    foreach (QSerialPortInfo info, QSerialPortInfo::availablePorts()) {
#if 0
        qCDebug(FirmwareUpgradeLog) << "Serial Port --------------";
        qCDebug(FirmwareUpgradeLog) << "\tport name:" << info.portName();
        qCDebug(FirmwareUpgradeLog) << "\tdescription:" << info.description();
        qCDebug(FirmwareUpgradeLog) << "\tsystem location:" << info.systemLocation();
        qCDebug(FirmwareUpgradeLog) << "\tvendor ID:" << info.vendorIdentifier();
        qCDebug(FirmwareUpgradeLog) << "\tproduct ID:" << info.productIdentifier();
#endif
        
        if (!info.portName().isEmpty()) {
            if (info.vendorIdentifier() == _pixhawkVendorId) {
                if (info.productIdentifier() == _pixhawkFMUV2ProductId) {
                    qCDebug(FirmwareUpgradeLog) << "Found PX4 FMU V2";
                    type = FoundBoardPX4FMUV1;
                    found = true;
                } else if (info.productIdentifier() == _pixhawkFMUV1ProductId) {
                    qCDebug(FirmwareUpgradeLog) << "Found PX4 FMU V1";
                    type = FoundBoardPX4FMUV2;
                    found = true;
                }
            } else if (info.vendorIdentifier() == _flowVendorId && info.productIdentifier() == _flowProductId) {
                qCDebug(FirmwareUpgradeLog) << "Found PX4 Flow";
                type = FoundBoardPX4Flow;
                found = true;
            } else if (info.vendorIdentifier() == _3drRadioVendorId && info.productIdentifier() == _3drRadioProductId) {
                qCDebug(FirmwareUpgradeLog) << "Found 3DR Radio";
                type = FoundBoard3drRadio;
                found = true;
            }
        }
        
        if (found) {
            portInfo = info;
            return true;
        }
    }
    
    return false;
}

void PX4FirmwareUpgradeThreadWorker::findBootloader(const QString portName, int msecTimeout)
{
    Q_UNUSED(msecTimeout);
    
    // Once the port shows up, we only try to connect to the bootloader a single time
    _portName = portName;
    _findBootloaderOnce();
}

void PX4FirmwareUpgradeThreadWorker::_findBootloaderOnce(void)
{
    qCDebug(FirmwareUpgradeLog) << "_findBootloaderOnce";
    
    uint32_t    bootloaderVersion, boardID, flashSize;

    _bootloaderPort = new QextSerialPort(QextSerialPort::Polling);
    Q_CHECK_PTR(_bootloaderPort);
    
    if (_bootloader->open(_bootloaderPort, _portName)) {
        if (_bootloader->sync(_bootloaderPort)) {
            if (_bootloader->getBoardInfo(_bootloaderPort, bootloaderVersion, boardID, flashSize)) {
                _closeFind();
                qCDebug(FirmwareUpgradeLog) << "Found bootloader";
                emit foundBootloader(bootloaderVersion, boardID, flashSize);
                return;
            }
        }
    }
    
    _closeFind();
    _bootloaderPort->close();
    _bootloaderPort->deleteLater();
    _bootloaderPort = NULL;
    qCDebug(FirmwareUpgradeLog) << "Bootloader error:" << _bootloader->errorString();
    emit error(commandBootloader, _bootloader->errorString());
}

void PX4FirmwareUpgradeThreadWorker::_closeFind(void)
{
    disconnect(_timerRetry, SIGNAL(timeout()), 0, 0);
    _timerRetry->stop();
    _timerTimeout->stop();
}

void PX4FirmwareUpgradeThreadWorker::cancelFind(void)
{
    _closeFind();
    emit complete(commandCancel);
}

void PX4FirmwareUpgradeThreadWorker::timeout(void)
{
    qCDebug(FirmwareUpgradeLog) << "Find timeout";
    _closeFind();
    emit findTimeout();
}

void PX4FirmwareUpgradeThreadWorker::sendBootloaderReboot(void)
{
    if (_bootloaderPort) {
        if (_bootloaderPort->isOpen()) {
            _bootloader->sendBootloaderReboot(_bootloaderPort);
        }
        _bootloaderPort->deleteLater();
        _bootloaderPort = NULL;
    }
}

void PX4FirmwareUpgradeThreadWorker::program(const QString firmwareFilename)
{
    qCDebug(FirmwareUpgradeLog) << "Program";
    if (!_bootloader->program(_bootloaderPort, firmwareFilename)) {
        _bootloaderPort->deleteLater();
        _bootloaderPort = NULL;
        qCDebug(FirmwareUpgradeLog) << "Program failed:" << _bootloader->errorString();
        emit error(commandProgram, _bootloader->errorString());
    } else {
        qCDebug(FirmwareUpgradeLog) << "Program complete";
        emit complete(commandProgram);
    }
}

void PX4FirmwareUpgradeThreadWorker::verify(const QString firmwareFilename)
{
    qCDebug(FirmwareUpgradeLog) << "Verify";
    if (!_bootloader->verify(_bootloaderPort, firmwareFilename)) {
        qCDebug(FirmwareUpgradeLog) << "Verify failed:" << _bootloader->errorString();
        emit error(commandVerify, _bootloader->errorString());
    } else {
        qCDebug(FirmwareUpgradeLog) << "Verify complete";
        emit complete(commandVerify);
    }
    _bootloaderPort->deleteLater();
    _bootloaderPort = NULL;
}

void PX4FirmwareUpgradeThreadWorker::erase(void)
{
    qCDebug(FirmwareUpgradeLog) << "Erase";
    if (!_bootloader->erase(_bootloaderPort)) {
        _bootloaderPort->deleteLater();
        _bootloaderPort = NULL;
        qCDebug(FirmwareUpgradeLog) << "Erase failed:" << _bootloader->errorString();
        emit error(commandErase, _bootloader->errorString());
    } else {
        qCDebug(FirmwareUpgradeLog) << "Erase complete";
        emit complete(commandErase);
    }
}

PX4FirmwareUpgradeThreadController::PX4FirmwareUpgradeThreadController(QObject* parent) :
    QObject(parent)
{
    _worker = new PX4FirmwareUpgradeThreadWorker();
    Q_CHECK_PTR(_worker);
    
    _workerThread = new QThread(this);
    Q_CHECK_PTR(_workerThread);
    _worker->moveToThread(_workerThread);
    
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::foundBoard, this, &PX4FirmwareUpgradeThreadController::_foundBoard);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::noBoardFound, this, &PX4FirmwareUpgradeThreadController::_noBoardFound);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::boardGone, this, &PX4FirmwareUpgradeThreadController::_boardGone);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::foundBootloader, this, &PX4FirmwareUpgradeThreadController::_foundBootloader);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::bootloaderSyncFailed, this, &PX4FirmwareUpgradeThreadController::_bootloaderSyncFailed);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::error, this, &PX4FirmwareUpgradeThreadController::_error);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::complete, this, &PX4FirmwareUpgradeThreadController::_complete);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::findTimeout, this, &PX4FirmwareUpgradeThreadController::_findTimeout);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::updateProgress, this, &PX4FirmwareUpgradeThreadController::_updateProgress);

    connect(this, &PX4FirmwareUpgradeThreadController::_initThreadWorker, _worker, &PX4FirmwareUpgradeThreadWorker::init);
    connect(this, &PX4FirmwareUpgradeThreadController::_startFindBoardLoopOnThread, _worker, &PX4FirmwareUpgradeThreadWorker::startFindBoardLoop);
    connect(this, &PX4FirmwareUpgradeThreadController::_findBootloaderOnThread, _worker, &PX4FirmwareUpgradeThreadWorker::findBootloader);
    connect(this, &PX4FirmwareUpgradeThreadController::_sendBootloaderRebootOnThread, _worker, &PX4FirmwareUpgradeThreadWorker::sendBootloaderReboot);
    connect(this, &PX4FirmwareUpgradeThreadController::_programOnThread, _worker, &PX4FirmwareUpgradeThreadWorker::program);
    connect(this, &PX4FirmwareUpgradeThreadController::_verifyOnThread, _worker, &PX4FirmwareUpgradeThreadWorker::verify);
    connect(this, &PX4FirmwareUpgradeThreadController::_eraseOnThread, _worker, &PX4FirmwareUpgradeThreadWorker::erase);
    connect(this, &PX4FirmwareUpgradeThreadController::_cancelFindOnThread, _worker, &PX4FirmwareUpgradeThreadWorker::cancelFind);
    
    _workerThread->start();
    
    emit _initThreadWorker();
}

PX4FirmwareUpgradeThreadController::~PX4FirmwareUpgradeThreadController()
{
    _workerThread->quit();
    _workerThread->wait();
}

void PX4FirmwareUpgradeThreadController::startFindBoardLoop(void)
{
    qCDebug(FirmwareUpgradeLog) << "PX4FirmwareUpgradeThreadController::findBoard";
    emit _startFindBoardLoopOnThread();
}

void PX4FirmwareUpgradeThreadController::findBootloader(const QString& portName, int msecTimeout)
{
    qCDebug(FirmwareUpgradeLog) << "PX4FirmwareUpgradeThreadController::findBootloader";
    emit _findBootloaderOnThread(portName, msecTimeout);
}

void PX4FirmwareUpgradeThreadController::_foundBoard(bool firstAttempt, const QSerialPortInfo& portInfo, int type)
{
    emit foundBoard(firstAttempt, portInfo, type);
}

void PX4FirmwareUpgradeThreadController::_boardGone(void)
{
    emit boardGone();
}

void PX4FirmwareUpgradeThreadController::_noBoardFound(void)
{
    emit noBoardFound();
}

void PX4FirmwareUpgradeThreadController::_foundBootloader(int bootloaderVersion, int boardID, int flashSize)
{
    emit foundBootloader(bootloaderVersion, boardID, flashSize);
}

void PX4FirmwareUpgradeThreadController::_bootloaderSyncFailed(void)
{    
    emit bootloaderSyncFailed();
}

void PX4FirmwareUpgradeThreadController::_findTimeout(void)
{
    emit findTimeout();
}
