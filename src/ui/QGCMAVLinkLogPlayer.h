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
    /** @brief Play / pause the log */
    void playPause(bool play);
    /** @brief Replay the logfile */
    void play();
    /** @brief Pause the logfile */
    void pause();
    /** @brief Reset the logfile */
    bool reset(int packetIndex=0);
    /** @brief Select logfile */
    bool selectLogFile();
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

protected:
    int lineCounter;
    int totalLines;
    quint64 startTime;
    quint64 endTime;
    quint64 currentStartTime;
    float accelerationFactor;
    MAVLinkProtocol* mavlink;
    MAVLinkSimulationLink* logLink;
    QFile logFile;
    QTimer loopTimer;
    int loopCounter;
    bool mavlinkLogFormat;
    int binaryBaudRate;
    bool isPlaying;
    unsigned int currPacketCount;
    static const int packetLen = MAVLINK_MAX_PACKET_LEN;
    static const int timeLen = sizeof(quint64);
    void changeEvent(QEvent *e);

private:
    Ui::QGCMAVLinkLogPlayer *ui;
};

#endif // QGCMAVLINKLOGPLAYER_H
