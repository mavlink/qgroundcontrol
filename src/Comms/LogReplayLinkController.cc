/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LogReplayLinkController.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(LogReplayLinkControllerLog, "qgc.comms.logreplaylink")

LogReplayLinkController::LogReplayLinkController(QObject *parent)
    : QObject(parent)
{
    // qCDebug(LogReplayLinkControllerLog) << Q_FUNC_INFO << this;
}

LogReplayLinkController::~LogReplayLinkController()
{
    // qCDebug(LogReplayLinkControllerLog) << Q_FUNC_INFO << this;
}

void LogReplayLinkController::setLink(LogReplayLink *link)
{
    if (_link) {
        (void) disconnect(_link);
        (void) disconnect(this, &LogReplayLinkController::playbackSpeedChanged, _link, &LogReplayLink::setPlaybackSpeed);

        _isPlaying = false;
        emit isPlayingChanged(_isPlaying);

        _percentComplete = 0;
        emit percentCompleteChanged(_percentComplete);

        _playheadTime.clear();
        emit playheadTimeChanged(_playheadTime);

        _totalTime.clear();
        emit totalTimeChanged(_totalTime);

        _link = nullptr;
        emit linkChanged(_link);
    }

    if (link) {
        _link = link;

        (void) connect(_link, &LogReplayLink::logFileStats, this, &LogReplayLinkController::_logFileStats);
        (void) connect(_link, &LogReplayLink::playbackStarted, this, &LogReplayLinkController::_playbackStarted);
        (void) connect(_link, &LogReplayLink::playbackPaused, this, &LogReplayLinkController::_playbackPaused);
        (void) connect(_link, &LogReplayLink::playbackPercentCompleteChanged, this, &LogReplayLinkController::_playbackPercentCompleteChanged);
        (void) connect(_link, &LogReplayLink::currentLogTimeSecs, this, &LogReplayLinkController::_currentLogTimeSecs);
        (void) connect(_link, &LogReplayLink::disconnected, this, &LogReplayLinkController::_linkDisconnected);

        (void) connect(this, &LogReplayLinkController::playbackSpeedChanged, _link, &LogReplayLink::setPlaybackSpeed);

        emit linkChanged(_link);
    }
}

void LogReplayLinkController::setIsPlaying(bool isPlaying) const
{
    if (isPlaying) {
        _link->play();
    } else {
        _link->pause();
    }
}

void LogReplayLinkController::setPercentComplete(qreal percentComplete) const
{
    _link->movePlayhead(percentComplete);
}

void LogReplayLinkController::_logFileStats(uint32_t logDurationSecs)
{
    const QString totalTime = _secondsToHMS(logDurationSecs);
    if (totalTime != _totalTime) {
        _totalTime = totalTime;
        emit totalTimeChanged(_totalTime);
    }
}

void LogReplayLinkController::_playbackStarted()
{
    if (!_isPlaying) {
        _isPlaying = true;
        emit isPlayingChanged(_isPlaying);
    }
}

void LogReplayLinkController::_playbackPaused()
{
    if (_isPlaying) {
        _isPlaying = false;
        emit isPlayingChanged(_isPlaying);
    }
}

void LogReplayLinkController::_playbackAtEnd()
{
    if (_isPlaying) {
        _isPlaying = false;
        emit isPlayingChanged(_isPlaying);
    }
}

void LogReplayLinkController::_playbackPercentCompleteChanged(qreal percentComplete)
{
    if (percentComplete != _percentComplete) {
        _percentComplete = percentComplete;
        emit percentCompleteChanged(_percentComplete);
    }
}

void LogReplayLinkController::_currentLogTimeSecs(uint32_t secs)
{
    if (secs != _playheadSecs) {
        _playheadSecs = secs;
        _playheadTime = _secondsToHMS(secs);
        emit playheadTimeChanged(_playheadTime);
    }
}

QString LogReplayLinkController::_secondsToHMS(uint32_t seconds)
{
    uint32_t secondsPart = seconds;
    uint32_t minutesPart = secondsPart / 60;
    const uint32_t hoursPart = minutesPart / 60;
    secondsPart -= (60 * minutesPart);
    minutesPart -= (60 * hoursPart);

    QString result = QStringLiteral("%2m:%3s").arg(minutesPart, 2, 10, QLatin1Char('0')).arg(secondsPart, 2, 10, QLatin1Char('0'));
    if (hoursPart != 0) {
        (void) result.prepend(QStringLiteral("%1h:").arg(hoursPart, 2, 10, QLatin1Char('0')));
    }

    return result;
}
