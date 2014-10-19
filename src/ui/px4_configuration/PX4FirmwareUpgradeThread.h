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

#ifndef PX4FirmwareUpgradeThread_H
#define PX4FirmwareUpgradeThread_H

#include <QObject>
#include <QSerialPort>
#include <QThread>
#include <QTimer>
#include <QTime>

#include "PX4Bootloader.h"

class PX4FirmwareUpgradeThreadWorker : public QObject
{
    Q_OBJECT
    
public:
    PX4FirmwareUpgradeThreadWorker(QObject* parent = NULL);
    
public slots:
    void init(void);
    void findBoard(int msecTimeout);
    void findBootloader(const QString portName, int msecTimeout);
    void cancelFind(void);
    void sendFMUReboot(const QString portName);
    void program(const QString firmwareFilename);
    
signals:
    void foundBoard(const QString portname, QString portDescription);
    void foundBootloader(int bootloaderVersion, int boardID, int flashSize);
    void bootloaderSyncFailed(void);
    void findTimeout(void);
    void rebootComplete(void);
    void updateProgress(int curr, int total);
    void upgradeComplete(void);
    
private slots:
    void _findBoardOnce(void);
    void _findBootloaderOnce(void);
    void _updateProgramProgress(int curr, int total) { emit updateProgress(curr, total); }
    
private:
    PX4Bootloader*  _bootloader;
    bool            _cancelFind;
    QTimer*         _timerTimeout;
    QTimer*         _timerRetry;
    QTime           _elapsed;
    QString         _portName;
};

class PX4FirmwareUpgradeThreadController : public QObject
{
    Q_OBJECT
    
public:
    PX4FirmwareUpgradeThreadController(QObject* parent = NULL);
    ~PX4FirmwareUpgradeThreadController(void);
    
    void findBoard(int msecTimeout);
    void findBootloader(const QString& portName, int msecTimeout);
    void cancelFind(void);
    void sendFMUReboot(const QString portName) { emit _sendFMURebootOnThread(portName); }
    void program(const QString firmwareFilename);

signals:
    void foundBoard(const QString portname, QString portDescription);
    void foundBootloader(int bootloaderVersion, int boardID, int flashSize);
    void bootloaderSyncFailed(void);
    void findTimeout(void);
    void rebootComplete();
    void updateProgress(int curr, int total);
    void upgradeComplete();
    
    void _initThreadWorker(void);
    void _findBoardOnThread(int msecTimeout);
    void _findBootloaderOnThread(const QString& portName, int msecTimeout);
    void _sendFMURebootOnThread(const QString portName);
    void _programOnThread(const QString firmwareFilename);
    
private slots:
    void _foundBoard(const QString portname, QString portDescription);
    void _foundBootloader(int bootloaderVersion, int boardID, int flashSize);
    void _bootloaderSyncFailed(void);
    void _rebootComplete(void) { emit rebootComplete(); };
    void _findTimeout(void);
    void _updateProgress(int curr, int total) { emit updateProgress(curr, total); }
    void _upgradeComplete(void) { emit upgradeComplete(); }
    
private:
    PX4FirmwareUpgradeThreadWorker* _worker;
    QThread*                        _workerThread;
    bool                            _cancelFind;
};

#endif
