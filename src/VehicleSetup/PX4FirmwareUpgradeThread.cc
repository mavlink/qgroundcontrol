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

#include "PX4FirmwareUpgradeThread.h"
#include "Bootloader.h"
#include "QGCLoggingCategory.h"
#include "QGC.h"

#include <QTimer>
#include <QDebug>
#include <QSerialPort>

PX4FirmwareUpgradeThreadWorker::PX4FirmwareUpgradeThreadWorker(PX4FirmwareUpgradeThreadController* controller) :
    _controller(controller),
    _bootloader(nullptr),
    _bootloaderPort(nullptr),
    _timerRetry(nullptr),
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
    
    Q_ASSERT(_timerRetry == nullptr);
    _timerRetry = new QTimer(this);
    Q_CHECK_PTR(_timerRetry);
    _timerRetry->setSingleShot(true);
    _timerRetry->setInterval(_retryTimeout);
    connect(_timerRetry, &QTimer::timeout, this, &PX4FirmwareUpgradeThreadWorker::_findBoardOnce);
    
    Q_ASSERT(_bootloader == nullptr);
    _bootloader = new Bootloader(this);
    connect(_bootloader, &Bootloader::updateProgress, this, &PX4FirmwareUpgradeThreadWorker::_updateProgress);
}

void PX4FirmwareUpgradeThreadWorker::_cancel(void)
{
    if (_bootloaderPort) {
        _bootloaderPort->close();
        _bootloaderPort->deleteLater();
        _bootloaderPort = nullptr;
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
    qCDebug(FirmwareUpgradeVerboseLog) << "_findBoardOnce";
    
    QGCSerialPortInfo               portInfo;
    QGCSerialPortInfo::BoardType_t  boardType;
    QString                         boardName;
    
    if (_findBoardFromPorts(portInfo, boardType, boardName)) {
        if (!_foundBoard) {
            _foundBoard = true;
            _foundBoardPortInfo = portInfo;
            emit foundBoard(_findBoardFirstAttempt, portInfo, boardType, boardName);
            if (!_findBoardFirstAttempt) {
                if (boardType == QGCSerialPortInfo::BoardTypeSiKRadio) {
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

bool PX4FirmwareUpgradeThreadWorker::_findBoardFromPorts(QGCSerialPortInfo& portInfo, QGCSerialPortInfo::BoardType_t& boardType, QString& boardName)
{
    for (const QGCSerialPortInfo& info: QGCSerialPortInfo::availablePorts()) {
        info.getBoardInfo(boardType, boardName);

        qCDebug(FirmwareUpgradeVerboseLog) << "Serial Port --------------";
        qCDebug(FirmwareUpgradeVerboseLog) << "\tboard type" << boardType;
        qCDebug(FirmwareUpgradeVerboseLog) << "\tboard name" << boardName;
        qCDebug(FirmwareUpgradeVerboseLog) << "\tmanufacturer:" << info.manufacturer();
        qCDebug(FirmwareUpgradeVerboseLog) << "\tport name:" << info.portName();
        qCDebug(FirmwareUpgradeVerboseLog) << "\tdescription:" << info.description();
        qCDebug(FirmwareUpgradeVerboseLog) << "\tsystem location:" << info.systemLocation();
        qCDebug(FirmwareUpgradeVerboseLog) << "\tvendor ID:" << info.vendorIdentifier();
        qCDebug(FirmwareUpgradeVerboseLog) << "\tproduct ID:" << info.productIdentifier();
        
        if (info.canFlash()) {
            portInfo = info;
            return true;
        }
    }
    
    return false;
}

void PX4FirmwareUpgradeThreadWorker::_3drRadioForceBootloader(const QGCSerialPortInfo& portInfo)
{
    // First make sure we can't get the bootloader
    
    if (_findBootloader(portInfo, true /* radio Mode */, false /* errorOnNotFound */)) {
        return;
    }

    // Couldn't find the bootloader. We'll need to reboot the radio into bootloader.
    
    QSerialPort port(portInfo);
    
    port.setBaudRate(QSerialPort::Baud57600);
    
    emit status(tr("Putting radio into command mode"));
    
    // Wait a little while for the USB port to initialize. 3DR Radio boot is really slow.
    QGC::SLEEP::msleep(2000);
    port.open(QIODevice::ReadWrite);
    
    if (!port.isOpen()) {
        emit error(tr("Unable to open port: %1 error: %2").arg(portInfo.systemLocation()).arg(port.errorString()));
        return;
    }

    // Put radio into command mode
    QGC::SLEEP::msleep(2000);
    port.write("+++", 3);
    if (!port.waitForReadyRead(1500)) {
        emit error(tr("Unable to put radio into command mode"));
        return;
    }
    QByteArray bytes = port.readAll();
    if (!bytes.contains("OK")) {
        qCDebug(FirmwareUpgradeLog) << bytes;
        emit error(tr("Unable to put radio into command mode"));
        return;
    }

    emit status(tr("Rebooting radio to bootloader"));
    
    port.write("AT&UPDATE\r\n");
    if (!port.waitForBytesWritten(1500)) {
        emit error(tr("Unable to reboot radio (bytes written)"));
        return;
    }
    if (!port.waitForReadyRead(1500)) {
        emit error(tr("Unable to reboot radio (ready read)"));
        return;
    }
    port.close();
    QGC::SLEEP::msleep(2000);

    // The bootloader should be waiting for us now
    
    _findBootloader(portInfo, true /* radio mode */, true /* errorOnNotFound */);
}

bool PX4FirmwareUpgradeThreadWorker::_findBootloader(const QGCSerialPortInfo& portInfo, bool radioMode, bool errorOnNotFound)
{
    qCDebug(FirmwareUpgradeLog) << "_findBootloader" << portInfo.systemLocation();
    
    uint32_t bootloaderVersion = 0;
    uint32_t boardID;
    uint32_t flashSize = 0;
    
    _bootloaderPort = new QSerialPort();
    if (radioMode) {
        _bootloaderPort->setBaudRate(QSerialPort::Baud115200);
    }

    // Wait a little while for the USB port to initialize.
    bool openFailed = true;
    for (int i=0; i<10; i++) {
        if (_bootloader->open(_bootloaderPort, portInfo.systemLocation())) {
            openFailed = false;
            break;
        } else {
            QGC::SLEEP::msleep(100);
        }
    }
    
    if (radioMode) {
        QGC::SLEEP::msleep(2000);
    }

    if (openFailed) {
        qCDebug(FirmwareUpgradeLog) << "Bootloader open port failed:" << _bootloader->errorString();
        if (errorOnNotFound) {
            emit error(_bootloader->errorString());
        }
        _bootloaderPort->deleteLater();
        _bootloaderPort = nullptr;
        return false;
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
    _bootloaderPort = nullptr;
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
        _bootloaderPort = nullptr;
    }
}

void PX4FirmwareUpgradeThreadWorker::_flash(void)
{
    qCDebug(FirmwareUpgradeLog) << "PX4FirmwareUpgradeThreadWorker::_flash";
    
    if (_erase()) {
        emit status(tr("Programming new version..."));
        
        if (_bootloader->program(_bootloaderPort, _controller->image())) {
            qCDebug(FirmwareUpgradeLog) << "Program complete";
            emit status("Program complete");
        } else {
            _bootloaderPort->deleteLater();
            _bootloaderPort = nullptr;
            qCDebug(FirmwareUpgradeLog) << "Program failed:" << _bootloader->errorString();
            emit error(_bootloader->errorString());
            return;
        }
        
        emit status(tr("Verifying program..."));
        
        if (_bootloader->verify(_bootloaderPort, _controller->image())) {
            qCDebug(FirmwareUpgradeLog) << "Verify complete";
            emit status(tr("Verify complete"));
        } else {
            qCDebug(FirmwareUpgradeLog) << "Verify failed:" << _bootloader->errorString();
            emit error(_bootloader->errorString());
            return;
        }
    }
    
    emit _reboot();
    
    emit flashComplete();
}

bool PX4FirmwareUpgradeThreadWorker::_erase(void)
{
    qCDebug(FirmwareUpgradeLog) << "PX4FirmwareUpgradeThreadWorker::_erase";
    
    emit eraseStarted();
    emit status(tr("Erasing previous program..."));
    
    if (_bootloader->erase(_bootloaderPort)) {
        qCDebug(FirmwareUpgradeLog) << "Erase complete";
        emit status(tr("Erase complete"));
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
