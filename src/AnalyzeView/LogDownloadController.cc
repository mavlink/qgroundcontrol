#include "LogDownloadController.h"
#include "AppSettings.h"
#include "LogDownloadStateMachine.h"
#include "LogEntry.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"

#include <QtCore/QApplicationStatic>

QGC_LOGGING_CATEGORY(LogDownloadControllerLog, "AnalyzeView.LogDownloadController")

LogDownloadController::LogDownloadController(QObject *parent)
    : QObject(parent)
    , _logEntriesModel(new QmlObjectListModel(this))
{
    qCDebug(LogDownloadControllerLog) << this;

    _stateMachine = new LogDownloadStateMachine(this, this);

    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &LogDownloadController::_setActiveVehicle);

    // Forward state machine signals
    (void) connect(_stateMachine, &LogDownloadStateMachine::requestingListChanged, this, &LogDownloadController::requestingListChanged);
    (void) connect(_stateMachine, &LogDownloadStateMachine::downloadingChanged, this, &LogDownloadController::downloadingLogsChanged);

    _setActiveVehicle(MultiVehicleManager::instance()->activeVehicle());
}

LogDownloadController::~LogDownloadController()
{
    qCDebug(LogDownloadControllerLog) << this;
}

bool LogDownloadController::_getRequestingList() const
{
    return _stateMachine->isRequestingList();
}

bool LogDownloadController::_getDownloadingLogs() const
{
    return _stateMachine->isDownloading();
}

void LogDownloadController::refresh()
{
    _logEntriesModel->clearAndDeleteContents();
    _stateMachine->startListRequest();
}

void LogDownloadController::download(const QString &path)
{
    QString dir = path.isEmpty() ? SettingsManager::instance()->appSettings()->logSavePath() : path;
    if (dir.isEmpty()) {
        return;
    }

    if (!dir.endsWith(QDir::separator())) {
        dir += QDir::separator();
    }

    _downloadPath = dir;

    QGCLogEntry *const log = _getNextSelected();
    if (log) {
        log->setStatus(tr("Waiting"));
    }

    _stateMachine->startDownload(_downloadPath);
}

void LogDownloadController::cancel()
{
    _stateMachine->cancel();

    if (_downloadData) {
        _downloadData->entry->setStatus(QStringLiteral("Canceled"));
        if (_downloadData->file.exists()) {
            (void) _downloadData->file.remove();
        }

        _downloadData.reset();
    }

    _resetSelection(true);
}

void LogDownloadController::eraseAll()
{
    if (!_vehicle) {
        qCWarning(LogDownloadControllerLog) << "Vehicle Unavailable";
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(LogDownloadControllerLog) << "Link Unavailable";
        return;
    }

    mavlink_message_t msg{};
    (void) mavlink_msg_log_erase_pack_chan(
        MAVLinkProtocol::instance()->getSystemId(),
        MAVLinkProtocol::getComponentId(),
        sharedLink->mavlinkChannel(),
        &msg,
        _vehicle->id(),
        _vehicle->defaultComponentId()
    );

    if (!_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg)) {
        qCWarning(LogDownloadControllerLog) << "Failed to send";
        return;
    }

    refresh();
}

void LogDownloadController::_setActiveVehicle(Vehicle *vehicle)
{
    if (vehicle == _vehicle) {
        return;
    }

    if (_vehicle) {
        _logEntriesModel->clearAndDeleteContents();
        (void) disconnect(_vehicle, &Vehicle::logEntry, this, &LogDownloadController::_logEntry);
        (void) disconnect(_vehicle, &Vehicle::logData,  this, &LogDownloadController::_logData);
    }

    _vehicle = vehicle;

    if (_vehicle) {
        (void) connect(_vehicle, &Vehicle::logEntry, this, &LogDownloadController::_logEntry);
        (void) connect(_vehicle, &Vehicle::logData,  this, &LogDownloadController::_logData);
    }
}

void LogDownloadController::_logEntry(uint32_t time_utc, uint32_t size, uint16_t id, uint16_t num_logs, uint16_t last_log_num)
{
    Q_UNUSED(last_log_num);
    _stateMachine->handleLogEntry(time_utc, size, id, num_logs);
}

void LogDownloadController::_logData(uint32_t ofs, uint16_t id, uint8_t count, const uint8_t *data)
{
    _stateMachine->handleLogData(ofs, id, count, data);
}

QGCLogEntry *LogDownloadController::_getNextSelected() const
{
    const int numLogs = _logEntriesModel->count();
    for (int i = 0; i < numLogs; i++) {
        QGCLogEntry *const entry = _logEntriesModel->value<QGCLogEntry*>(i);
        if (!entry) {
            continue;
        }

        if (entry->selected()) {
           return entry;
        }
    }

    return nullptr;
}

void LogDownloadController::_resetSelection(bool canceled)
{
    const int num_logs = _logEntriesModel->count();
    for (int i = 0; i < num_logs; i++) {
        QGCLogEntry *const entry = _logEntriesModel->value<QGCLogEntry*>(i);
        if (!entry) {
            continue;
        }

        if (entry->selected()) {
            if (canceled) {
                entry->setStatus(tr("Canceled"));
            }
            entry->setSelected(false);
        }
    }

    emit selectionChanged();
}

void LogDownloadController::_updateDataRate()
{
    if (!_downloadData || _downloadData->elapsed.elapsed() < kGUIRateMs) {
        return;
    }

    const qreal rate = _downloadData->rate_bytes / (_downloadData->elapsed.elapsed() / 1000.0);
    _downloadData->rate_avg = (_downloadData->rate_avg * 0.95) + (rate * 0.05);
    _downloadData->rate_bytes = 0;

    const QString status = QStringLiteral("%1 (%2/s)").arg(qgcApp()->bigSizeToString(_downloadData->written),
                                                           qgcApp()->bigSizeToString(_downloadData->rate_avg));

    _downloadData->entry->setStatus(status);
    _downloadData->elapsed.start();
}

void LogDownloadController::setCompressLogs(bool compress)
{
    if (_compressLogs != compress) {
        _compressLogs = compress;
        emit compressLogsChanged();
    }
}

bool LogDownloadController::compressLogFile(const QString &logPath)
{
    Q_UNUSED(logPath)
    qCWarning(LogDownloadControllerLog) << "Log compression not yet implemented (decompression-only API)";
    return false;
}

void LogDownloadController::cancelCompression()
{
    // Not implemented - compression API is decompression-only
}

void LogDownloadController::_handleCompressionProgress(qreal progress)
{
    Q_UNUSED(progress)
    // Not implemented - compression API is decompression-only
}

void LogDownloadController::_handleCompressionFinished(bool success)
{
    Q_UNUSED(success)
    // Not implemented - compression API is decompression-only
}
