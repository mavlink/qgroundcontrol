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
#include "QGCApplication.h"
#include "MultiVehicleManager.h"
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#include "MAVLinkProtocol.h"
#else
#include "MAVLinkLib.h"
#endif
#include "QGCLoggingCategory.h"

#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QtEndian>
#include <QtTest/QSignalSpy>

QGC_LOGGING_CATEGORY(LogReplayLinkLog, "qgc.comms.logreplaylink")

/*===========================================================================*/

LogReplayLinkConfiguration::LogReplayLinkConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
{
    // qCDebug(LogReplayLinkLog) << Q_FUNC_INFO << this;
}

LogReplayLinkConfiguration::LogReplayLinkConfiguration(const LogReplayLinkConfiguration *copy, QObject *parent)
    : LinkConfiguration(copy, parent)
    , _logFilename(copy->logFilename())
{
    // qCDebug(LogReplayLinkLog) << Q_FUNC_INFO << this;

    Q_ASSERT(copy);

    LogReplayLinkConfiguration::copyFrom(copy);
}

LogReplayLinkConfiguration::~LogReplayLinkConfiguration()
{
    // qCDebug(LogReplayLinkLog) << Q_FUNC_INFO << this;
}

void LogReplayLinkConfiguration::copyFrom(const LinkConfiguration *source)
{
    Q_ASSERT(source);
    LinkConfiguration::copyFrom(source);

    const LogReplayLinkConfiguration *const logReplaySource = qobject_cast<const LogReplayLinkConfiguration*>(source);
    Q_ASSERT(logReplaySource);

    setLogFilename(logReplaySource->logFilename());
}

QString LogReplayLinkConfiguration::logFilenameShort()
{
    return QFileInfo(_logFilename).fileName();
}

void LogReplayLinkConfiguration::setLogFilename(const QString &logFilename)
{
    if (logFilename != _logFilename) {
        _logFilename = logFilename;
        emit fileNameChanged();
    }
}

void LogReplayLinkConfiguration::loadSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    setLogFilename(settings.value("logFilename", _logFilename).toString());

    settings.endGroup();
}

void LogReplayLinkConfiguration::saveSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    settings.setValue("logFilename", _logFilename);

    settings.endGroup();
}

/*===========================================================================*/

const QString LogReplayLink::_errorTitle = QStringLiteral("Log Replay Link Error");

LogReplayLink::LogReplayLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _logReplayConfig(qobject_cast<LogReplayLinkConfiguration*>(config.get()))
    , _readTickTimer(new QTimer(this))
{
    // qCDebug(LogReplayLinkLog) << Q_FUNC_INFO << this;

    (void) connect(_readTickTimer, &QTimer::timeout, this, &LogReplayLink::_readNextLogEntry);
}

LogReplayLink::~LogReplayLink()
{
    LogReplayLink::disconnect();

    // qCDebug(LogReplayLinkLog) << Q_FUNC_INFO << this;
}

bool LogReplayLink::_connect()
{
    if (MultiVehicleManager::instance()->activeVehicle()) {
        _replayError(tr("You must close all connections prior to replaying a log."));
        return false;
    }

    if (isRunning()) {
        quit();
        wait();
    }

    start(HighPriority);
    return true;
}

void LogReplayLink::disconnect()
{
    if (_connected) {
        quit();
        wait();
        _connected = false;
        emit disconnected();
    }
}

void LogReplayLink::run()
{
    if (!_loadLogFile()) {
        return;
    }

    _connected = true;
    emit connected();

    _play();

    exec();

    (void) QMetaObject::invokeMethod(_readTickTimer, "stop", Qt::AutoConnection);
}

bool LogReplayLink::isPlaying() const
{
    return _readTickTimer->isActive();
}

void LogReplayLink::play()
{
    (void) QMetaObject::invokeMethod(this, "_play", Qt::AutoConnection);
}

void LogReplayLink::pause()
{
    (void) QMetaObject::invokeMethod(this, "_pause", Qt::AutoConnection);
}

void LogReplayLink::setPlaybackSpeed(qreal playbackSpeed)
{
    (void) QMetaObject::invokeMethod(this, "_setPlaybackSpeed", Qt::AutoConnection, playbackSpeed);
}

void LogReplayLink::_replayError(const QString &errorMsg)
{
    emit communicationError(tr("Log Replay Link Error"), tr("Link: %1, %2.").arg(_logReplayConfig->name(), errorMsg));
}

quint64 LogReplayLink::_parseTimestamp(const QByteArray &bytes)
{
    quint64 timestamp = qFromBigEndian(*reinterpret_cast<const quint64*>(bytes.constData()));
    const quint64 currentTimestamp = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch()) * 1000;
    if (timestamp > currentTimestamp) {
        timestamp = qbswap(timestamp);
    }

    return timestamp;
}

quint64 LogReplayLink::_readNextMavlinkMessage(QByteArray &bytes)
{
    bytes.clear();

    mavlink_status_t status;
    char nextByte;
    while (_logFile.getChar(&nextByte)) {
        mavlink_message_t message;
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

quint64 LogReplayLink::_seekToNextMavlinkMessage(mavlink_message_t &nextMsg)
{
    mavlink_reset_channel_status(_mavlinkChannel);

    mavlink_status_t status;
    qint64 messageStartPos = -1;
    char nextByte;
    while (_logFile.getChar(&nextByte)) {
        const bool messageFound = mavlink_parse_char(_mavlinkChannel, nextByte, &nextMsg, &status);

        if (status.parse_state == MAVLINK_PARSE_STATE_GOT_STX) {
            messageStartPos = (_logFile.pos() - 1);
        }

        if (messageFound && (messageStartPos != -1)) {
            (void) _logFile.seek(messageStartPos - kTimestamp);
            const QByteArray rawTime = _logFile.read(kTimestamp);
            return _parseTimestamp(rawTime);
        }
    }

    return 0;
}

quint64 LogReplayLink::_findLastTimestamp()
{
    quint64 lastTimestamp = 0;

    mavlink_reset_channel_status(_mavlinkChannel);

    (void) _logFile.reset();
    while (_logFile.bytesAvailable() > kTimestamp) {
        lastTimestamp = _parseTimestamp(_logFile.read(kTimestamp));

        bool endOfMessage = false;
        char nextByte;
        while (!endOfMessage && _logFile.getChar(&nextByte)) {
            mavlink_message_t msg;
            mavlink_status_t status;
            endOfMessage = mavlink_parse_char(_mavlinkChannel, nextByte, &msg, &status);
        }
    }

    return lastTimestamp;
}

bool LogReplayLink::_loadLogFile()
{
    const QString logFilename = _logReplayConfig->logFilename();
    QFileInfo logFileInfo;
    int logDurationSecondsTotal;
    quint64 startTimeUSecs;
    quint64 endTimeUSecs;

    QString errorMsg;
    if (_logFile.isOpen()) {
        errorMsg = tr("Attempt to load new log while log being played");
        goto Error;
    }

    _logFile.setFileName(logFilename);
    if (!_logFile.open(QFile::ReadOnly)) {
        errorMsg = tr("Unable to open log file: '%1', error: %2").arg(logFilename, _logFile.errorString());
        goto Error;
    }

    logFileInfo.setFile(logFilename);
    _logFileSize = logFileInfo.size();

    startTimeUSecs = _parseTimestamp(_logFile.read(kTimestamp));
    endTimeUSecs = _findLastTimestamp();
    if (endTimeUSecs <= startTimeUSecs) {
        errorMsg = tr("The log file '%1' is corrupt or empty.").arg(logFilename);
        goto Error;
    }

    _logEndTimeUSecs = endTimeUSecs;
    _logStartTimeUSecs = startTimeUSecs;
    _logDurationUSecs = endTimeUSecs - startTimeUSecs;
    _logCurrentTimeUSecs = startTimeUSecs;

    (void) _logFile.reset();

    logDurationSecondsTotal = _logDurationUSecs / 1000000;
    emit logFileStats(logDurationSecondsTotal);

    return true;

Error:
    if (_logFile.isOpen()) {
        _logFile.close();
    }
    _replayError(errorMsg);

    return false;
}

void LogReplayLink::_readNextLogEntry()
{
    int timeToNextExecutionMSecs = 0;
    while (timeToNextExecutionMSecs < 3) {
        QByteArray bytes;
        const qint64 nextTimeUSecs = _readNextMavlinkMessage(bytes);
        emit bytesReceived(this, bytes);
        emit playbackPercentCompleteChanged((static_cast<float>(_logCurrentTimeUSecs - _logStartTimeUSecs) / static_cast<float>(_logDurationUSecs)) * 100);

        if (_logFile.atEnd()) {
            _finishPlayback();
            return;
        }

        _logCurrentTimeUSecs = nextTimeUSecs;

        const quint64 currentTimeMSecs = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
        const quint64 desiredPlayheadMovementTimeMSecs = ((_logCurrentTimeUSecs - _playbackStartLogTimeUSecs) / 1000) / _playbackSpeed;
        const quint64 desiredCurrentTimeMSecs = _playbackStartTimeMSecs + desiredPlayheadMovementTimeMSecs;
        timeToNextExecutionMSecs = desiredCurrentTimeMSecs - currentTimeMSecs;
    }

    _signalCurrentLogTimeSecs();

    (void) QMetaObject::invokeMethod(_readTickTimer, "start", Qt::AutoConnection, timeToNextExecutionMSecs);
}

void LogReplayLink::_play()
{
    LinkManager::instance()->setConnectionsSuspended(tr("Connect not allowed during Flight Data replay."));
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    MAVLinkProtocol::instance()->suspendLogForReplay(true);
#endif

    if (_logFile.atEnd()) {
        _resetPlaybackToBeginning();
    }

    _playbackStartTimeMSecs = (quint64)QDateTime::currentMSecsSinceEpoch();
    _playbackStartLogTimeUSecs = _logCurrentTimeUSecs;
    (void) QMetaObject::invokeMethod(_readTickTimer, "start", Qt::AutoConnection, 1);

    emit playbackStarted();
}

void LogReplayLink::_pause()
{
    LinkManager::instance()->setConnectionsAllowed();
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    MAVLinkProtocol::instance()->suspendLogForReplay(false);
#endif

    (void) QMetaObject::invokeMethod(_readTickTimer, "stop", Qt::AutoConnection);

    emit playbackPaused();
}

void LogReplayLink::_resetPlaybackToBeginning()
{
    if (_logFile.isOpen()) {
        _logFile.reset();
    }

    _playbackStartTimeMSecs = 0;
    _playbackStartLogTimeUSecs = 0;
    _logCurrentTimeUSecs = _logStartTimeUSecs;
}

void LogReplayLink::movePlayhead(qreal percentComplete)
{
    if (isPlaying()) {
        pause();
        QSignalSpy waitForPause(this, SIGNAL(playbackPaused()));
        (void) waitForPause.wait();
        if (_readTickTimer->isActive()) {
            return;
        }
    }

    percentComplete = qBound(0., percentComplete, 100.);
    const qreal percentCompleteMult = percentComplete / 100.0;
    const qint64 newFilePos = static_cast<qint64>(percentCompleteMult * static_cast<qreal>(_logFile.size()));
    if (!_logFile.seek(newFilePos)) {
        _replayError(tr("Unable to seek to new position"));
        return;
    }

    mavlink_message_t dummy{};
    _logCurrentTimeUSecs = _seekToNextMavlinkMessage(dummy);
    qreal newRelativeTimeUSecs = static_cast<qreal>(_logCurrentTimeUSecs - _logStartTimeUSecs);
    const qreal baudRate = _logFile.size() / static_cast<qreal>(_logDurationUSecs) / 1e6;
    const qreal desiredTimeUSecs = percentCompleteMult * _logDurationUSecs;
    const qint64 offset = (newRelativeTimeUSecs - desiredTimeUSecs) * baudRate;
    if (!_logFile.seek(_logFile.pos() + offset)) {
        _replayError(tr("Unable to seek to new position"));
        return;
    }

    _logCurrentTimeUSecs = _seekToNextMavlinkMessage(dummy);
    _signalCurrentLogTimeSecs();

    newRelativeTimeUSecs = static_cast<qreal>(_logCurrentTimeUSecs - _logStartTimeUSecs);
    percentComplete = ((newRelativeTimeUSecs / _logDurationUSecs) * 100);
    emit playbackPercentCompleteChanged(percentComplete);
}

void LogReplayLink::_setPlaybackSpeed(qreal playbackSpeed)
{
    _playbackSpeed = playbackSpeed;
    _playbackStartTimeMSecs = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    _playbackStartLogTimeUSecs = _logCurrentTimeUSecs;
    _readTickTimer->start(1);
}

void LogReplayLink::_finishPlayback()
{
    _pause();

    emit playbackAtEnd();
}

void LogReplayLink::_signalCurrentLogTimeSecs()
{
    emit currentLogTimeSecs((_logCurrentTimeUSecs - _logStartTimeUSecs) / 1000000);
}
