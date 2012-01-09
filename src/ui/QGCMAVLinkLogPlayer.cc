#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>

#include "MainWindow.h"
#include "QGCMAVLinkLogPlayer.h"
#include "QGC.h"
#include "ui_QGCMAVLinkLogPlayer.h"

QGCMAVLinkLogPlayer::QGCMAVLinkLogPlayer(MAVLinkProtocol* mavlink, QWidget *parent) :
    QWidget(parent),
    lineCounter(0),
    totalLines(0),
    startTime(0),
    endTime(0),
    currentStartTime(0),
    accelerationFactor(1.0f),
    mavlink(mavlink),
    logLink(NULL),
    loopCounter(0),
    mavlinkLogFormat(true),
    binaryBaudRate(57600),
    isPlaying(false),
    currPacketCount(0),
    ui(new Ui::QGCMAVLinkLogPlayer)
{
    ui->setupUi(this);
    ui->gridLayout->setAlignment(Qt::AlignTop);

    // Connect protocol
    connect(this, SIGNAL(bytesReady(LinkInterface*,QByteArray)), mavlink, SLOT(receiveBytes(LinkInterface*,QByteArray)));

    // Setup timer
    connect(&loopTimer, SIGNAL(timeout()), this, SLOT(logLoop()));

    // Setup buttons
    connect(ui->selectFileButton, SIGNAL(clicked()), this, SLOT(selectLogFile()));
    connect(ui->playButton, SIGNAL(clicked()), this, SLOT(playPauseToggle()));
    connect(ui->speedSlider, SIGNAL(valueChanged(int)), this, SLOT(setAccelerationFactorInt(int)));
    connect(ui->positionSlider, SIGNAL(valueChanged(int)), this, SLOT(jumpToSliderVal(int)));
    connect(ui->positionSlider, SIGNAL(sliderPressed()), this, SLOT(pause()));

    setAccelerationFactorInt(49);
    ui->speedSlider->setValue(49);
    ui->positionSlider->setValue(ui->positionSlider->minimum());
}

QGCMAVLinkLogPlayer::~QGCMAVLinkLogPlayer()
{
    delete ui;
}

void QGCMAVLinkLogPlayer::playPause(bool enabled)
{
    if (enabled)
    {
        play();
    }
    else
    {
        pause();
    }
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
    if (logFile.isOpen())
    {
        ui->selectFileButton->setEnabled(false);
        if (logLink)
        {
            logLink->disconnect();
            LinkManager::instance()->removeLink(logLink);
            delete logLink;
        }
        logLink = new MAVLinkSimulationLink("");

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
            loopTimer.start(interval*accelerationFactor);
        }
        isPlaying = true;
        ui->logStatsLabel->setText(tr("Started playing.."));
        ui->playButton->setIcon(QIcon(":images/actions/media-playback-pause.svg"));
    }
    else
    {
        ui->playButton->setChecked(false);
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText(tr("No logfile selected"));
        msgBox.setInformativeText(tr("Please select first a MAVLink log file before playing it."));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }
}

void QGCMAVLinkLogPlayer::pause()
{
    isPlaying = false;
    loopTimer.stop();
    ui->playButton->setIcon(QIcon(":images/actions/media-playback-start.svg"));
    ui->selectFileButton->setEnabled(true);
    if (logLink)
    {
        logLink->disconnect();
        LinkManager::instance()->removeLink(logLink);
        delete logLink;
        logLink = NULL;
    }
}

bool QGCMAVLinkLogPlayer::reset(int packetIndex)
{
    // Reset only for valid values
    const unsigned int packetSize = timeLen + packetLen;
    if (packetIndex >= 0 && packetIndex*packetSize <= logFile.size() - packetSize)
    {
        bool result = true;
        pause();
        loopCounter = 0;
        logFile.reset();

        if (!logFile.seek(packetIndex*packetSize))
        {
            // Fallback: Start from scratch
            logFile.reset();
            ui->logStatsLabel->setText(tr("Changing packet index failed, back to start."));
            result = false;
        }

        ui->playButton->setIcon(QIcon(":images/actions/media-playback-start.svg"));
        ui->positionSlider->blockSignals(true);
        int sliderVal = (packetIndex / (double)(logFile.size()/packetSize)) * (ui->positionSlider->maximum() - ui->positionSlider->minimum());
        ui->positionSlider->setValue(sliderVal);
        ui->positionSlider->blockSignals(false);
        startTime = 0;
        return result;
    }
    else
    {
        return false;
    }
}

bool QGCMAVLinkLogPlayer::selectLogFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Specify MAVLink log file name to replay"), QDesktopServices::storageLocation(QDesktopServices::DesktopLocation), tr("MAVLink or Binary Logfile (*.mavlink *.bin *.log)"));

    if (fileName == "")
    {
        return false;
    }
    else
    {
        return loadLogFile(fileName);
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
        loopTimer.start(interval/accelerationFactor);
    }

    //qDebug() << "FACTOR:" << accelerationFactor;

    ui->speedLabel->setText(tr("Speed: %1X").arg(accelerationFactor, 5, 'f', 2, '0'));
}

bool QGCMAVLinkLogPlayer::loadLogFile(const QString& file)
{
    // Check if logging is still enabled
    if (mavlink->loggingEnabled())
    {
        mavlink->enableLogging(false);
        MainWindow::instance()->showInfoMessage(tr("MAVLink Logging Stopped during Replay"), tr("MAVLink logging has been stopped during the log replay. To re-enable logging, use the link properties in the communication menu."));
    }

    // Ensure that the playback process is stopped
    if (logFile.isOpen())
    {
        pause();
        logFile.close();
    }
    logFile.setFileName(file);

    if (!logFile.open(QFile::ReadOnly))
    {
        MainWindow::instance()->showCriticalMessage(tr("The selected logfile is unreadable"), tr("Please make sure that the file %1 is readable or select a different file").arg(file));
        logFile.setFileName("");
        return false;
    }
    else
    {
        QFileInfo logFileInfo(file);
        logFile.reset();
        startTime = 0;
        ui->logFileNameLabel->setText(tr("%1").arg(logFileInfo.baseName()));

        // Select if binary or MAVLink log format is used
        mavlinkLogFormat = file.endsWith(".mavlink");

        if (mavlinkLogFormat)
        {
            // Get the time interval from the logfile
            QByteArray timestamp = logFile.read(timeLen);

            // First timestamp
            quint64 starttime = *((quint64*)(timestamp.constData()));

            // Last timestamp
            logFile.seek(logFile.size()-packetLen-timeLen);
            QByteArray timestamp2 = logFile.read(timeLen);
            quint64 endtime = *((quint64*)(timestamp2.constData()));
            // Reset everything
            logFile.reset();

            qDebug() << "Starttime:" << starttime << "End:" << endtime;

            // WARNING: Order matters in this computation
            int seconds = (endtime - starttime)/1000000;
            int minutes = seconds / 60;
            int hours = minutes / 60;
            seconds -= 60*minutes;
            minutes -= 60*hours;

            QString timelabel = tr("%1h:%2m:%3s").arg(hours, 2).arg(minutes, 2).arg(seconds, 2);
            currPacketCount = logFileInfo.size()/(MAVLINK_MAX_PACKET_LEN+sizeof(quint64));
            ui->logStatsLabel->setText(tr("%2 MB, %3 packets, %4").arg(logFileInfo.size()/1000000.0f, 0, 'f', 2).arg(currPacketCount).arg(timelabel));
        }
        else
        {
            // Load in binary mode

            // Set baud rate if any present
            QStringList parts = logFileInfo.baseName().split("_");

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

        // Reset current state
        reset(0);

        return true;
    }
}

/**
 * Jumps to the current percentage of the position slider
 */
void QGCMAVLinkLogPlayer::jumpToSliderVal(int slidervalue)
{
    loopTimer.stop();
    // Set the logfile to the correct percentage and
    // align to the timestamp values
    int packetCount = logFile.size() / (packetLen + timeLen);
    int packetIndex = (packetCount - 1) * (slidervalue / (double)(ui->positionSlider->maximum() - ui->positionSlider->minimum()));

    // Do only accept valid jumps
    if (reset(packetIndex))
    {
        if (mavlinkLogFormat)
        {
            ui->logStatsLabel->setText(tr("Jumped to packet %1").arg(packetIndex));
        }
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
    if (mavlinkLogFormat)
    {
        bool ok;

        // First check initialization
        if (startTime == 0)
        {
            QByteArray startBytes = logFile.read(timeLen);

            // Check if the correct number of bytes could be read
            if (startBytes.length() != timeLen)
            {
                ui->logStatsLabel->setText(tr("Error reading first %1 bytes").arg(timeLen));
                MainWindow::instance()->showCriticalMessage(tr("Failed loading MAVLink Logfile"), tr("Error reading first %1 bytes from logfile. Got %2 instead of %1 bytes. Is the logfile readable?").arg(timeLen).arg(startBytes.length()));
                reset();
                return;
            }

            // Convert data to timestamp
            startTime = *((quint64*)(startBytes.constData()));
            currentStartTime = QGC::groundTimeUsecs();
            ok = true;

            //qDebug() << "START TIME: " << startTime;

            // Check if these bytes could be correctly decoded
            if (!ok)
            {
                ui->logStatsLabel->setText(tr("Error decoding first timestamp, aborting."));
                MainWindow::instance()->showCriticalMessage(tr("Failed loading MAVLink Logfile"), tr("Could not load initial timestamp from file %1. Is the file corrupted?").arg(logFile.fileName()));
                reset();
                return;
            }
        }


        // Initialization seems fine, load next chunk
        QByteArray chunk = logFile.read(timeLen+packetLen);
        QByteArray packet = chunk.mid(0, packetLen);

        // Emit this packet
        emit bytesReady(logLink, packet);

        // Check if reached end of file before reading next timestamp
        if (chunk.length() < timeLen+packetLen || logFile.atEnd())
        {
            // Reached end of file
            reset();

            QString status = tr("Reached end of MAVLink log file.");
            ui->logStatsLabel->setText(status);
            MainWindow::instance()->showStatusMessage(status);
            return;
        }

        // End of file not reached, read next timestamp
        QByteArray rawTime = chunk.mid(packetLen);

        // This is the timestamp of the next packet
        quint64 time = *((quint64*)(rawTime.constData()));
        ok = true;
        if (!ok)
        {
            // Convert it to 64bit number
            QString status = tr("Time conversion error during log replay. Continuing..");
            ui->logStatsLabel->setText(status);
            MainWindow::instance()->showStatusMessage(status);
        }
        else
        {
            // Normal processing, passed all checks
            // start timer to match time offset between
            // this and next packet


            // Offset in ms
            qint64 timediff = (time - startTime)/accelerationFactor;

            // Immediately load any data within
            // a 3 ms interval

            int nextExecutionTime = (((qint64)currentStartTime + (qint64)timediff) - (qint64)QGC::groundTimeUsecs())/1000;

            //qDebug() << "nextExecutionTime:" << nextExecutionTime << "QGC START TIME:" << currentStartTime << "LOG START TIME:" << startTime;

            if (nextExecutionTime < 2)
            {
                logLoop();
            }
            else
            {
                loopTimer.start(nextExecutionTime);
            }
        }
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
            // Reached end of file
            reset();

            QString status = tr("Reached end of binary log file.");
            ui->logStatsLabel->setText(status);
            MainWindow::instance()->showStatusMessage(status);
            return;
        }
    }
    // Ui update: Only every 20 messages
    // to prevent flickering and high CPU load

    // Update status label
    // Update progress bar
    if (loopCounter % 40 == 0 || currPacketCount < 500)
    {
        QFileInfo logFileInfo(logFile);
        int progress = (ui->positionSlider->maximum()-ui->positionSlider->minimum())*(logFile.pos()/static_cast<float>(logFileInfo.size()));
        //qDebug() << "Progress:" << progress;
        ui->positionSlider->blockSignals(true);
        ui->positionSlider->setValue(progress);
        ui->positionSlider->blockSignals(false);
    }
    loopCounter++;
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
