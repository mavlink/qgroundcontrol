#ifndef QGCMAVLINKLOGPLAYER_H
#define QGCMAVLINKLOGPLAYER_H

#include <QWidget>
#include <QFile>

#include "MAVLinkProtocol.h"
#include "LinkInterface.h"
#include "MAVLinkSimulationLink.h"

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
    explicit QGCMAVLinkLogPlayer(MAVLinkProtocol* mavlink, QWidget *parent = 0);
    ~QGCMAVLinkLogPlayer();
    bool isPlayingLogFile()
    {
        return isPlaying;
    }

    bool isLogFileSelected()
    {
        return logFile.isOpen();
    }

public slots:
    /** @brief Toggle between play and pause */
    void playPauseToggle();
    /** @brief Replay the logfile */
    void play();
    /** @brief Pause the log player. */
    void pause();
    /** @brief Reset the internal log player state, including the UI */
    void reset();
    /** @brief Load log file */
    bool loadLogFile(const QString& file);
    /** @brief Jump to a position in the logfile */
    void jumpToSliderVal(int slidervalue);
    /** @brief The logging mainloop */
    void logLoop();
    /** @brief Set acceleration factor in percent */
    void setAccelerationFactorInt(int factor);

signals:
    /** @brief Send ready bytes */
    void bytesReady(LinkInterface* link, const QByteArray& bytes);
    void logFileEndReached();

protected:
    quint64 playbackStartTime;     ///< The time when the logfile was first played back. This is used to pace out replaying the messages to fix long-term drift/skew. 0 indicates that the player hasn't initiated playback of this log file. In units of milliseconds since epoch UTC.
    quint64 logCurrentTime;        ///< The timestamp of the next message in the log file. In units of microseconds since epoch UTC.
    quint64 logStartTime;          ///< The first timestamp in the current log file. In units of microseconds since epoch UTC.
    quint64 logEndTime;            ///< The last timestamp in the current log file. In units of microseconds since epoch UTC.
    float accelerationFactor;
    MAVLinkProtocol* mavlink;
    MAVLinkSimulationLink* logLink;
    QFile logFile;
    QTimer loopTimer;
    int loopCounter;
    bool mavlinkLogFormat; ///< If the logfile is stored in the timestamped MAVLink log format
    int binaryBaudRate;
    static const int defaultBinaryBaudRate = 57600;
    bool isPlaying;
    unsigned int currPacketCount;
    static const int packetLen = MAVLINK_MAX_PACKET_LEN;
    static const int timeLen = sizeof(quint64);
    void changeEvent(QEvent *e);
    
private slots:
    void _selectLogFileForPlayback(void);

private:
    Ui::QGCMAVLinkLogPlayer *ui;
	virtual void paintEvent(QPaintEvent *);

    /** @brief Parse out a quint64 timestamp in microseconds in the proper endianness. */
    quint64 parseTimestamp(const QByteArray &data);

    /**
     * This function parses out the next MAVLink message and its corresponding timestamp.
     *
     * It makes no assumptions about where in the file we currently are. It leaves the file right
     * at the beginning of the successfully parsed message. Note that this function will not attempt to
     * correct for any MAVLink parsing failures, so it always returns the next successfully-parsed
     * message.
     *
     * @param msg[output] Where the final parsed message output will go.
     * @return A Unix timestamp in microseconds UTC or 0 if parsing failed
     */
    quint64 findNextMavlinkMessage(mavlink_message_t *msg);

    /**
     * Updates the QSlider UI to be at the given percentage.
     * @param percent A percentage value between 0.0% and 100.0%.
     */
    void updatePositionSliderUi(float percent);

    /**
     * Jumps to a new position in the current playback file as a percentage.
     * @param percentage The position of the file to jump to as a percentage.
     * @return True if the new file position was successfully jumped to, false otherwise
     */
    bool jumpToPlaybackLocation(float percentage);
    
    void _finishPlayback(void);
    void _playbackError(void);
};

#endif // QGCMAVLINKLOGPLAYER_H
