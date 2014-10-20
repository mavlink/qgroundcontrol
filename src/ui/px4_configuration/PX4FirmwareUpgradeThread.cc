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

#include <QTimer>
#include <QSerialPortInfo>
#include <QDebug>

PX4FirmwareUpgradeThreadWorker::PX4FirmwareUpgradeThreadWorker(QObject* parent) :
    QObject(parent),
    _bootloader(NULL),
    _timerTimeout(NULL),
    _timerRetry(NULL)   
{
    
}

void PX4FirmwareUpgradeThreadWorker::init(void)
{
    Q_ASSERT(_timerTimeout == NULL);
    _timerTimeout = new QTimer(this);
    Q_CHECK_PTR(_timerTimeout);
    connect(_timerTimeout, &QTimer::timeout, this, &PX4FirmwareUpgradeThreadWorker::cancelFind);
    _timerTimeout->setSingleShot(true);
    
    Q_ASSERT(_timerRetry == NULL);
    _timerRetry = new QTimer(this);
    Q_CHECK_PTR(_timerRetry);
    connect(_timerRetry, &QTimer::timeout, this, &PX4FirmwareUpgradeThreadWorker::_findBoardOnce);
    _timerRetry->setSingleShot(true);
    _timerRetry->setInterval(1000);
    
    Q_ASSERT(_bootloader == NULL);
    _bootloader = new PX4Bootloader(this);
    connect(_bootloader, &PX4Bootloader::updateProgramProgress, this, &PX4FirmwareUpgradeThreadWorker::_updateProgramProgress);
}

void PX4FirmwareUpgradeThreadWorker::findBoard(int msecTimeout)
{
    _cancelFind = false;
    _timerTimeout->start(msecTimeout);
    _elapsed.start();
    _findBoardOnce();
}

void PX4FirmwareUpgradeThreadWorker::_findBoardOnce(void)
{
    qDebug() << "_findBoardOnce";
    
    QString portName;
    QString portDescription;
    
    foreach (QSerialPortInfo info, QSerialPortInfo::availablePorts()) {
        if (!info.portName().isEmpty() && (info.description().contains("PX4") || info.vendorIdentifier() == 9900 /* 3DR */)) {
            
            qDebug() << "Found Board:";
            qDebug() << "\tport name:" << info.portName();
            qDebug() << "\tdescription:" << info.description();
            qDebug() << "\tsystem location:" << info.systemLocation();
            qDebug() << "\tvendor ID:" << info.vendorIdentifier();
            qDebug() << "\tproduct ID:" << info.productIdentifier();
            
            portName = info.portName();
            portDescription = info.description();
            
#ifdef Q_OS_WIN
            // Stupid windows fixes
            portName.prepend("\\\\.\\");
#endif
            
            _timerTimeout->stop();
            emit foundBoard(portName, portDescription);
            return;
        }
    }
    
    emit updateProgress(_elapsed.elapsed(), _timerTimeout->interval());
    
    if (_cancelFind) {
        _timerTimeout->stop();
        emit findTimeout();
    } else {
        _timerRetry->start();
    }
}

void PX4FirmwareUpgradeThreadWorker::findBootloader(const QString portName, int msecTimeout)
{
    _portName = portName;
    _cancelFind = false;
    _timerTimeout->start(msecTimeout);
    _elapsed.start();
    _findBootloaderOnce();
}

void PX4FirmwareUpgradeThreadWorker::_findBootloaderOnce(void)
{
    uint32_t bootloaderVersion, boardID, flashSize;
    
    if (_bootloader->open(_portName)) {
        if (_bootloader->sync()) {
            if (_bootloader->getBoardInfo(bootloaderVersion, boardID, flashSize)) {
                _timerTimeout->stop();
                emit foundBootloader(bootloaderVersion, boardID, flashSize);
                return;
            } else {
                // FIXME: Error handling
                Q_ASSERT(false);
            }
        } else {
            _bootloader->close();
            _timerTimeout->stop();
            emit bootloaderSyncFailed();
            return;
        }
    }
    
    emit updateProgress(_elapsed.elapsed(), _timerTimeout->interval());
    
    if (_cancelFind) {
        _bootloader->close();
        _timerTimeout->stop();
        emit findTimeout();
    } else {
        _timerRetry->start();
    }
}

void PX4FirmwareUpgradeThreadWorker::cancelFind(void)
{
    _timerTimeout->stop();
    _timerRetry->stop();
    _cancelFind = true;
}

void PX4FirmwareUpgradeThreadWorker::sendFMUReboot(const QString portName)
{
    _bootloader->sendFMUReboot(portName);
    emit rebootComplete();
}


void PX4FirmwareUpgradeThreadWorker::program(const QString firmwareFilename)
{
    if (!_bootloader->program(firmwareFilename)) {
        qDebug() << "Program failed: " << _bootloader->errorString();
        Q_ASSERT(false);
    } else {
        emit upgradeComplete();
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
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::foundBootloader, this, &PX4FirmwareUpgradeThreadController::_foundBootloader);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::bootloaderSyncFailed, this, &PX4FirmwareUpgradeThreadController::_bootloaderSyncFailed);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::rebootComplete, this, &PX4FirmwareUpgradeThreadController::_rebootComplete);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::findTimeout, this, &PX4FirmwareUpgradeThreadController::_findTimeout);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::updateProgress, this, &PX4FirmwareUpgradeThreadController::_updateProgress);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::upgradeComplete, this, &PX4FirmwareUpgradeThreadController::_upgradeComplete);

    connect(this, &PX4FirmwareUpgradeThreadController::_initThreadWorker, _worker, &PX4FirmwareUpgradeThreadWorker::init);
    connect(this, &PX4FirmwareUpgradeThreadController::_findBoardOnThread, _worker, &PX4FirmwareUpgradeThreadWorker::findBoard);
    connect(this, &PX4FirmwareUpgradeThreadController::_findBootloaderOnThread, _worker, &PX4FirmwareUpgradeThreadWorker::findBootloader);
    connect(this, &PX4FirmwareUpgradeThreadController::_sendFMURebootOnThread, _worker, &PX4FirmwareUpgradeThreadWorker::sendFMUReboot);
    connect(this, &PX4FirmwareUpgradeThreadController::_programOnThread, _worker, &PX4FirmwareUpgradeThreadWorker::program);
    
    _workerThread->start();
    
    emit _initThreadWorker();
}

PX4FirmwareUpgradeThreadController::~PX4FirmwareUpgradeThreadController()
{
    _workerThread->quit();
    _workerThread->wait();
}

void PX4FirmwareUpgradeThreadController::findBoard(int msecTimeout)
{
    emit _findBoardOnThread(msecTimeout);
}

void PX4FirmwareUpgradeThreadController::cancelFind(void)
{
    _worker->cancelFind();
}

void PX4FirmwareUpgradeThreadController::program(const QString firmwareFilename)
{
    emit _programOnThread(firmwareFilename);
}

void PX4FirmwareUpgradeThreadController::findBootloader(const QString& portName, int msecTimeout)
{
    emit _findBootloaderOnThread(portName, msecTimeout);
}

void PX4FirmwareUpgradeThreadController::_foundBoard(const QString portName, QString portDescription)
{
    emit foundBoard(portName, portDescription);
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
