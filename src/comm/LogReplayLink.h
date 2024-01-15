/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MAVLinkProtocol.h"

#include <QTimer>
#include <QFile>

class LinkManager;

class LogReplayLinkConfiguration : public LinkConfiguration
{
    Q_OBJECT

public:
    Q_PROPERTY(QString  fileName    READ logFilename    WRITE setLogFilename    NOTIFY fileNameChanged)

    LogReplayLinkConfiguration(const QString& name);
    LogReplayLinkConfiguration(LogReplayLinkConfiguration* copy);

    QString logFilename(void) { return _logFilename; }
    void setLogFilename(const QString logFilename) { _logFilename = logFilename; emit fileNameChanged(); }

    QString logFilenameShort(void);

    // Virtuals from LinkConfiguration
    LinkType    type                    (void) override                                         { return LinkConfiguration::TypeLogReplay; }
    void        copyFrom                (LinkConfiguration* source) override;
    void        loadSettings            (QSettings& settings, const QString& root) override;
    void        saveSettings            (QSettings& settings, const QString& root) override;
    QString     settingsURL             (void) override                                         { return "LogReplaySettings.qml"; }
    QString     settingsTitle           (void) override                                          { return tr("Log Replay Link Settings"); }

signals:
    void fileNameChanged();

private:
    static const char*  _logFilenameKey;
    QString             _logFilename;
};

/// Pseudo link that reads a telemetry log and feeds it into the application.
class LogReplayLink : public LinkInterface
{
    Q_OBJECT

public:
    LogReplayLink(SharedLinkConfigurationPtr& config);
    virtual ~LogReplayLink();

    /// @return true: log is currently playing, false: log playback is paused
    bool isPlaying(void) { return _readTickTimer.isActive(); }

    void play           (void) { emit _playOnThread(); }
    void pause          (void) { emit _pauseOnThread(); }
    void movePlayhead   (qreal percentComplete);

    // overrides from LinkInterface
    bool isConnected(void) const override { return _connected; }
    bool isLogReplay(void) override { return true; }
    void disconnect (void) override;

public slots:
    /// Sets the acceleration factor: -100: 0.01X, 0: 1.0X, 100: 100.0X
    void setPlaybackSpeed(qreal playbackSpeed) { emit _setPlaybackSpeedOnThread(playbackSpeed); }

signals:
    void logFileStats                   (int logDurationSecs);
    void playbackStarted                (void);
    void playbackPaused                 (void);
    void playbackAtEnd                  (void);
    void playbackPercentCompleteChanged (qreal percentComplete);
    void currentLogTimeSecs             (int secs);

    // Internal signals
    void _playOnThread              (void);
    void _pauseOnThread             (void);
    void _setPlaybackSpeedOnThread  (qreal playbackSpeed);

private slots:
    // LinkInterface overrides
    void _writeBytes(const QByteArray bytes) override;

    void _readNextLogEntry  (void);
    void _play              (void);
    void _pause             (void);
    void _setPlaybackSpeed  (qreal playbackSpeed);

private:

    // LinkInterface overrides
    bool _connect(void) override;

    void    _replayError                (const QString& errorMsg);
    quint64 _parseTimestamp             (const QByteArray& bytes);
    quint64 _seekToNextMavlinkMessage   (mavlink_message_t* nextMsg);
    quint64 _findLastTimestamp          (void);
    quint64 _readNextMavlinkMessage     (QByteArray& bytes);
    bool    _loadLogFile                (void);
    void    _finishPlayback             (void);
    void    _resetPlaybackToBeginning   (void);
    void    _signalCurrentLogTimeSecs   (void);

    // QThread overrides
    void run(void) override;

    LogReplayLinkConfiguration* _logReplayConfig;

    bool    _connected;
    uint8_t _mavlinkChannel;
    QTimer  _readTickTimer;      ///< Timer which signals a read of next log record

    QString _errorTitle; ///< Title for communicatorError signals

    quint64 _logCurrentTimeUSecs;   ///< The timestamp of the next message in the log file.
    quint64 _logStartTimeUSecs;     ///< The first timestamp in the current log file.
    quint64 _logEndTimeUSecs;       ///< The last timestamp in the current log file.
    quint64 _logDurationUSecs;

    qreal   _playbackSpeed;
    quint64 _playbackStartTimeMSecs;    ///< The time when the logfile was first played back. This is used to pace out replaying the messages to fix long-term drift/skew. 0 indicates that the player hasn't initiated playback of this log file.
    quint64 _playbackStartLogTimeUSecs;

    MAVLinkProtocol*    _mavlink;
    QFile               _logFile;
    quint64             _logFileSize;

    static const int cbTimestamp = sizeof(quint64);
};

class LogReplayLinkController : public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY(LogReplayLink*   link            READ link               WRITE setLink               NOTIFY linkChanged)
    Q_PROPERTY(bool             isPlaying       READ isPlaying          WRITE setIsPlaying          NOTIFY isPlayingChanged)
    Q_PROPERTY(qreal            percentComplete READ percentComplete    WRITE setPercentComplete    NOTIFY percentCompleteChanged)
    Q_PROPERTY(QString          totalTime       MEMBER _totalTime                                   NOTIFY totalTimeChanged)
    Q_PROPERTY(QString          playheadTime    MEMBER _playheadTime                                NOTIFY playheadTimeChanged)
    Q_PROPERTY(qreal            playbackSpeed   MEMBER _playbackSpeed                               NOTIFY playbackSpeedChanged)

    LogReplayLinkController(void);

    LogReplayLink*  link            (void) { return _link; }
    bool            isPlaying       (void) const{ return _isPlaying; }
    qreal           percentComplete (void) const{ return _percentComplete; }

    void setLink            (LogReplayLink* link);
    void setIsPlaying       (bool isPlaying);
    void setPercentComplete (qreal percentComplete);

signals:
    void linkChanged            (LogReplayLink* link);
    void isPlayingChanged       (bool isPlaying);
    void percentCompleteChanged (qreal percentComplete);
    void playheadTimeChanged    (QString playheadTime);
    void totalTimeChanged       (QString totalTime);
    void playbackSpeedChanged   (qreal playbackSpeed);

private slots:
    void _logFileStats                   (int logDurationSecs);
    void _playbackStarted                (void);
    void _playbackPaused                 (void);
    void _playbackAtEnd                  (void);
    void _playbackPercentCompleteChanged (qreal percentComplete);
    void _currentLogTimeSecs             (int secs);
    void _linkDisconnected               (void);

private:
    QString _secondsToHMS(int seconds);

    LogReplayLink*  _link;
    bool            _isPlaying;
    qreal           _percentComplete;
    int             _playheadSecs;
    QString         _playheadTime;
    QString         _totalTime;
    qreal           _playbackSpeed;
};

