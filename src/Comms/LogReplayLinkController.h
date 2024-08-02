/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>

#include "LogReplayLink.h"

Q_DECLARE_LOGGING_CATEGORY(LogReplayLinkControllerLog)

class LogReplayLinkController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(LogReplayLink    *link           READ    link            WRITE setLink               NOTIFY linkChanged)
    Q_PROPERTY(bool             isPlaying       READ    isPlaying       WRITE setIsPlaying          NOTIFY isPlayingChanged)
    Q_PROPERTY(qreal            percentComplete READ    percentComplete WRITE setPercentComplete    NOTIFY percentCompleteChanged)
    Q_PROPERTY(QString          totalTime       MEMBER  _totalTime                                  NOTIFY totalTimeChanged)
    Q_PROPERTY(QString          playheadTime    MEMBER  _playheadTime                               NOTIFY playheadTimeChanged)
    Q_PROPERTY(qreal            playbackSpeed   MEMBER  _playbackSpeed                              NOTIFY playbackSpeedChanged)

public:
    explicit LogReplayLinkController(QObject *parent = nullptr);
    ~LogReplayLinkController();

    LogReplayLink *link() const { return _link; }
    void setLink(LogReplayLink *link);

    bool isPlaying() const { return _isPlaying; }
    void setIsPlaying(bool isPlaying) const;

    qreal percentComplete() const { return _percentComplete; }
    void setPercentComplete(qreal percentComplete) const;

signals:
    void isPlayingChanged(bool isPlaying);
    void linkChanged(LogReplayLink *link);
    void percentCompleteChanged(qreal percentComplete);
    void playbackSpeedChanged(qreal playbackSpeed);
    void playheadTimeChanged(const QString &playheadTime);
    void totalTimeChanged(const QString &totalTime);

private slots:
    void _currentLogTimeSecs(uint32_t secs);
    void _linkDisconnected() { setLink(nullptr); }
    void _logFileStats(uint32_t logDurationSecs);
    void _playbackAtEnd();
    void _playbackPaused();
    void _playbackPercentCompleteChanged(qreal percentComplete);
    void _playbackStarted();

private:
    static QString _secondsToHMS(uint32_t seconds);

    bool _isPlaying = false;
    qreal _percentComplete = 0;
    uint32_t _playheadSecs = 0;
    qreal _playbackSpeed = 1;
    QString _playheadTime;
    QString _totalTime;
    LogReplayLink *_link = nullptr;
};
