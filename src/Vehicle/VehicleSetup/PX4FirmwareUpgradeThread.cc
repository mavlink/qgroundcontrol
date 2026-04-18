#include "PX4FirmwareUpgradeThread.h"
#include "Bootloader.h"
#include "FirmwareImage.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QThread>

QGC_LOGGING_CATEGORY(PX4FirmwareUpgradeThreadLog, "Vehicle.VehicleSetup.PX4FirmwareUpgradeThread")
QGC_LOGGING_CATEGORY(PX4FirmwareUpgradeThreadVerboseLog, "Vehicle.VehicleSetup.PX4FirmwareUpgradeThread.Verbose")
#include <QtCore/QTimer>

PX4FirmwareUpgradeThreadWorker::PX4FirmwareUpgradeThreadWorker(PX4FirmwareUpgradeThreadController* controller)
    : _controller(controller)
{
    connect(_controller, &PX4FirmwareUpgradeThreadController::_initThreadWorker,            this, &PX4FirmwareUpgradeThreadWorker::_init);
    connect(_controller, &PX4FirmwareUpgradeThreadController::_startFindBoardLoopOnThread,  this, &PX4FirmwareUpgradeThreadWorker::_startFindBoardLoop);
    connect(_controller, &PX4FirmwareUpgradeThreadController::_flashOnThread,               this, &PX4FirmwareUpgradeThreadWorker::_flash);
    connect(_controller, &PX4FirmwareUpgradeThreadController::_rebootOnThread,              this, &PX4FirmwareUpgradeThreadWorker::_reboot);
    connect(_controller, &PX4FirmwareUpgradeThreadController::_cancel,                      this, &PX4FirmwareUpgradeThreadWorker::_cancel);
}

PX4FirmwareUpgradeThreadWorker::~PX4FirmwareUpgradeThreadWorker()
{

}

void PX4FirmwareUpgradeThreadWorker::_init(void)
{
    // We create the timers here so that they are on the right thread

    _findBoardTimer = new QTimer(this);
    _findBoardTimer->setSingleShot(true);
    _findBoardTimer->setInterval(500);
    connect(_findBoardTimer, &QTimer::timeout, this, &PX4FirmwareUpgradeThreadWorker::_findBoardOnce);
}

void PX4FirmwareUpgradeThreadWorker::_cancel(void)
{
    qCDebug(PX4FirmwareUpgradeThreadVerboseLog) << "_cancel";
    if (_bootloader) {
        _bootloader->reboot();
        _bootloader->close();
        _bootloader->deleteLater();
        _bootloader = nullptr;
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
    qCDebug(PX4FirmwareUpgradeThreadVerboseLog) << "_findBoardOnce";

    QGCSerialPortInfo               portInfo;
    QGCSerialPortInfo::BoardType_t  boardType;
    QString                         boardName;

    if (_findBoardFromPorts(portInfo, boardType, boardName)) {
        if (!_foundBoard) {
            _foundBoard = true;
            _foundBoardPortInfo = portInfo;
            emit foundBoard(_findBoardFirstAttempt, portInfo, boardType, boardName);
            if (!_findBoardFirstAttempt) {

                _bootloader = new Bootloader(boardType == QGCSerialPortInfo::BoardTypeSiKRadio, this);
                connect(_bootloader, &Bootloader::updateProgress, this, &PX4FirmwareUpgradeThreadWorker::_updateProgress);

                if (_bootloader->open(portInfo.portName())) {
                    uint32_t    bootloaderVersion;
                    uint32_t    boardId;
                    uint32_t    flashSize;
                    if (_bootloader->getBoardInfo(bootloaderVersion, boardId, flashSize)) {
                        emit foundBoardInfo(bootloaderVersion, boardId, flashSize);
                    } else {
                        emit error(_bootloader->errorString());
                    }
                } else {
                    emit error(_bootloader->errorString());
                }
                return;
            }
        }
    } else {
        if (_foundBoard) {
            _foundBoard = false;
            qCDebug(PX4FirmwareUpgradeThreadLog) << "Board gone";
            emit boardGone();
        } else if (_findBoardFirstAttempt) {
            emit noBoardFound();
        }
    }

    _findBoardFirstAttempt = false;
    _findBoardTimer->start();
}

bool PX4FirmwareUpgradeThreadWorker::_findBoardFromPorts(QGCSerialPortInfo& portInfo, QGCSerialPortInfo::BoardType_t& boardType, QString& boardName)
{
    for (const QGCSerialPortInfo& info: QGCSerialPortInfo::availablePorts()) {
        info.getBoardInfo(boardType, boardName);

        qCDebug(PX4FirmwareUpgradeThreadVerboseLog) << "Serial Port --------------";
        qCDebug(PX4FirmwareUpgradeThreadVerboseLog) << "\tboard type" << boardType;
        qCDebug(PX4FirmwareUpgradeThreadVerboseLog) << "\tboard name" << boardName;
        qCDebug(PX4FirmwareUpgradeThreadVerboseLog) << "\tmanufacturer:" << info.manufacturer();
        qCDebug(PX4FirmwareUpgradeThreadVerboseLog) << "\tport name:" << info.portName();
        qCDebug(PX4FirmwareUpgradeThreadVerboseLog) << "\tdescription:" << info.description();
        qCDebug(PX4FirmwareUpgradeThreadVerboseLog) << "\tsystem location:" << info.systemLocation();
        qCDebug(PX4FirmwareUpgradeThreadVerboseLog) << "\tvendor ID:" << info.vendorIdentifier();
        qCDebug(PX4FirmwareUpgradeThreadVerboseLog) << "\tproduct ID:" << info.productIdentifier();

        if (info.canFlash()) {
            portInfo = info;
            return true;
        }
    }

    return false;
}

void PX4FirmwareUpgradeThreadWorker::_reboot(void)
{
    _bootloader->reboot();
}

void PX4FirmwareUpgradeThreadWorker::_flash(void)
{
    qCDebug(PX4FirmwareUpgradeThreadLog) << "PX4FirmwareUpgradeThreadWorker::_flash";

    if (!_bootloader->initFlashSequence()) {
        emit error(_bootloader->errorString());
        return;
    }

    if (_erase()) {
        emit status(tr("Programming new version..."));

        if (_bootloader->program(_controller->image())) {
            qCDebug(PX4FirmwareUpgradeThreadLog) << "Program complete";
            emit status("Program complete");
        } else {
            qCDebug(PX4FirmwareUpgradeThreadLog) << "Program failed:" << _bootloader->errorString();
            goto Error;
        }

        emit status(tr("Verifying program..."));

        if (_bootloader->verify(_controller->image())) {
            qCDebug(PX4FirmwareUpgradeThreadLog) << "Verify complete";
            emit status(tr("Verify complete"));
        } else {
            qCDebug(PX4FirmwareUpgradeThreadLog) << "Verify failed:" << _bootloader->errorString();
            goto Error;
        }
    }

    emit status(tr("Rebooting board"));
    _reboot();

    _bootloader->close();
    _bootloader->deleteLater();
    _bootloader = nullptr;

    emit flashComplete();

    return;

Error:
    emit error(_bootloader->errorString());
    _reboot();
    _bootloader->close();
    _bootloader->deleteLater();
    _bootloader = nullptr;
}

bool PX4FirmwareUpgradeThreadWorker::_erase(void)
{
    qCDebug(PX4FirmwareUpgradeThreadLog) << "PX4FirmwareUpgradeThreadWorker::_erase";

    emit eraseStarted();
    emit status(tr("Erasing previous program..."));

    if (_bootloader->erase()) {
        qCDebug(PX4FirmwareUpgradeThreadLog) << "Erase complete";
        emit status(tr("Erase complete"));
        emit eraseComplete();
        return true;
    } else {
        qCDebug(PX4FirmwareUpgradeThreadLog) << "Erase failed:" << _bootloader->errorString();
        emit error(_bootloader->errorString());
        return false;
    }
}

PX4FirmwareUpgradeThreadController::PX4FirmwareUpgradeThreadController(QObject* parent) :
    QObject(parent)
{
    _worker         = new PX4FirmwareUpgradeThreadWorker(this);
    _workerThread   = new QThread(this);
    _worker->moveToThread(_workerThread);

    connect(_worker, &PX4FirmwareUpgradeThreadWorker::updateProgress,       this, &PX4FirmwareUpgradeThreadController::_updateProgress);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::foundBoard,           this, &PX4FirmwareUpgradeThreadController::_foundBoard);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::noBoardFound,         this, &PX4FirmwareUpgradeThreadController::_noBoardFound);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::boardGone,            this, &PX4FirmwareUpgradeThreadController::_boardGone);
    connect(_worker, &PX4FirmwareUpgradeThreadWorker::foundBoardInfo,       this, &PX4FirmwareUpgradeThreadController::_foundBoardInfo);
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
    qCDebug(PX4FirmwareUpgradeThreadLog) << "PX4FirmwareUpgradeThreadController::findBoard";
    emit _startFindBoardLoopOnThread();
}

void PX4FirmwareUpgradeThreadController::cancel(void)
{
    qCDebug(PX4FirmwareUpgradeThreadLog) << "PX4FirmwareUpgradeThreadController::cancel";
    emit _cancel();
}

void PX4FirmwareUpgradeThreadController::flash(const FirmwareImage* image)
{
    _image = image;
    emit _flashOnThread();
}
