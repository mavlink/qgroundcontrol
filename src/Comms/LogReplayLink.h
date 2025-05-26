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
#include <QtCore/QLoggingCategory>

#include "LinkConfiguration.h"
#include "LinkInterface.h"

class QTimer;

typedef struct __mavlink_message mavlink_message_t;

Q_DECLARE_LOGGING_CATEGORY(LogReplayLinkLog)

/*===========================================================================*/

class LogReplayConfiguration : public LinkConfiguration
{
    Q_OBJECT

    Q_PROPERTY(QString filename READ logFilename WRITE setLogFilename NOTIFY filenameChanged)

public:
    explicit LogReplayConfiguration(const QString &name, QObject *parent = nullptr);
    explicit LogReplayConfiguration(const LogReplayConfiguration *copy, QObject *parent = nullptr);
    virtual ~LogReplayConfiguration();

    LinkType type() const override { return LinkConfiguration::TypeLogReplay; }
    void copyFrom(const LinkConfiguration *source) override;
    void loadSettings(QSettings &settings, const QString &root) override;
    void saveSettings(QSettings &settings, const QString &root) const override;
    QString settingsURL() const override { return QStringLiteral("LogReplaySettings.qml"); }
    QString settingsTitle() const override { return tr("Log Replay Link Settings"); }

    QString logFilenameShort() const;
    QString logFilename() const { return _logFilename; }
    void setLogFilename(const QString &logFilename);

signals:
    void filenameChanged();

private:
    QString _logFilename;
};

/*===========================================================================*/

class LogReplayWorker : public QObject
{
    Q_OBJECT

public:
    explicit LogReplayWorker(const LogReplayConfiguration *config, QObject *parent = nullptr);
    ~LogReplayWorker();

    bool isConnected() const { return _isConnected; }
    bool isPlaying() const;

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &errorString);
    void dataReceived(const QByteArray &data);
    void logFileStats(uint32_t logDurationSecs);
    void playbackStarted();
    void playbackPaused();
    void playbackAtEnd();
    void playbackPercentCompleteChanged(qreal percentComplete);
    void currentLogTimeSecs(uint32_t secs);

public slots:
    void setup();
    void connectToLog();
    void disconnectFromLog();
    void play();
    void pause();
    void setPlaybackSpeed(qreal playbackSpeed);
    void movePlayhead(qreal percentComplete);

private slots:
    void _readNextLogEntry();

private:
    quint64 _parseTimestamp(const QByteArray &bytes);
    quint64 _seekToNextMavlinkMessage(mavlink_message_t &nextMsg);
    quint64 _findLastTimestamp();
    quint64 _readNextMavlinkMessage(QByteArray &bytes);
    bool _loadLogFile();
    void _resetPlaybackToBeginning();
    void _signalCurrentLogTimeSecs();

    const LogReplayConfiguration *_logReplayConfig = nullptr;
    QTimer *_readTickTimer = nullptr;

    bool _isConnected = false;
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

    static constexpr size_t kTimestamp = sizeof(quint64);
};

/*===========================================================================*/

class LogReplayLink : public LinkInterface
{
    Q_OBJECT

public:
    explicit LogReplayLink(SharedLinkConfigurationPtr &config, QObject *parent = nullptr);
    virtual ~LogReplayLink();

    bool isConnected() const override { return _worker->isConnected(); }
    void disconnect() override;
    bool isLogReplay() const final { return true; }

    bool isPlaying() const { return _worker->isPlaying(); }
    void play();
    void pause();
    void setPlaybackSpeed(qreal playbackSpeed);
    void movePlayhead(qreal percentComplete);

signals:
    void logFileStats(uint32_t logDurationSecs);
    void playbackStarted();
    void playbackPaused();
    void playbackAtEnd();
    void playbackPercentCompleteChanged(qreal percentComplete);
    void currentLogTimeSecs(uint32_t secs);

private slots:
    void _writeBytes(const QByteArray &bytes) override { Q_UNUSED(bytes); }
    void _onConnected() { emit connected(); }
    void _onDisconnected() { emit disconnected(); }
    void _onErrorOccurred(const QString &errorString);
    void _onDataReceived(const QByteArray &data);

private:
    bool _connect() override;

    const LogReplayConfiguration *_logReplayConfig = nullptr;
    LogReplayWorker *_worker = nullptr;
    QThread *_workerThread = nullptr;
};
