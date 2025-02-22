/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LogReplayLink.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFileInfo>
#include <QtCore/QtEndian>
#include <QtCore/QThread>
#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(LogReplayLinkLog, "qgc.comms.logreplaylink")

/*===========================================================================*/

LogReplayConfiguration::LogReplayConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
{
    // qCDebug(LogReplayLinkLog) << Q_FUNC_INFO << this;
}

LogReplayConfiguration::LogReplayConfiguration(const LogReplayConfiguration *copy, QObject *parent)
    : LinkConfiguration(copy, parent)
    , _logFilename(copy->logFilename())
{
    // qCDebug(LogReplayLinkLog) << Q_FUNC_INFO << this;
}

LogReplayConfiguration::~LogReplayConfiguration()
{
    // qCDebug(LogReplayLinkLog) << Q_FUNC_INFO << this;
}

void LogReplayConfiguration::copyFrom(const LinkConfiguration *source)
{
    Q_ASSERT(source);
    LinkConfiguration::copyFrom(source);

    const LogReplayConfiguration *const logReplaySource = qobject_cast<const LogReplayConfiguration*>(source);
    Q_ASSERT(logReplaySource);

    setLogFilename(logReplaySource->logFilename());
}

void LogReplayConfiguration::loadSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    setLogFilename(settings.value("logFilename", "").toString());

    settings.endGroup();
}

void LogReplayConfiguration::saveSettings(QSettings &settings, const QString &root) const
{
    settings.beginGroup(root);

    settings.setValue("logFilename", _logFilename);

    settings.endGroup();
}

QString LogReplayConfiguration::logFilenameShort() const
{
    return QFileInfo(_logFilename).fileName();
}

void LogReplayConfiguration::setLogFilename(const QString &logFilename)
{
    if (logFilename != _logFilename) {
        _logFilename = logFilename;
        emit filenameChanged();
    }
}

/*===========================================================================*/

LogReplayWorker::LogReplayWorker(const LogReplayConfiguration *config, QObject *parent)
    : QObject(parent)
    , _logReplayConfig(config)
{
    // qCDebug(LogReplayLinkLog) << Q_FUNC_INFO << this;
}

LogReplayWorker::~LogReplayWorker()
{
    disconnectFromLog();

    // qCDebug(LogReplayLinkLog) << Q_FUNC_INFO << this;
}

void LogReplayWorker::setup()
{
    Q_ASSERT(!_readTickTimer);
    _readTickTimer = new QTimer(this);

    (void) connect(_readTickTimer, &QTimer::timeout, this, &LogReplayWorker::_readNextLogEntry);
}

void LogReplayWorker::connectToLog()
{
    if (isConnected()) {
        qCWarning(LogReplayLinkLog) << "Already connected";
        return;
    }

    if (MultiVehicleManager::instance()->activeVehicle()) {
        emit errorOccurred(tr("You must close all connections prior to replaying a log."));
        return;
    }

    if (!_loadLogFile()) {
        disconnectFromLog();
        return;
    }

    _isConnected = true;
    emit connected();

    play();
}

void LogReplayWorker::disconnectFromLog()
{
    if (!isConnected()) {
        qCDebug(LogReplayLinkLog) << "Already disconnected";
        return;
    }

    _isConnected = false;
    emit disconnected();

    _readTickTimer->stop();
}

bool LogReplayWorker::isPlaying() const
{
    return (_readTickTimer && _readTickTimer->isActive());
}

void LogReplayWorker::play()
{
    LinkManager::instance()->setConnectionsSuspended(tr("Connect not allowed during Flight Data replay."));
    MAVLinkProtocol::instance()->suspendLogForReplay(true);

    if (_logFile.atEnd()) {
        _resetPlaybackToBeginning();
    }

    _playbackStartTimeMSecs = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    _playbackStartLogTimeUSecs = _logCurrentTimeUSecs;
    _readTickTimer->start(1);

    emit playbackStarted();
}

void LogReplayWorker::pause()
{
    LinkManager::instance()->setConnectionsAllowed();
    MAVLinkProtocol::instance()->suspendLogForReplay(false);

    _readTickTimer->stop();

    emit playbackPaused();
}

void LogReplayWorker::setPlaybackSpeed(qreal playbackSpeed)
{
    _playbackSpeed = playbackSpeed;
    _playbackStartTimeMSecs = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    _playbackStartLogTimeUSecs = _logCurrentTimeUSecs;
    _readTickTimer->start(1);
}

void LogReplayWorker::movePlayhead(qreal percentComplete)
{
    if (isPlaying()) {
        pause();
        if (_readTickTimer->isActive()) {
            return;
        }
    }

    percentComplete = qBound(0., percentComplete, 100.);
    const qreal percentCompleteMult = percentComplete / 100.0;
    const qint64 newFilePos = static_cast<qint64>(percentCompleteMult * static_cast<qreal>(_logFile.size()));
    if (!_logFile.seek(newFilePos)) {
        emit errorOccurred(tr("Unable to seek to new position"));
        return;
    }

    mavlink_message_t dummy{};
    _logCurrentTimeUSecs = _seekToNextMavlinkMessage(dummy);

    qreal newRelativeTimeUSecs = static_cast<qreal>(_logCurrentTimeUSecs - _logStartTimeUSecs);
    const qreal baudRate = _logFile.size() / static_cast<qreal>(_logDurationUSecs) / 1e6;
    const qreal desiredTimeUSecs = percentCompleteMult * _logDurationUSecs;
    const qint64 offset = (newRelativeTimeUSecs - desiredTimeUSecs) * baudRate;
    if (!_logFile.seek(_logFile.pos() + offset)) {
        emit errorOccurred(tr("Unable to seek to new position"));
        return;
    }

    _logCurrentTimeUSecs = _seekToNextMavlinkMessage(dummy);
    _signalCurrentLogTimeSecs();

    newRelativeTimeUSecs = static_cast<qreal>(_logCurrentTimeUSecs - _logStartTimeUSecs);
    percentComplete = ((newRelativeTimeUSecs / _logDurationUSecs) * 100);
    emit playbackPercentCompleteChanged(percentComplete);
}

void LogReplayWorker::_resetPlaybackToBeginning()
{
    if (_logFile.isOpen()) {
        if (!_logFile.reset()) {
            qCWarning(LogReplayLinkLog) << "failed to reset log file:" << _logFile.error() << _logFile.errorString();
        }
    }

    _playbackStartTimeMSecs = 0;
    _playbackStartLogTimeUSecs = 0;
    _logCurrentTimeUSecs = _logStartTimeUSecs;
}

void LogReplayWorker::_readNextLogEntry()
{
    int timeToNextExecutionMSecs = 0;
    while (timeToNextExecutionMSecs < 3) {
        QByteArray bytes;
        bytes.reserve(_logFile.bytesAvailable());
        const qint64 nextTimeUSecs = _readNextMavlinkMessage(bytes);
        emit dataReceived(bytes);
        emit playbackPercentCompleteChanged((static_cast<float>(_logCurrentTimeUSecs - _logStartTimeUSecs) / static_cast<float>(_logDurationUSecs)) * 100);

        if (_logFile.atEnd()) {
            pause();
            emit playbackAtEnd();
            return;
        }

        _logCurrentTimeUSecs = nextTimeUSecs;

        const quint64 currentTimeMSecs = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
        const quint64 desiredPlayheadMovementTimeMSecs = ((_logCurrentTimeUSecs - _playbackStartLogTimeUSecs) / 1000) / _playbackSpeed;
        const quint64 desiredCurrentTimeMSecs = _playbackStartTimeMSecs + desiredPlayheadMovementTimeMSecs;
        timeToNextExecutionMSecs = desiredCurrentTimeMSecs - currentTimeMSecs;
    }

    _signalCurrentLogTimeSecs();

    _readTickTimer->start(timeToNextExecutionMSecs);
}

void LogReplayWorker::_signalCurrentLogTimeSecs()
{
    emit currentLogTimeSecs((_logCurrentTimeUSecs - _logStartTimeUSecs) / 1000000);
}

bool LogReplayWorker::_loadLogFile()
{
    if (_logFile.isOpen()) {
        _logFile.close();
        emit errorOccurred(tr("Attempt to load new log while log being played"));
        return false;
    }

    const QString logFilename = _logReplayConfig->logFilename();
    _logFile.setFileName(logFilename);
    if (!_logFile.open(QFile::ReadOnly)) {
        emit errorOccurred(tr("Unable to open log file: '%1', error: %2").arg(logFilename, _logFile.errorString()));
        return false;
    }

    QFileInfo logFileInfo;
    logFileInfo.setFile(logFilename);
    _logFileSize = logFileInfo.size();

    const quint64 startTimeUSecs = _parseTimestamp(_logFile.read(kTimestamp));
    const quint64 endTimeUSecs = _findLastTimestamp();
    if (endTimeUSecs <= startTimeUSecs) {
        _logFile.close();
        emit errorOccurred(tr("The log file '%1' is corrupt or empty.").arg(logFilename));
        return false;
    }

    _logEndTimeUSecs = endTimeUSecs;
    _logStartTimeUSecs = startTimeUSecs;
    _logDurationUSecs = endTimeUSecs - startTimeUSecs;
    _logCurrentTimeUSecs = startTimeUSecs;

    if (!_logFile.reset()) {
        qCWarning(LogReplayLinkLog) << "failed to reset log file:" << _logFile.error() << _logFile.errorString();
    }

    const quint64 logDurationSecondsTotal = _logDurationUSecs / 1000000;
    emit logFileStats(logDurationSecondsTotal);

    return true;
}

quint64 LogReplayWorker::_parseTimestamp(const QByteArray &bytes)
{
    const quint64 currentTimestamp = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch()) * 1000;
    quint64 timestamp = qFromBigEndian(*reinterpret_cast<const quint64*>(bytes.constData()));
    if (timestamp > currentTimestamp) {
        timestamp = qbswap(timestamp);
    }

    return timestamp;
}

quint64 LogReplayWorker::_readNextMavlinkMessage(QByteArray &bytes)
{
    bytes.clear();

    char nextByte;
    while (_logFile.getChar(&nextByte)) {
        mavlink_message_t message{};
        mavlink_status_t status{};
        const bool messageFound = mavlink_parse_char(_mavlinkChannel, nextByte, &message, &status);

        if (status.parse_state == MAVLINK_PARSE_STATE_GOT_STX) {
            bytes.clear();
        }
        (void) bytes.append(nextByte);

        if (messageFound) {
            const QByteArray rawTime = _logFile.read(kTimestamp);
            return _parseTimestamp(rawTime);
        }
    }

    return 0;
}

quint64 LogReplayWorker::_seekToNextMavlinkMessage(mavlink_message_t &nextMsg)
{
    mavlink_reset_channel_status(_mavlinkChannel);

    qint64 messageStartPos = -1;
    char nextByte;
    while (_logFile.getChar(&nextByte)) {
        mavlink_status_t status{};
        const bool messageFound = mavlink_parse_char(_mavlinkChannel, nextByte, &nextMsg, &status);

        if (status.parse_state == MAVLINK_PARSE_STATE_GOT_STX) {
            messageStartPos = _logFile.pos() - 1;
        }

        if (messageFound && (messageStartPos != -1)) {
            if (!_logFile.seek(messageStartPos - kTimestamp)) {
                qCWarning(LogReplayLinkLog) << "Failed to seek next message:" << _logFile.error() << _logFile.errorString();
                break;
            }

            const QByteArray rawTime = _logFile.read(kTimestamp);
            return _parseTimestamp(rawTime);
        }
    }

    return 0;
}

quint64 LogReplayWorker::_findLastTimestamp()
{
    if (!_logFile.reset()) {
        qCWarning(LogReplayLinkLog) << "failed to reset log file:" << _logFile.error() << _logFile.errorString();
    }

    mavlink_reset_channel_status(_mavlinkChannel);

    quint64 lastTimestamp = 0;

    while (_logFile.bytesAvailable() > kTimestamp) {
        lastTimestamp = _parseTimestamp(_logFile.read(kTimestamp));

        bool endOfMessage = false;
        char nextByte;
        while (!endOfMessage && _logFile.getChar(&nextByte)) {
            mavlink_message_t msg{};
            mavlink_status_t status{};
            endOfMessage = mavlink_parse_char(_mavlinkChannel, nextByte, &msg, &status);
        }
    }

    return lastTimestamp;
}

/*===========================================================================*/

LogReplayLink::LogReplayLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _logReplayConfig(qobject_cast<LogReplayConfiguration*>(config.get()))
    , _worker(new LogReplayWorker(_logReplayConfig))
    , _workerThread(new QThread(this))
{
    // qCDebug(LogReplayLinkLog) << Q_FUNC_INFO << this;

    _workerThread->setObjectName(QStringLiteral("LogReplay_%1").arg(_logReplayConfig->name()));

    _worker->moveToThread(_workerThread);

    (void) connect(_workerThread, &QThread::started, _worker, &LogReplayWorker::setup);
    (void) connect(_workerThread, &QThread::finished, _worker, &QObject::deleteLater);

    (void) connect(_worker, &LogReplayWorker::connected, this, &LogReplayLink::_onConnected, Qt::QueuedConnection);
    (void) connect(_worker, &LogReplayWorker::disconnected, this, &LogReplayLink::_onDisconnected, Qt::QueuedConnection);
    (void) connect(_worker, &LogReplayWorker::errorOccurred, this, &LogReplayLink::_onErrorOccurred, Qt::QueuedConnection);
    (void) connect(_worker, &LogReplayWorker::dataReceived, this, &LogReplayLink::_onDataReceived, Qt::QueuedConnection);

    (void) connect(_worker, &LogReplayWorker::logFileStats, this, &LogReplayLink::logFileStats, Qt::QueuedConnection);
    (void) connect(_worker, &LogReplayWorker::playbackStarted, this, &LogReplayLink::playbackStarted, Qt::QueuedConnection);
    (void) connect(_worker, &LogReplayWorker::playbackPaused, this, &LogReplayLink::playbackPaused, Qt::QueuedConnection);
    (void) connect(_worker, &LogReplayWorker::playbackPercentCompleteChanged, this, &LogReplayLink::playbackPercentCompleteChanged, Qt::QueuedConnection);
    (void) connect(_worker, &LogReplayWorker::currentLogTimeSecs, this, &LogReplayLink::currentLogTimeSecs, Qt::QueuedConnection);
    (void) connect(_worker, &LogReplayWorker::disconnected, this, &LogReplayLink::disconnected, Qt::QueuedConnection);

    _workerThread->start();
}

LogReplayLink::~LogReplayLink()
{
    LogReplayLink::disconnect();

    _workerThread->quit();
    if (!_workerThread->wait()) {
        qCWarning(LogReplayLinkLog) << "Failed to wait for LogReplay Thread to close";
    }

    // qCDebug(LogReplayLinkLog) << Q_FUNC_INFO << this;
}

bool LogReplayLink::_connect()
{
    return QMetaObject::invokeMethod(_worker, "connectToLog", Qt::QueuedConnection);
}

void LogReplayLink::disconnect()
{
    (void) QMetaObject::invokeMethod(_worker, "disconnectFromLog", Qt::QueuedConnection);
}

void LogReplayLink::_onErrorOccurred(const QString &errorString)
{
    qCWarning(LogReplayLinkLog) << "Error:" << errorString;
    emit communicationError(tr("Log Replay Link Error"), tr("Link: %1, %2.").arg(_logReplayConfig->name(), errorString));
}

void LogReplayLink::_onDataReceived(const QByteArray &data)
{
    emit bytesReceived(this, data);
}

void LogReplayLink::play()
{
    (void) QMetaObject::invokeMethod(_worker, "play", Qt::QueuedConnection);
}

void LogReplayLink::pause()
{
    (void) QMetaObject::invokeMethod(_worker, "pause", Qt::QueuedConnection);
}

void LogReplayLink::setPlaybackSpeed(qreal playbackSpeed)
{
    (void) QMetaObject::invokeMethod(_worker, "setPlaybackSpeed", Qt::QueuedConnection, playbackSpeed);
}

void LogReplayLink::movePlayhead(qreal percentComplete)
{
    (void) QMetaObject::invokeMethod(_worker, "movePlayhead", Qt::QueuedConnection, percentComplete);
}
