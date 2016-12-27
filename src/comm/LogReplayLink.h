/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef LogReplayLink_H
#define LogReplayLink_H

#include "LinkInterface.h"
#include "LinkConfiguration.h"
#include "MAVLinkProtocol.h"

#include <QTimer>
#include <QFile>

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
    LinkType    type                    () { return LinkConfiguration::TypeLogReplay; }
    void        copyFrom                (LinkConfiguration* source);
    void        loadSettings            (QSettings& settings, const QString& root);
    void        saveSettings            (QSettings& settings, const QString& root);
    void        updateSettings          ();
    bool        isAutoConnectAllowed    () { return false; }
    QString     settingsURL             () { return "LogReplaySettings.qml"; }
signals:
    void fileNameChanged();

private:
    static const char*  _logFilenameKey;
    QString             _logFilename;
};

class LogReplayLink : public LinkInterface
{
    Q_OBJECT

    friend class LinkManager;

public:
    /// @return true: log is currently playing, false: log playback is paused
    bool isPlaying(void) { return _readTickTimer.isActive(); }

    /// Start replay at current position
    void play(void) { emit _playOnThread(); }

    /// Pause replay
    void pause(void) { emit _pauseOnThread(); }

    /// Move the playhead to the specified percent complete
    void movePlayhead(int percentComplete);

    /// Sets the acceleration factor: -100: 0.01X, 0: 1.0X, 100: 100.0X
    void setAccelerationFactor(int factor) { emit _setAccelerationFactorOnThread(factor); }

    // Virtuals from LinkInterface
    virtual QString getName(void) const { return _config->name(); }
    virtual void requestReset(void){ }
    virtual bool isConnected(void) const { return _connected; }
    virtual qint64 getConnectionSpeed(void) const { return 100000000; }
    virtual qint64 bytesAvailable(void) { return 0; }
    virtual bool isLogReplay(void) { return true; }

    // These are left unimplemented in order to cause linker errors which indicate incorrect usage of
    // connect/disconnect on link directly. All connect/disconnect calls should be made through LinkManager.
    bool connect(void);
    bool disconnect(void);

private slots:
    virtual void _writeBytes(const QByteArray bytes);

signals:
    void logFileStats(bool logTimestamped, int logDurationSecs, int binaryBaudRate);
    void playbackStarted(void);
    void playbackPaused(void);
    void playbackAtEnd(void);
    void playbackError(void);
    void playbackPercentCompleteChanged(int percentComplete);

    // Internal signals
    void _playOnThread(void);
    void _pauseOnThread(void);
    void _setAccelerationFactorOnThread(int factor);

private slots:
    void _readNextLogEntry(void);
    void _play(void);
    void _pause(void);
    void _setAccelerationFactor(int factor);

private:
    // Links are only created/destroyed by LinkManager so constructor/destructor is not public
    LogReplayLink(SharedLinkConfigurationPointer& config);
    ~LogReplayLink();

    void _replayError(const QString& errorMsg);
    quint64 _parseTimestamp(const QByteArray& bytes);
    quint64 _seekToNextMavlinkMessage(mavlink_message_t* nextMsg);
    bool _loadLogFile(void);
    void _finishPlayback(void);
    void _playbackError(void);
    void _resetPlaybackToBeginning(void);

    // Virtuals from LinkInterface
    virtual bool _connect(void);
    virtual void _disconnect(void);

    // Virtuals from QThread
    virtual void run(void);

    LogReplayLinkConfiguration* _logReplayConfig;

    bool    _connected;
    QTimer _readTickTimer;      ///< Timer which signals a read of next log record

    static const char* _errorTitle; ///< Title for communicatorError signals

    quint64 _logCurrentTimeUSecs;   ///< The timestamp of the next message in the log file.
    quint64 _logStartTimeUSecs;     ///< The first timestamp in the current log file.
    quint64 _logEndTimeUSecs;       ///< The last timestamp in the current log file.
    quint64 _logDurationUSecs;

    static const int    _defaultBinaryBaudRate = 57600;
    int                 _binaryBaudRate;        ///< Playback rate for binary log format

    float   _replayAccelerationFactor;  ///< Factor to apply to playback rate
    quint64 _playbackStartTimeMSecs;    ///< The time when the logfile was first played back. This is used to pace out replaying the messages to fix long-term drift/skew. 0 indicates that the player hasn't initiated playback of this log file.

    MAVLinkProtocol*    _mavlink;
    QFile               _logFile;
    quint64             _logFileSize;
    bool                _logTimestamped;    ///< true: Timestamped log format, false: no timestamps

    static const int cbTimestamp = sizeof(quint64);
};

#endif
