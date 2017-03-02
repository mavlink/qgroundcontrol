#include <QStandardPaths>
#include <QtEndian>

#include "MainWindow.h"
#ifndef NO_SERIAL_LINK
#include "SerialLink.h"
#endif
#include "QGCMAVLinkLogPlayer.h"
#include "QGC.h"
#include "ui_QGCMAVLinkLogPlayer.h"
#include "QGCApplication.h"
#include "LinkManager.h"
#include "QGCFileDialog.h"
#include "QGCMessageBox.h"

QGCMAVLinkLogPlayer::QGCMAVLinkLogPlayer(QWidget *parent) :
    QWidget(parent),
    _replayLink(NULL),
    _ui(new Ui::QGCMAVLinkLogPlayer)
{
    _ui->setupUi(this);
    _ui->horizontalLayout->setAlignment(Qt::AlignTop);

    // Setup buttons
    connect(_ui->selectFileButton, &QPushButton::clicked, this, &QGCMAVLinkLogPlayer::_selectLogFileForPlayback);
    connect(_ui->playButton, &QPushButton::clicked, this, &QGCMAVLinkLogPlayer::_playPauseToggle);
    connect(_ui->positionSlider, &QSlider::valueChanged, this, &QGCMAVLinkLogPlayer::_setPlayheadFromSlider);
    connect(_ui->positionSlider, &QSlider::sliderPressed, this, &QGCMAVLinkLogPlayer::_pause);

#if 0
    // Speed slider is removed from 3.0 release. Too broken to fix.
    connect(_ui->speedSlider, &QSlider::valueChanged, this, &QGCMAVLinkLogPlayer::_setAccelerationFromSlider);
    _ui->speedSlider->setMinimum(-100);
    _ui->speedSlider->setMaximum(100);
    _ui->speedSlider->setValue(0);
#endif

    _enablePlaybackControls(false);

    _ui->positionSlider->setMinimum(0);
    _ui->positionSlider->setMaximum(100);

}

QGCMAVLinkLogPlayer::~QGCMAVLinkLogPlayer()
{
    delete _ui;
}

void QGCMAVLinkLogPlayer::_playPauseToggle(void)
{
    if (_replayLink->isPlaying()) {
        _pause();
    } else {
        _replayLink->play();
    }
}

void QGCMAVLinkLogPlayer::_pause(void)
{
    _replayLink->pause();
}

void QGCMAVLinkLogPlayer::_selectLogFileForPlayback(void)
{
    // Disallow replay when any links are connected
    if (qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()) {
        QGCMessageBox::information(tr("Log Replay"), tr("You must close all connections prior to replaying a log."));
        return;
    }

    QString logFilename = QGCFileDialog::getOpenFileName(
        this,
        tr("Load MAVLink Log File"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        tr("MAVLink Log Files (*.mavlink);;All Files (*)"));

    if (logFilename.isEmpty()) {
        return;
    }

    LinkInterface* createConnectedLink(LinkConfiguration* config);

    LogReplayLinkConfiguration* linkConfig = new LogReplayLinkConfiguration(QString("Log Replay"));
    linkConfig->setLogFilename(logFilename);
    linkConfig->setName(linkConfig->logFilenameShort());
    _ui->logFileNameLabel->setText(linkConfig->logFilenameShort());

    LinkManager* linkMgr = qgcApp()->toolbox()->linkManager();
    SharedLinkConfigurationPointer sharedConfig = linkMgr->addConfiguration(linkConfig);
    _replayLink = (LogReplayLink*)qgcApp()->toolbox()->linkManager()->createConnectedLink(sharedConfig);

    connect(_replayLink, &LogReplayLink::logFileStats, this, &QGCMAVLinkLogPlayer::_logFileStats);
    connect(_replayLink, &LogReplayLink::playbackStarted, this, &QGCMAVLinkLogPlayer::_playbackStarted);
    connect(_replayLink, &LogReplayLink::playbackPaused, this, &QGCMAVLinkLogPlayer::_playbackPaused);
    connect(_replayLink, &LogReplayLink::playbackPercentCompleteChanged, this, &QGCMAVLinkLogPlayer::_playbackPercentCompleteChanged);
    connect(_replayLink, &LogReplayLink::disconnected, this, &QGCMAVLinkLogPlayer::_replayLinkDisconnected);

    _ui->positionSlider->setValue(0);
#if 0
    _ui->speedSlider->setValue(0);
#endif
}

void QGCMAVLinkLogPlayer::_playbackError(void)
{
    _ui->logFileNameLabel->setText("Error");
    _enablePlaybackControls(false);
}

QString QGCMAVLinkLogPlayer::_secondsToHMS(int seconds)
{
    int secondsPart  = seconds;
    int minutesPart  = secondsPart / 60;
    int hoursPart    = minutesPart / 60;
    secondsPart -= 60 * minutesPart;
    minutesPart -= 60 * hoursPart;

    return QString("%1h:%2m:%3s").arg(hoursPart, 2).arg(minutesPart, 2).arg(secondsPart, 2);
}

/// Signalled from LogReplayLink once log file information is known
void QGCMAVLinkLogPlayer::_logFileStats(bool    logTimestamped,         ///< true: timestamped log
                                        int     logDurationSeconds,     ///< Log duration
                                        int     binaryBaudRate)         ///< Baud rate for non-timestamped log
{
    Q_UNUSED(logTimestamped);
    Q_UNUSED(binaryBaudRate);

    _logDurationSeconds = logDurationSeconds;

    _ui->logStatsLabel->setText(_secondsToHMS(logDurationSeconds));
}

/// Signalled from LogReplayLink when replay starts
void QGCMAVLinkLogPlayer::_playbackStarted(void)
{
    _enablePlaybackControls(true);
    _ui->playButton->setChecked(true);
    _ui->playButton->setIcon(QIcon(":/res/Pause"));
}

/// Signalled from LogReplayLink when replay is paused
void QGCMAVLinkLogPlayer::_playbackPaused(void)
{
    _ui->playButton->setIcon(QIcon(":/res/Play"));
    _ui->playButton->setChecked(false);
}

void QGCMAVLinkLogPlayer::_playbackPercentCompleteChanged(int percentComplete)
{
    _ui->positionSlider->blockSignals(true);
    _ui->positionSlider->setValue(percentComplete);
    _ui->positionSlider->blockSignals(false);
}

void QGCMAVLinkLogPlayer::_setPlayheadFromSlider(int value)
{
    if (_replayLink) {
        _replayLink->movePlayhead(value);
    }
}

void QGCMAVLinkLogPlayer::_enablePlaybackControls(bool enabled)
{
    _ui->playButton->setEnabled(enabled);
#if 0
    _ui->speedSlider->setEnabled(enabled);
#endif
    _ui->positionSlider->setEnabled(enabled);
}

#if 0
void QGCMAVLinkLogPlayer::_setAccelerationFromSlider(int value)
{
    //qDebug() << value;
    if (_replayLink) {
        _replayLink->setAccelerationFactor(value);
    }

    // Factor: -100: 0.01x, 0: 1.0x, 100: 100x

    float accelerationFactor;
    if (value < 0) {
        accelerationFactor = 0.01f;
        value -= -100;
        if (value > 0) {
            accelerationFactor *= (float)value;
        }
    } else if (value > 0) {
        accelerationFactor = 1.0f * (float)value;
    } else {
        accelerationFactor = 1.0f;
    }

    _ui->speedLabel->setText(QString("Speed: %1X").arg(accelerationFactor, 5, 'f', 2, '0'));
}
#endif

void QGCMAVLinkLogPlayer::_replayLinkDisconnected(void)
{
    _enablePlaybackControls(false);
    _replayLink = NULL;
}
