/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief PX4 Firmware Upgrade operations which occur on a separate thread.
///     @author Don Gagne <don@thegagnes.com>

#ifndef PX4FirmwareUpgradeThread_H
#define PX4FirmwareUpgradeThread_H

#include "Bootloader.h"
#include "FirmwareImage.h"
#include "QGCSerialPortInfo.h"

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QTime>
#include <QSerialPort>

#include <stdint.h>

class PX4FirmwareUpgradeThreadController;

/// @brief Used to run bootloader commands on a separate thread. These routines are mainly meant to to be called
///         internally by the PX4FirmwareUpgradeThreadController. Clients should call the various public methods
///         exposed by PX4FirmwareUpgradeThreadController.
class PX4FirmwareUpgradeThreadWorker : public QObject
{
    Q_OBJECT
    
public:
    PX4FirmwareUpgradeThreadWorker(PX4FirmwareUpgradeThreadController* controller);
    ~PX4FirmwareUpgradeThreadWorker();
    
signals:
    void updateProgress         (int curr, int total);
    void foundBoard             (bool firstAttempt, const QGCSerialPortInfo& portInfo, int type, QString boardName);
    void noBoardFound           (void);
    void boardGone              (void);
    void foundBoardInfo         (int bootloaderVersion, int boardID, int flashSize);
    void error                  (const QString& errorString);
    void status                 (const QString& statusText);
    void eraseStarted           (void);
    void eraseComplete          (void);
    void flashComplete          (void);
    
private slots:
    void _init              (void);
    void _startFindBoardLoop(void);
    void _reboot            (void);
    void _flash             (void);
    void _findBoardOnce     (void);
    void _updateProgress    (int curr, int total) { emit updateProgress(curr, total); }
    void _cancel            (void);
    
private:
    bool _findBoardFromPorts(QGCSerialPortInfo& portInfo, QGCSerialPortInfo::BoardType_t& boardType, QString& boardName);
    bool _erase             (void);
    
    PX4FirmwareUpgradeThreadController* _controller;
    
    Bootloader*         _bootloader             = nullptr;
    QTimer*             _findBoardTimer         = nullptr;
    QTime               _elapsed;
    bool                _foundBoard             = false;
    bool                _boardIsSiKRadio        = false;
    bool                _findBoardFirstAttempt  = true;     ///< true: we found the board right away, it needs to be unplugged and plugged back in
    QGCSerialPortInfo   _foundBoardPortInfo;                ///< port info for found board
};

/// @brief Provides methods to interact with the bootloader. The commands themselves are signalled
///         across to PX4FirmwareUpgradeThreadWorker so that they run on the separate thread.
class PX4FirmwareUpgradeThreadController : public QObject
{
    Q_OBJECT
    
public:
    PX4FirmwareUpgradeThreadController(QObject* parent = nullptr);
    ~PX4FirmwareUpgradeThreadController(void);
    
    /// @brief Begins the process of searching for a supported board connected to any serial port. This will
    /// continue until cancelFind is called. Signals foundBoard and boardGone as boards come and go.
    void startFindBoardLoop(void);
    
    void cancel(void);
    
    /// @brief Sends a reboot command to the bootloader
    void reboot(void) { emit _rebootOnThread(); }
    
    void flash(const FirmwareImage* image);
    
    const FirmwareImage* image(void) { return _image; }
    
signals:
    void foundBoard     (bool firstAttempt, const QGCSerialPortInfo &portInfo, int boardType, QString boardName);
    void noBoardFound   (void);
    void boardGone      (void);
    void foundBoardInfo (int bootloaderVersion, int boardID, int flashSize);
    void error          (const QString& errorString);
    void status         (const QString& status);
    void eraseStarted   (void);
    void eraseComplete  (void);
    void flashComplete  (void);
    void updateProgress (int curr, int total);
    
    // Internal signals to communicate with thread worker
    void _initThreadWorker          (void);
    void _startFindBoardLoopOnThread(void);
    void _rebootOnThread            (void);
    void _flashOnThread             (void);
    void _cancel                    (void);
    
private slots:
    void _foundBoard            (bool firstAttempt, const QGCSerialPortInfo& portInfo, int type, QString name) { emit foundBoard(firstAttempt, portInfo, type, name); }
    void _noBoardFound          (void) { emit noBoardFound(); }
    void _boardGone             (void) { emit boardGone(); }
    void _foundBoardInfo        (int bootloaderVersion, int boardID, int flashSize) { emit foundBoardInfo(bootloaderVersion, boardID, flashSize); }
    void _error                 (const QString& errorString) { emit error(errorString); }
    void _status                (const QString& statusText) { emit status(statusText); }
    void _eraseStarted          (void) { emit eraseStarted(); }
    void _eraseComplete         (void) { emit eraseComplete(); }
    void _flashComplete         (void) { emit flashComplete(); }
    
private:
    void _updateProgress(int curr, int total) { emit updateProgress(curr, total); }
    
    PX4FirmwareUpgradeThreadWorker* _worker         = nullptr;
    QThread*                        _workerThread   = nullptr;  ///< Thread which PX4FirmwareUpgradeThreadWorker runs on
    
    const FirmwareImage* _image;
};

#endif
