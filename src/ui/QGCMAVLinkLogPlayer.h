#pragma once

#include <QWidget>
#include <QFile>

#include "MAVLinkProtocol.h"
#include "LinkInterface.h"
#include "LogReplayLink.h"

namespace Ui
{
class QGCMAVLinkLogPlayer;
}

/**
 * @brief Replays MAVLink log files
 *
 * This class allows to replay MAVLink logs at varying speeds.
 * captured flights can be replayed, shown to others and analyzed
 * in-depth later on.
 */
class QGCMAVLinkLogPlayer : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMAVLinkLogPlayer(QWidget *parent = 0);
    ~QGCMAVLinkLogPlayer();

private slots:
    void _selectLogFileForPlayback(void);
    void _playPauseToggle(void);
    void _pause(void);
    void _setPlayheadFromSlider(int value);
#if 0
    void _setAccelerationFromSlider(int value);
#endif
    void _logFileStats(bool logTimestamped, int logDurationSeconds, int binaryBaudRate);
    void _playbackStarted(void);
    void _playbackPaused(void);
    void _playbackPercentCompleteChanged(int percentComplete);
    void _playbackError(void);
    void _replayLinkDisconnected(void);
    void _setCurrentLogTime(int secs);

private:
    void _finishPlayback(void);
    QString _secondsToHMS(int seconds);
    void _enablePlaybackControls(bool enabled);

    LogReplayLink*  _replayLink;
    int             _logDurationSeconds;
    int             _lastCurrentTime;
    
    Ui::QGCMAVLinkLogPlayer* _ui;
};

