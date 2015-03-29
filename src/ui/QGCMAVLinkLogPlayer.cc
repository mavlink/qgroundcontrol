#include <QStandardPaths>
#include <QtEndian>

#include "MainWindow.h"
#include "SerialLink.h"
#include "QGCMAVLinkLogPlayer.h"
#include "QGC.h"
#include "ui_QGCMAVLinkLogPlayer.h"
#include "QGCApplication.h"
#include "LinkManager.h"
#include "QGCFileDialog.h"
#include "QGCMessageBox.h"

QGCMAVLinkLogPlayer::QGCMAVLinkLogPlayer(MAVLinkProtocol* mavlink, QWidget *parent) :
    QWidget(parent),
    playbackStartTime(0),
    logStartTime(0),
    logEndTime(0),
    accelerationFactor(1.0f),
    mavlink(mavlink),
    logLink(NULL),
    loopCounter(0),
    mavlinkLogFormat(true),
    binaryBaudRate(defaultBinaryBaudRate),
    isPlaying(false),
    currPacketCount(0),
    ui(new Ui::QGCMAVLinkLogPlayer)
{
    Q_ASSERT(mavlink);
    
    ui->setupUi(this);
    ui->horizontalLayout->setAlignment(Qt::AlignTop);

    // Connect protocol
    connect(this, SIGNAL(bytesReady(LinkInterface*,QByteArray)), mavlink, SLOT(receiveBytes(LinkInterface*,QByteArray)));

    // Setup timer
    connect(&loopTimer, &QTimer::timeout, this, &QGCMAVLinkLogPlayer::logLoop);

    // Setup buttons
    connect(ui->selectFileButton, &QPushButton::clicked, this, &QGCMAVLinkLogPlayer::_selectLogFileForPlayback);
    connect(ui->playButton, &QPushButton::clicked, this, &QGCMAVLinkLogPlayer::playPauseToggle);
    connect(ui->speedSlider, &QSlider::valueChanged, this, &QGCMAVLinkLogPlayer::setAccelerationFactorInt);
    connect(ui->positionSlider, &QSlider::valueChanged, this, &QGCMAVLinkLogPlayer::jumpToSliderVal);
    connect(ui->positionSlider, &QSlider::sliderPressed, this, &QGCMAVLinkLogPlayer::pause);
    
    setAccelerationFactorInt(49);
    ui->speedSlider->setValue(49);
    updatePositionSliderUi(0.0);

    ui->playButton->setEnabled(false);
    ui->speedSlider->setEnabled(false);
    ui->positionSlider->setEnabled(false);
    ui->speedLabel->setEnabled(false);
    ui->logFileNameLabel->setEnabled(false);
    ui->logStatsLabel->setEnabled(false);

    // Monitor for when the end of the log file is reached. This is done using signals because the main work is in a timer.
    connect(this,  &QGCMAVLinkLogPlayer::logFileEndReached, &loopTimer, &QTimer::stop);
}

QGCMAVLinkLogPlayer::~QGCMAVLinkLogPlayer()
{
    delete ui;
}

void QGCMAVLinkLogPlayer::playPauseToggle()
{
    if (isPlaying)
    {
        pause();
    }
    else
    {
        play();
    }
}

void QGCMAVLinkLogPlayer::play()
{
    Q_ASSERT(logFile.isOpen());
    
    LinkManager::instance()->setConnectionsSuspended(tr("Connect not allowed during Flight Data replay."));
    mavlink->suspendLogForReplay(true);
    
    // Disable the log file selector button
    ui->selectFileButton->setEnabled(false);

    // Make sure we aren't at the end of the file, if we are, reset to the beginning and play from there.
    if (logFile.atEnd())
    {
        reset();
    }

    // Always correct the current start time such that the next message will play immediately at playback.
    // We do this by subtracting the current file playback offset  from now()
    playbackStartTime = (quint64)QDateTime::currentMSecsSinceEpoch() - (logCurrentTime - logStartTime) / 1000;

    // Start timer
    if (mavlinkLogFormat)
    {
        loopTimer.start(1);
    }
    else
    {
        // Read len bytes at a time
        int len = 100;
        // Calculate the number of times to read 100 bytes per second
        // to guarantee the baud rate, then divide 1000 by the number of read
        // operations to obtain the interval in milliseconds
        int interval = 1000 / ((binaryBaudRate / 10) / len);
        loopTimer.start(interval / accelerationFactor);
    }
    isPlaying = true;
    ui->playButton->setChecked(true);
    ui->playButton->setIcon(QIcon(":files/images/actions/media-playback-pause.svg"));
}

void QGCMAVLinkLogPlayer::pause()
{
    LinkManager::instance()->setConnectionsAllowed();
    mavlink->suspendLogForReplay(false);

    loopTimer.stop();
    isPlaying = false;
    ui->playButton->setIcon(QIcon(":files/images/actions/media-playback-start.svg"));
    ui->playButton->setChecked(false);
    ui->selectFileButton->setEnabled(true);
}

void QGCMAVLinkLogPlayer::reset()
{
    pause();
    loopCounter = 0;
    if (logFile.isOpen()) {
        logFile.reset();
    }

    // Now update the position slider to its default location
    updatePositionSliderUi(0.0);

    // And since we haven't starting playback, clear the time of initial playback and the current timestamp.
    playbackStartTime = 0;
    logCurrentTime = logStartTime;
}

bool QGCMAVLinkLogPlayer::jumpToPlaybackLocation(float percentage)
{
    // Reset only for valid values
    if (percentage <= 100.0f && percentage >= 0.0f)
    {
        bool result = true;
        if (mavlinkLogFormat)
        {
            // But if we have a timestamped MAVLink log, then actually aim to hit that percentage in terms of
            // time through the file.
            qint64 newFilePos = (qint64)(percentage * (float)logFile.size());

            // Now seek to the appropriate position, failing gracefully if we can't.
            if (!logFile.seek(newFilePos))
            {
                // Fallback: Start from scratch
                logFile.reset();
                ui->logStatsLabel->setText(tr("Changing packet index failed, back to start."));
                result = false;
            }

            // But we do align to the next MAVLink message for consistency.
            mavlink_message_t dummy;
            logCurrentTime = findNextMavlinkMessage(&dummy);

            // Now calculate the current file location based on time.
            float newRelativeTime = (float)(logCurrentTime - logStartTime);

            // Calculate the effective baud rate of the file in bytes/s.
            float logDuration = (logEndTime - logStartTime);
            float baudRate = logFile.size() / logDuration / 1e6;

            // And the desired time is:
            float desiredTime = percentage * logDuration;

            // And now jump the necessary number of bytes in the proper direction
            qint64 offset = (newRelativeTime - desiredTime) * baudRate;
            logFile.seek(logFile.pos() + offset);

            // And scan until we reach the start of a MAVLink message. We make sure to record this timestamp for
            // smooth jumping around the file.
            logCurrentTime = findNextMavlinkMessage(&dummy);

            // Now update the UI with our actual final position.
            newRelativeTime = (float)(logCurrentTime - logStartTime);
            percentage = newRelativeTime / logDuration;
            updatePositionSliderUi(percentage);
        }
        else
        {
            // If we're working with a non-timestamped file, we just jump to that percentage of the file,
            // align to the next MAVLink message and roll with it. No reason to do anything more complicated.
            qint64 newFilePos = (qint64)(percentage * (float)logFile.size());

            // Now seek to the appropriate position, failing gracefully if we can't.
            if (!logFile.seek(newFilePos))
            {
                // Fallback: Start from scratch
                logFile.reset();
                ui->logStatsLabel->setText(tr("Changing packet index failed, back to start."));
                result = false;
            }

            // But we do align to the next MAVLink message for consistency.
            mavlink_message_t dummy;
            findNextMavlinkMessage(&dummy);
        }

        // Now update the UI. This is necessary because stop() is called when loading a new logfile

        return result;
    }
    else
    {
        return false;
    }
}

void QGCMAVLinkLogPlayer::updatePositionSliderUi(float percent)
{
    ui->positionSlider->blockSignals(true);
    int sliderVal = ui->positionSlider->minimum() + (int)(percent * (float)(ui->positionSlider->maximum() - ui->positionSlider->minimum()));
    ui->positionSlider->setValue(sliderVal);

    // Calculate the runtime in hours:minutes:seconds
    // WARNING: Order matters in this computation
    quint32 seconds = percent * (logEndTime - logStartTime) / 1e6;
    quint32 minutes = seconds / 60;
    quint32 hours = minutes / 60;
    seconds -= 60*minutes;
    minutes -= 60*hours;

    // And show the user the details we found about this file.
    QString timeLabel = tr("%1h:%2m:%3s").arg(hours, 2).arg(minutes, 2).arg(seconds, 2);
    ui->positionSlider->setToolTip(timeLabel);
    ui->positionSlider->blockSignals(false);
}

/// @brief Displays a file dialog to allow the user to select a log file to play back.
void QGCMAVLinkLogPlayer::_selectLogFileForPlayback(void)
{
    // Disallow replay when any links are connected
    if (LinkManager::instance()->anyConnectedLinks()) {
        QGCMessageBox::information(tr("Log Replay"), tr("You must close all connections prior to replaying a log."));
        return;
    }
    
    QString logFile = QGCFileDialog::getOpenFileName(
        this,
        tr("Load MAVLink Log File"),
        qgcApp()->mavlinkLogFilesLocation(),
        tr("MAVLink Log Files (*.mavlink);;All Files (*)"));

    if (!logFile.isEmpty()) {
        loadLogFile(logFile);
    }
}

/**
 * @param factor 0: 0.01X, 50: 1.0X, 100: 100.0X
 */
void QGCMAVLinkLogPlayer::setAccelerationFactorInt(int factor)
{
    float f = factor+1.0f;
    f -= 50.0f;

    if (f < 0.0f)
    {
        accelerationFactor = 1.0f / (-f/2.0f);
    }
    else
    {
        accelerationFactor = 1+(f/2.0f);
    }

    // Update timer interval
    if (!mavlinkLogFormat)
    {
        // Read len bytes at a time
        int len = 100;
        // Calculate the number of times to read 100 bytes per second
        // to guarantee the baud rate, then divide 1000 by the number of read
        // operations to obtain the interval in milliseconds
        int interval = 1000 / ((binaryBaudRate / 10) / len);
        loopTimer.stop();
        loopTimer.start(interval / accelerationFactor);
    }

    ui->speedLabel->setText(tr("Speed: %1X").arg(accelerationFactor, 5, 'f', 2, '0'));
}

bool QGCMAVLinkLogPlayer::loadLogFile(const QString& file)
{
    // Make sure to stop the logging process and reset everything.
    reset();
    logFile.close();

    // Now load the new file.
    logFile.setFileName(file);
    if (!logFile.open(QFile::ReadOnly)) {
        QGCMessageBox::critical(tr("Log Replay"), tr("The selected file is unreadable. Please make sure that the file %1 is readable or select a different file").arg(file));
        _playbackError();
        return false;
    }
    
    QFileInfo logFileInfo(file);
    ui->logFileNameLabel->setText(tr("File: %1").arg(logFileInfo.fileName()));

    // If there's an existing MAVLinkSimulationLink() being used for an old file,
    // we replace it.
    if (logLink) {
        LinkManager::instance()->_deleteLink(logLink);
    }
    logLink = new MAVLinkSimulationLink("");

    // Select if binary or MAVLink log format is used
    mavlinkLogFormat = file.endsWith(".mavlink");

    if (mavlinkLogFormat)
    {
        // Get the first timestamp from the logfile
        // This should be a big-endian uint64.
        QByteArray timestamp = logFile.read(timeLen);
        quint64 starttime = parseTimestamp(timestamp);

        // Now find the last timestamp by scanning for the last MAVLink packet and
        // find the timestamp before it. To do this we start searchin a little before
        // the end of the file, specifically the maximum MAVLink packet size + the
        // timestamp size. This guarantees that we will hit a MAVLink packet before
        // the end of the file. Unfortunately, it basically guarantees that we will
        // hit more than one. This is why we have to search for a bit.
        qint64 fileLoc = logFile.size() - MAVLINK_MAX_PACKET_LEN - timeLen;
        logFile.seek(fileLoc);
        quint64 endtime = starttime; // Set a sane default for the endtime
        mavlink_message_t msg;
        quint64 newTimestamp;
        while ((newTimestamp = findNextMavlinkMessage(&msg)) > endtime) {
            endtime = newTimestamp;
        }

        if (endtime == starttime) {
            QGCMessageBox::critical(tr("Log Replay"), tr("The selected file is corrupt. No valid timestamps were found at the end of the file.").arg(file));
            _playbackError();
            return false;
        }

        // Remember the start and end time so we can move around this logfile with the slider.
        logEndTime = endtime;
        logStartTime = starttime;
        logCurrentTime = logStartTime;

        // Reset our log file so when we go to read it for the first time, we start at the beginning.
        logFile.reset();

        // Calculate the runtime in hours:minutes:seconds
        // WARNING: Order matters in this computation
        quint32 seconds = (endtime - starttime)/1000000;
        quint32 minutes = seconds / 60;
        quint32 hours = minutes / 60;
        seconds -= 60*minutes;
        minutes -= 60*hours;

        // And show the user the details we found about this file.
        QString timelabel = tr("%1h:%2m:%3s").arg(hours, 2).arg(minutes, 2).arg(seconds, 2);
        currPacketCount = logFileInfo.size()/(32 + MAVLINK_NUM_NON_PAYLOAD_BYTES + sizeof(quint64)); // Count packets by assuming an average payload size of 32 bytes
        ui->logStatsLabel->setText(tr("%2 MB, ~%3 packets, %4").arg(logFileInfo.size()/1000000.0f, 0, 'f', 2).arg(currPacketCount).arg(timelabel));
    }
    else
    {
        // Load in binary mode. In this mode, files should be have a filename postfix
        // of the baud rate they were recorded at, like `test_run_115200.bin`. Then on
        // playback, the datarate is equal to set to this value.

        // Set baud rate if any present. Otherwise we default to 57600.
        QStringList parts = logFileInfo.baseName().split("_");
        binaryBaudRate = defaultBinaryBaudRate;
        if (parts.count() > 1)
        {
            bool ok;
            int rate = parts.last().toInt(&ok);
            // 9600 baud to 100 MBit
            if (ok && (rate > 9600 && rate < 100000000))
            {
                // Accept this as valid baudrate
                binaryBaudRate = rate;
            }
        }

        int seconds = logFileInfo.size() / (binaryBaudRate / 10);
        int minutes = seconds / 60;
        int hours = minutes / 60;
        seconds -= 60*minutes;
        minutes -= 60*hours;

        QString timelabel = tr("%1h:%2m:%3s").arg(hours, 2).arg(minutes, 2).arg(seconds, 2);
        ui->logStatsLabel->setText(tr("%2 MB, %4 at %5 KB/s").arg(logFileInfo.size()/1000000.0f, 0, 'f', 2).arg(timelabel).arg(binaryBaudRate/10.0f/1024.0f, 0, 'f', 2));
    }

    // Enable controls
    ui->playButton->setEnabled(true);
    ui->speedSlider->setEnabled(true);
    ui->positionSlider->setEnabled(true);
    ui->speedLabel->setEnabled(true);
    ui->logFileNameLabel->setEnabled(true);
    ui->logStatsLabel->setEnabled(true);
    
    play();

    return true;
}

quint64 QGCMAVLinkLogPlayer::parseTimestamp(const QByteArray &data)
{
    // Retrieve the timestamp from the ByteArray assuming a proper BigEndian quint64 timestamp in microseconds.
    quint64 timestamp = qFromBigEndian(*((quint64*)(data.constData())));

    // And get the current time in microseconds
    quint64 currentTimestamp = ((quint64)QDateTime::currentMSecsSinceEpoch()) * 1000;

    // Now if the parsed timestamp is in the future, it must be an old file where the timestamp was stored as
    // little endian, so switch it.
    if (timestamp > currentTimestamp) {
        timestamp = qbswap(timestamp);
    }

    return timestamp;
}

/**
 * Jumps to the current percentage of the position slider. When this is called, the LogPlayer should already
 * have been paused, so it just jumps to the proper location in the file and resumes playing.
 */
void QGCMAVLinkLogPlayer::jumpToSliderVal(int slidervalue)
{
    // Determine what percentage through the file we should be (time or packet number depending).
    float newLocation = slidervalue / (float)(ui->positionSlider->maximum() - ui->positionSlider->minimum());

    // And clamp our calculated values to the valid range of [0,100]
    if (newLocation > 100.0f)
    {
        newLocation = 100.0f;
    }
    if (newLocation < 0.0f)
    {
        newLocation = 0.0f;
    }

    // Do only valid jumps
    if (jumpToPlaybackLocation(newLocation))
    {
        if (mavlinkLogFormat)
        {
            // Grab the total seconds of this file (1e6 is due to microsecond -> second conversion)
            int seconds = newLocation * (logEndTime - logStartTime) / 1e6;
            int minutes = seconds / 60;
            int hours = minutes / 60;
            seconds -= 60*minutes;
            minutes -= 60*hours;

            ui->logStatsLabel->setText(tr("Jumped to time %1h:%2m:%3s").arg(hours, 2).arg(minutes, 2).arg(seconds, 2));
        }
        else
        {
            ui->logStatsLabel->setText(tr("Jumped to %1").arg(newLocation));
        }

        play();
    }
    else
    {
        reset();
    }
}

/**
 * This function is the "mainloop" of the log player, reading one line
 * and adjusting the mainloop timer to read the next line in time.
 * It might not perfectly match the timing of the log file,
 * but it will never induce a static drift into the log file replay.
 * For scientific logging, the use of onboard timestamps and the log
 * functionality of the line chart plot is recommended.
 */
void QGCMAVLinkLogPlayer::logLoop()
{
    // If we have a file with timestamps, try and pace this out following the time differences
    // between the timestamps and the current playback speed.
    if (mavlinkLogFormat)
    {
        // Now parse MAVLink messages, grabbing their timestamps as we go. We stop once we
        // have at least 3ms until the next one. 

        // We track what the next execution time should be in milliseconds, which we use to set
        // the next timer interrupt.
        int nextExecutionTime = 0;

        // We use the `findNextMavlinkMessage()` function to scan ahead for MAVLink messages. This
        // is necessary because we don't know how big each MAVLink message is until we finish parsing
        // one, and since we only output arrays of bytes, we need to know the size of that array.
        mavlink_message_t msg;
        findNextMavlinkMessage(&msg);

        while (nextExecutionTime < 3) {

            // Now we're sitting at the start of a MAVLink message, so read it all into a byte array for feeding to our parser.
            QByteArray message = logFile.read(msg.len + MAVLINK_NUM_NON_PAYLOAD_BYTES);

            // Emit this message to our MAVLink parser.
            emit bytesReady(logLink, message);

            // If we've reached the end of the of the file, make sure we handle that well
            if (logFile.atEnd()) {
                _finishPlayback();
                return;
            }

            // Run our parser to find the next timestamp and leave us at the start of the next MAVLink message.
            logCurrentTime = findNextMavlinkMessage(&msg);

            // Calculate how long we should wait in real time until parsing this message.
            // We pace ourselves relative to the start time of playback to fix any drift (initially set in play())
            qint64 timediff = (logCurrentTime - logStartTime) / accelerationFactor;
            quint64 desiredPacedTime = playbackStartTime + ((quint64)timediff) / 1000;
            quint64 currentTime = (quint64)QDateTime::currentMSecsSinceEpoch();
            nextExecutionTime = desiredPacedTime - currentTime;
        }

        // And schedule the next execution of this function.
        loopTimer.start(nextExecutionTime);
    }
    else
    {
        // Binary format - read at fixed rate
        const int len = 100;
        QByteArray chunk = logFile.read(len);

        // Emit this packet
        emit bytesReady(logLink, chunk);

        // Check if reached end of file before reading next timestamp
        if (chunk.length() < len || logFile.atEnd())
        {
            _finishPlayback();
            return;
        }
    }

    // Update the UI every 2^5=32 times, or when there isn't much data to be played back.
    // Reduces flickering and minimizes CPU load.
    if ((loopCounter & 0x1F) == 0 || currPacketCount < 2000)
    {
        QFileInfo logFileInfo(logFile);
        updatePositionSliderUi(logFile.pos() / static_cast<float>(logFileInfo.size()));
    }
    loopCounter++;
}

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
quint64 QGCMAVLinkLogPlayer::findNextMavlinkMessage(mavlink_message_t *msg)
{
    char nextByte;
    mavlink_status_t comm;
    while (logFile.getChar(&nextByte)) { // Loop over every byte
        bool messageFound = mavlink_parse_char(logLink->getMavlinkChannel(), nextByte, msg, &comm);

        // If we've found a message, jump back to the start of the message, grab the timestamp,
        // and go back to the end of this file.
        if (messageFound) {
            logFile.seek(logFile.pos() - (msg->len + MAVLINK_NUM_NON_PAYLOAD_BYTES + timeLen));
            QByteArray rawTime = logFile.read(timeLen);
            return parseTimestamp(rawTime);
        }
    }

    // Otherwise, if we never find a message, return a failure code of 0.
    return 0;
}

void QGCMAVLinkLogPlayer::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

/**
 * Implement paintEvent() so that stylesheets work for our custom widget.
 */
void QGCMAVLinkLogPlayer::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

/// @brief Called when playback is complete
void QGCMAVLinkLogPlayer::_finishPlayback(void)
{
    pause();
    
    QString status = tr("Flight Data replay complete");
    ui->logStatsLabel->setText(status);
    
    // Note that we explicitly set the slider to 100%, as it may not hit that by itself depending on log file size.
    updatePositionSliderUi(100.0f);
    
    emit logFileEndReached();
    
    mavlink->suspendLogForReplay(false);
    LinkManager::instance()->setConnectionsAllowed();
}

/// @brief Called when an error occurs during playback to reset playback system state.
void QGCMAVLinkLogPlayer::_playbackError(void)
{
    pause();
    
    logFile.close();
    logFile.setFileName("");
    
    ui->logFileNameLabel->setText(tr("No Flight Data selected"));    
    
    // Disable playback controls
    ui->playButton->setEnabled(false);
    ui->speedSlider->setEnabled(false);
    ui->positionSlider->setEnabled(false);
    ui->speedLabel->setEnabled(false);
    ui->logFileNameLabel->setEnabled(false);
    ui->logStatsLabel->setEnabled(false);
}
