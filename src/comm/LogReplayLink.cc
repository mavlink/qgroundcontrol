/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "LogReplayLink.h"
#include "LinkManager.h"
#include "QGCApplication.h"

#include <QFileInfo>
#include <QtEndian>

const char*  LogReplayLinkConfiguration::_logFilenameKey = "logFilename";

const char* LogReplayLink::_errorTitle = "Log Replay Error";

LogReplayLinkConfiguration::LogReplayLinkConfiguration(const QString& name) :
LinkConfiguration(name)
{
    
}

LogReplayLinkConfiguration::LogReplayLinkConfiguration(LogReplayLinkConfiguration* copy) :
LinkConfiguration(copy)
{
    _logFilename = copy->logFilename();
}

void LogReplayLinkConfiguration::copyFrom(LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);
    LogReplayLinkConfiguration* ssource = dynamic_cast<LogReplayLinkConfiguration*>(source);
    Q_ASSERT(ssource != NULL);
    _logFilename = ssource->logFilename();
}

void LogReplayLinkConfiguration::saveSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    settings.setValue(_logFilenameKey, _logFilename);
    settings.endGroup();
}

void LogReplayLinkConfiguration::loadSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    _logFilename = settings.value(_logFilenameKey, "").toString();
    settings.endGroup();
}

void LogReplayLinkConfiguration::updateSettings(void)
{
    // Doesn't support changing filename on the fly is already connected
}

QString LogReplayLinkConfiguration::logFilenameShort(void)
{
    QFileInfo fi(_logFilename);
    return fi.fileName();
}

LogReplayLink::LogReplayLink(SharedLinkConfigurationPointer& config)
    : LinkInterface(config)
    , _logReplayConfig(qobject_cast<LogReplayLinkConfiguration*>(config.data()))
    , _connected(false)
    , _replayAccelerationFactor(1.0f)
{
    Q_ASSERT(_logReplayConfig);
    
    _readTickTimer.moveToThread(this);
    
	QObject::connect(&_readTickTimer, &QTimer::timeout, this, &LogReplayLink::_readNextLogEntry);
    QObject::connect(this, &LogReplayLink::_playOnThread, this, &LogReplayLink::_play);
    QObject::connect(this, &LogReplayLink::_pauseOnThread, this, &LogReplayLink::_pause);
    QObject::connect(this, &LogReplayLink::_setAccelerationFactorOnThread, this, &LogReplayLink::_setAccelerationFactor);
    
    moveToThread(this);
}

LogReplayLink::~LogReplayLink(void)
{
    _disconnect();
}

bool LogReplayLink::_connect(void)
{
    // Disallow replay when any links are connected
    if (qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()) {
        emit communicationError(_errorTitle, "You must close all connections prior to replaying a log.");
        return false;
    }

    if (isRunning()) {
        quit();
        wait();
    }
    start(HighPriority);
    return true;
}

void LogReplayLink::_disconnect(void)
{
    if (_connected) {
        quit();
        wait();
        _connected = false;
        emit disconnected();
    }
}

void LogReplayLink::run(void)
{
    // Load the log file
    if (!_loadLogFile()) {
        return;
    }
    
    _connected = true;
    emit connected();
    
    // Start playback
    _play();

    // Run normal event loop until exit
    exec();
    
    _readTickTimer.stop();
}

void LogReplayLink::_replayError(const QString& errorMsg)
{
    qDebug() << _errorTitle << errorMsg;
    emit communicationError(_errorTitle, errorMsg);
}

/// Since this is log replay, we just drops writes on the floor
void LogReplayLink::_writeBytes(const QByteArray bytes)
{
    Q_UNUSED(bytes);
}

/// Parses a BigEndian quint64 timestamp
/// @return A Unix timestamp in microseconds UTC for found message or 0 if parsing failed
quint64 LogReplayLink::_parseTimestamp(const QByteArray& bytes)
{
    quint64 timestamp = qFromBigEndian(*((quint64*)(bytes.constData())));
    quint64 currentTimestamp = ((quint64)QDateTime::currentMSecsSinceEpoch()) * 1000;
    
    // Now if the parsed timestamp is in the future, it must be an old file where the timestamp was stored as
    // little endian, so switch it.
    if (timestamp > currentTimestamp) {
        timestamp = qbswap(timestamp);
    }
    
    return timestamp;
}

/// Seeks to the beginning of the next successfully parsed mavlink message in the log file.
///     @param nextMsg[output] Parsed next message that was found
/// @return A Unix timestamp in microseconds UTC for found message or 0 if parsing failed
quint64 LogReplayLink::_seekToNextMavlinkMessage(mavlink_message_t* nextMsg)
{
    char nextByte;
    mavlink_status_t comm;
    while (_logFile.getChar(&nextByte)) { // Loop over every byte
        bool messageFound = mavlink_parse_char(mavlinkChannel(), nextByte, nextMsg, &comm);
        
        // If we've found a message, jump back to the start of the message, grab the timestamp,
        // and go back to the end of this file.
        if (messageFound) {
            _logFile.seek(_logFile.pos() - (nextMsg->len + MAVLINK_NUM_NON_PAYLOAD_BYTES + cbTimestamp));
            QByteArray rawTime = _logFile.read(cbTimestamp);
            return _parseTimestamp(rawTime);
        }
    }
    
    return 0;
}

bool LogReplayLink::_loadLogFile(void)
{
    QString errorMsg;
    QString logFilename = _logReplayConfig->logFilename();
    QFileInfo logFileInfo;
    int logDurationSecondsTotal;
    
    if (_logFile.isOpen()) {
        errorMsg = "Attempt to load new log while log being played";
        goto Error;
    }
    
    _logFile.setFileName(logFilename);
    if (!_logFile.open(QFile::ReadOnly)) {
        errorMsg = QString("Unable to open log file: '%1', error: %2").arg(logFilename).arg(_logFile.errorString());
        goto Error;
    }
    logFileInfo.setFile(logFilename);
    _logFileSize = logFileInfo.size();
    
    _logTimestamped = logFilename.endsWith(".mavlink");
    
    if (_logTimestamped) {
        // Get the first timestamp from the log
        // This should be a big-endian uint64.
        QByteArray timestamp = _logFile.read(cbTimestamp);
        quint64 startTimeUSecs = _parseTimestamp(timestamp);
        
        // Now find the last timestamp by scanning for the last MAVLink packet and
        // find the timestamp before it. To do this we start searchin a little before
        // the end of the file, specifically the maximum MAVLink packet size + the
        // timestamp size. This guarantees that we will hit a MAVLink packet before
        // the end of the file. Unfortunately, it basically guarantees that we will
        // hit more than one. This is why we have to search for a bit.
        qint64 fileLoc = _logFile.size() - MAVLINK_MAX_PACKET_LEN - cbTimestamp;
        _logFile.seek(fileLoc);
        quint64 endTimeUSecs = startTimeUSecs; // Set a sane default for the endtime
        mavlink_message_t msg;
        quint64 messageTimeUSecs;
        while ((messageTimeUSecs = _seekToNextMavlinkMessage(&msg)) > endTimeUSecs) {
            endTimeUSecs = messageTimeUSecs;
        }
        
        if (endTimeUSecs == startTimeUSecs) {
            errorMsg = QString("The log file '%1' is corrupt. No valid timestamps were found at the end of the file.").arg(logFilename);
            goto Error;
        }
        
        // Remember the start and end time so we can move around this _logFile with the slider.
        _logEndTimeUSecs = endTimeUSecs;
        _logStartTimeUSecs = startTimeUSecs;
        _logDurationUSecs = endTimeUSecs - startTimeUSecs;
        _logCurrentTimeUSecs = startTimeUSecs;
        
        // Reset our log file so when we go to read it for the first time, we start at the beginning.
        _logFile.reset();
        
        logDurationSecondsTotal = (_logDurationUSecs) / 1000000;
    } else {
        // Load in binary mode. In this mode, files should be have a filename postfix
        // of the baud rate they were recorded at, like `test_run_115200.bin`. Then on
        // playback, the datarate is equal to set to this value.
        
        
        // Set baud rate if any present. Otherwise we default to 57600.
        QStringList parts = logFileInfo.baseName().split("_");
        _binaryBaudRate = _defaultBinaryBaudRate;
        if (parts.count() > 1)
        {
            bool ok;
            int rate = parts.last().toInt(&ok);
            // 9600 baud to 100 MBit
            if (ok && (rate > 9600 && rate < 100000000))
            {
                // Accept this as valid baudrate
                _binaryBaudRate = rate;
            }
        }
        
        logDurationSecondsTotal = logFileInfo.size() / (_binaryBaudRate / 10);
    }
    
    emit logFileStats(_logTimestamped, logDurationSecondsTotal, _binaryBaudRate);
    
    return true;
    
Error:
    if (_logFile.isOpen()) {
        _logFile.close();
    }
    _replayError(errorMsg);
    return false;
}

/// This function will read the next available log entry. It will then start
/// the _readTickTimer timer to read the new log entry at the appropriate time.
/// It might not perfectly match the timing of the log file, but it will never
/// induce a static drift into the log file replay.
void LogReplayLink::_readNextLogEntry(void)
{
    // If we have a file with timestamps, try and pace this out following the time differences
    // between the timestamps and the current playback speed.
    if (_logTimestamped) {
        // Now parse MAVLink messages, grabbing their timestamps as we go. We stop once we
        // have at least 3ms until the next one.
        
        // We track what the next execution time should be in milliseconds, which we use to set
        // the next timer interrupt.
        int timeToNextExecutionMSecs = 0;
        
        // We use the `_seekToNextMavlinkMessage()` function to scan ahead for MAVLink messages. This
        // is necessary because we don't know how big each MAVLink message is until we finish parsing
        // one, and since we only output arrays of bytes, we need to know the size of that array.
        mavlink_message_t msg;
        _seekToNextMavlinkMessage(&msg);
        
        while (timeToNextExecutionMSecs < 3) {
            
            // Now we're sitting at the start of a MAVLink message, so read it all into a byte array for feeding to our parser.
            QByteArray message = _logFile.read(msg.len + MAVLINK_NUM_NON_PAYLOAD_BYTES);
            
            emit bytesReceived(this, message);            
            emit playbackPercentCompleteChanged(((float)(_logCurrentTimeUSecs - _logStartTimeUSecs) / (float)_logDurationUSecs) * 100);
            
            // If we've reached the end of the of the file, make sure we handle that well
            if (_logFile.atEnd()) {
                _finishPlayback();
                return;
            }
            
            // Run our parser to find the next timestamp and leave us at the start of the next MAVLink message.
            _logCurrentTimeUSecs = _seekToNextMavlinkMessage(&msg);
            
            // Calculate how long we should wait in real time until parsing this message.
            // We pace ourselves relative to the start time of playback to fix any drift (initially set in play())
            qint64 timeDiffMSecs = ((_logCurrentTimeUSecs - _logStartTimeUSecs) / 1000) / _replayAccelerationFactor;
            quint64 desiredPacedTimeMSecs = _playbackStartTimeMSecs + timeDiffMSecs;
            quint64 currentTimeMSecs = (quint64)QDateTime::currentMSecsSinceEpoch();
            timeToNextExecutionMSecs = desiredPacedTimeMSecs - currentTimeMSecs;
        }
        
        // And schedule the next execution of this function.
        _readTickTimer.start(timeToNextExecutionMSecs);
    }
    else
    {
        // Binary format - read at fixed rate
        const int len = 100;
        QByteArray chunk = _logFile.read(len);
        
        emit bytesReceived(this, chunk);
        emit playbackPercentCompleteChanged(((float)_logFile.pos() / (float)_logFileSize) * 100);
        
        // Check if reached end of file before reading next timestamp
        if (chunk.length() < len || _logFile.atEnd())
        {
            _finishPlayback();
            return;
        }
    }
    
}

void LogReplayLink::_play(void)
{
    qgcApp()->toolbox()->linkManager()->setConnectionsSuspended(tr("Connect not allowed during Flight Data replay."));
#ifndef __mobile__
    qgcApp()->toolbox()->mavlinkProtocol()->suspendLogForReplay(true);
#endif
    
    // Make sure we aren't at the end of the file, if we are, reset to the beginning and play from there.
    if (_logFile.atEnd()) {
        _resetPlaybackToBeginning();
    }
    
    // Always correct the current start time such that the next message will play immediately at playback.
    // We do this by subtracting the current file playback offset  from now()
    _playbackStartTimeMSecs = (quint64)QDateTime::currentMSecsSinceEpoch() - ((_logCurrentTimeUSecs - _logStartTimeUSecs) / 1000);
    
    // Start timer
    if (_logTimestamped) {
        _readTickTimer.start(1);
    } else {
        // Read len bytes at a time
        int len = 100;
        // Calculate the number of times to read 100 bytes per second
        // to guarantee the baud rate, then divide 1000 by the number of read
        // operations to obtain the interval in milliseconds
        int interval = 1000 / ((_binaryBaudRate / 10) / len);
        _readTickTimer.start(interval / _replayAccelerationFactor);
    }
    
    emit playbackStarted();
}

void LogReplayLink::_pause(void)
{
    qgcApp()->toolbox()->linkManager()->setConnectionsAllowed();
#ifndef __mobile__
    qgcApp()->toolbox()->mavlinkProtocol()->suspendLogForReplay(false);
#endif
    
    _readTickTimer.stop();
    
    emit playbackPaused();
}

void LogReplayLink::_resetPlaybackToBeginning(void)
{
    if (_logFile.isOpen()) {
        _logFile.reset();
    }
    
    // And since we haven't starting playback, clear the time of initial playback and the current timestamp.
    _playbackStartTimeMSecs = 0;
    _logCurrentTimeUSecs = _logStartTimeUSecs;
}

void LogReplayLink::movePlayhead(int percentComplete)
{
    if (isPlaying()) {
        qWarning() << "Should not move playhead while playing, pause first";
        return;
    }

    if (percentComplete < 0 || percentComplete > 100) {
        qWarning() << "Bad percentage value" << percentComplete;
        return;
    }
    
    float floatPercentComplete = (float)percentComplete / 100.0f;
    
    if (_logTimestamped) {
        // But if we have a timestamped MAVLink log, then actually aim to hit that percentage in terms of
        // time through the file.
        qint64 newFilePos = (qint64)(floatPercentComplete * (float)_logFile.size());
        
        // Now seek to the appropriate position, failing gracefully if we can't.
        if (!_logFile.seek(newFilePos)) {
            _replayError("Unable to seek to new position");
            return;
        }
        
        // But we do align to the next MAVLink message for consistency.
        mavlink_message_t dummy;
        _logCurrentTimeUSecs = _seekToNextMavlinkMessage(&dummy);
        
        // Now calculate the current file location based on time.
        float newRelativeTimeUSecs = (float)(_logCurrentTimeUSecs - _logStartTimeUSecs);
        
        // Calculate the effective baud rate of the file in bytes/s.
        float baudRate = _logFile.size() / (float)_logDurationUSecs / 1e6;
        
        // And the desired time is:
        float desiredTimeUSecs = floatPercentComplete * _logDurationUSecs;
        
        // And now jump the necessary number of bytes in the proper direction
        qint64 offset = (newRelativeTimeUSecs - desiredTimeUSecs) * baudRate;
        if (!_logFile.seek(_logFile.pos() + offset)) {
            _replayError("Unable to seek to new position");
            return;
        }
        
        // And scan until we reach the start of a MAVLink message. We make sure to record this timestamp for
        // smooth jumping around the file.
        _logCurrentTimeUSecs = _seekToNextMavlinkMessage(&dummy);
        
        // Now update the UI with our actual final position.
        newRelativeTimeUSecs = (float)(_logCurrentTimeUSecs - _logStartTimeUSecs);
        percentComplete = (newRelativeTimeUSecs / _logDurationUSecs) * 100;
        emit playbackPercentCompleteChanged(percentComplete);
    } else {
        // If we're working with a non-timestamped file, we just jump to that percentage of the file,
        // align to the next MAVLink message and roll with it. No reason to do anything more complicated.
        qint64 newFilePos = (qint64)(floatPercentComplete * (float)_logFile.size());
        
        // Now seek to the appropriate position, failing gracefully if we can't.
        if (!_logFile.seek(newFilePos)) {
            _replayError("Unable to seek to new position");
            return;
        }
        
        // But we do align to the next MAVLink message for consistency.
        mavlink_message_t dummy;
        _seekToNextMavlinkMessage(&dummy);
    }
}

void LogReplayLink::_setAccelerationFactor(int factor)
{
    // factor: -100: 0.01X, 0: 1.0X, 100: 100.0X
    
    if (factor < 0) {
        _replayAccelerationFactor = 0.01f;
        factor -= -100;
        if (factor > 0) {
            _replayAccelerationFactor *= (float)factor;
        }
    } else if (factor > 0) {
        _replayAccelerationFactor = 1.0f * (float)factor;
    } else {
        _replayAccelerationFactor = 1.0f;
    }
    
    // Update timer interval
    if (!_logTimestamped) {
        // Read len bytes at a time
        int len = 100;
        // Calculate the number of times to read 100 bytes per second
        // to guarantee the baud rate, then divide 1000 by the number of read
        // operations to obtain the interval in milliseconds
        int interval = 1000 / ((_binaryBaudRate / 10) / len);
        _readTickTimer.stop();
        _readTickTimer.start(interval / _replayAccelerationFactor);
    }
}

/// @brief Called when playback is complete
void LogReplayLink::_finishPlayback(void)
{
    _pause();
    
    emit playbackAtEnd();
}

/// @brief Called when an error occurs during playback to reset playback system state.
void LogReplayLink::_playbackError(void)
{
    _pause();
    _logFile.close();
    emit playbackError();
}
