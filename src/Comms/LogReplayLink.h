/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QFile>

#include "LinkConfiguration.h"
#include "LinkInterface.h"

Q_DECLARE_LOGGING_CATEGORY(LogReplayLinkLog)

typedef struct __mavlink_message mavlink_message_t;

class QTimer;

typedef struct __mavlink_message mavlink_message_t;

class LogReplayLinkConfiguration : public LinkConfiguration
{
    Q_OBJECT

    Q_PROPERTY(QString fileName READ logFilename WRITE setLogFilename NOTIFY fileNameChanged)

public:
    explicit LogReplayLinkConfiguration(const QString &name, QObject *parent = nullptr);
    explicit LogReplayLinkConfiguration(const LogReplayLinkConfiguration *copy, QObject *parent = nullptr);
    virtual ~LogReplayLinkConfiguration();

    QString logFilenameShort();
    QString logFilename() const { return _logFilename; }
    void setLogFilename(const QString &logFilename);

    LinkType type() const override { return LinkConfiguration::TypeLogReplay; }
    void copyFrom(LinkConfiguration *source) override;
    void loadSettings(QSettings &settings, const QString &root) override;
    void saveSettings(QSettings &settings, const QString &root) override;
    QString settingsURL() override { return QStringLiteral("LogReplaySettings.qml"); }
    QString settingsTitle() override { return QStringLiteral("Log Replay Link Settings"); }

signals:
    void fileNameChanged();

private:
    QString _logFilename;
};

/*===========================================================================*/

class LogReplayLink : public LinkInterface
{
    Q_OBJECT

public:
    explicit LogReplayLink(SharedLinkConfigurationPtr &config, QObject *parent = nullptr);
    virtual ~LogReplayLink();

    bool isPlaying() const;
    void play();
    void pause();
    void setPlaybackSpeed(qreal playbackSpeed);
    void movePlayhead(qreal percentComplete);

    void run() override {}
    bool isConnected() const override { return _connected; }
    void disconnect() override;
    bool isLogReplay() const override { return true; }

signals:
    void logFileStats(uint32_t logDurationSecs);
    void playbackStarted();
    void playbackPaused();
    void playbackAtEnd();
    void playbackPercentCompleteChanged(qreal percentComplete);
    void currentLogTimeSecs(uint32_t secs);

private slots:
    void _writeBytes(const QByteArray &bytes) override { Q_UNUSED(bytes); }
    void _play();
    void _pause();
    void _setPlaybackSpeed(qreal playbackSpeed);
    void _readNextLogEntry();

private:
    bool _connect() override;
    void _replayError(const QString &errorMsg);
    quint64 _parseTimestamp(const QByteArray &bytes);
    quint64 _seekToNextMavlinkMessage(mavlink_message_t &nextMsg);
    quint64 _findLastTimestamp();
    quint64 _readNextMavlinkMessage(QByteArray &bytes);
    bool _loadLogFile();
    void _finishPlayback();
    void _resetPlaybackToBeginning();
    void _signalCurrentLogTimeSecs();

    LogReplayLinkConfiguration *_logReplayConfig = nullptr;
    QTimer *_readTickTimer = nullptr;
    bool _connected = false;
    uint8_t _mavlinkChannel = 0;

    quint64 _logCurrentTimeUSecs = 0;
    quint64 _logStartTimeUSecs = 0;
    quint64 _logEndTimeUSecs = 0;
    quint64 _logDurationUSecs = 0;

    qreal _playbackSpeed = 1;
    quint64 _playbackStartTimeMSecs = 0;
    quint64 _playbackStartLogTimeUSecs = 0;

    QFile _logFile;
    quint64 _logFileSize = 0;

    static const QString _errorTitle;

    static constexpr size_t kTimestamp = sizeof(quint64);
};

/*===========================================================================*/

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

    LogReplayLink *link() { return _link; }
    void setLink(LogReplayLink *link);
    bool isPlaying() const { return _isPlaying; }
    void setIsPlaying(bool isPlaying);
    qreal percentComplete() const { return _percentComplete; }
    void setPercentComplete(qreal percentComplete) { _link->movePlayhead(percentComplete); }

signals:
    void linkChanged(LogReplayLink *link);
    void isPlayingChanged(bool isPlaying);
    void percentCompleteChanged(qreal percentComplete);
    void playheadTimeChanged(const QString &playheadTime);
    void totalTimeChanged(const QString &totalTime);
    void playbackSpeedChanged(qreal playbackSpeed);

private slots:
    void _logFileStats(uint32_t logDurationSecs);
    void _playbackStarted();
    void _playbackPaused();
    void _playbackAtEnd();
    void _playbackPercentCompleteChanged(qreal percentComplete);
    void _currentLogTimeSecs(uint32_t secs);
    void _linkDisconnected() { setLink(nullptr); }

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
