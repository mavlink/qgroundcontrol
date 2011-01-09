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
    ui(new Ui::QGCMAVLinkLogPlayer)
{
    ui->setupUi(this);
    ui->gridLayout->setAlignment(Qt::AlignTop);

    // Setup timer
    connect(&loopTimer, SIGNAL(timeout()), this, SLOT(logLoop()));

    // Setup buttons
    connect(ui->selectFileButton, SIGNAL(clicked()), this, SLOT(selectLogFile()));
    connect(ui->pauseButton, SIGNAL(clicked()), this, SLOT(pause()));
    connect(ui->playButton, SIGNAL(clicked()), this, SLOT(play()));
    connect(ui->speedSlider, SIGNAL(valueChanged(int)), this, SLOT(setAccelerationFactorInt(int)));

    setAccelerationFactorInt(49);
    ui->speedSlider->setValue(49);
}

QGCMAVLinkLogPlayer::~QGCMAVLinkLogPlayer()
{
    delete ui;
}

void QGCMAVLinkLogPlayer::play()
{
    if (logFile.isOpen())
    {
        ui->pauseButton->setChecked(false);
        ui->selectFileButton->setEnabled(false);
        if (logLink != NULL)
        {
            delete logLink;
        }
        logLink = new MAVLinkSimulationLink("");

        // Start timer
        loopTimer.start(1);
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
    loopTimer.stop();
    ui->playButton->setChecked(false);
    ui->selectFileButton->setEnabled(true);
    delete logLink;
    logLink = NULL;
}

void QGCMAVLinkLogPlayer::reset()
{
    pause();
    loopCounter = 0;
    logFile.reset();
    ui->pauseButton->setChecked(true);
    ui->progressBar->setValue(0);
    startTime = 0;
}

void QGCMAVLinkLogPlayer::selectLogFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Specify MAVLink log file name"), QDesktopServices::storageLocation(QDesktopServices::DesktopLocation), tr("MAVLink Logfile (*.mavlink);;"));

    loadLogFile(fileName);
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

    //qDebug() << "FACTOR:" << accelerationFactor;

    ui->speedLabel->setText(tr("Speed: %1X").arg(accelerationFactor, 0, 'f', 2));
}

void QGCMAVLinkLogPlayer::loadLogFile(const QString& file)
{
    // Ensure that the playback process is stopped
    if (logFile.isOpen())
    {
        loopTimer.stop();
        logFile.reset();
        logFile.close();
    }
    logFile.setFileName(file);

    if (!logFile.open(QFile::ReadOnly))
    {
        MainWindow::instance()->showCriticalMessage(tr("The selected logfile is unreadable"), tr("Please make sure that the file %1 is readable or select a different file").arg(file));
        logFile.setFileName("");
    }
    else
    {
        QFileInfo logFileInfo(file);
        logFile.reset();
        startTime = 0;
        ui->logFileNameLabel->setText(tr("%1").arg(logFileInfo.baseName()));
        ui->logStatsLabel->setText(tr("Log: %2 MB, %3 packets").arg(logFileInfo.size()/1000000.0f, 0, 'f', 2).arg(logFileInfo.size()/(MAVLINK_MAX_PACKET_LEN+sizeof(quint64))));
    }
}

union log64
{
    quint64 q;
    const char* b;
};

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
    const int packetLen = MAVLINK_MAX_PACKET_LEN;
    const int timeLen = sizeof(quint64);
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
    mavlink->receiveBytes(logLink, packet);

    // Check if reached end of file before reading next timestamp
    if (chunk.length() < timeLen+packetLen || logFile.atEnd())
    {
        // Reached end of file
        reset();

        QString status = tr("Reached end of MAVLink log file.");
        ui->logStatsLabel->setText(status);
        ui->progressBar->setValue(100);
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

        if (nextExecutionTime < 5)
        {
            logLoop();
        }
        else
        {
            loopTimer.start(nextExecutionTime);
        }
//        loopTimer.start(20);

        if (loopCounter % 40 == 0)
        {
            QFileInfo logFileInfo(logFile);
            int progress = 100.0f*(loopCounter/(float)(logFileInfo.size()/(MAVLINK_MAX_PACKET_LEN+sizeof(quint64))));
            //qDebug() << "Progress:" << progress;
            ui->progressBar->setValue(progress);
        }
        loopCounter++;
        // Ui update: Only every 20 messages
        // to prevent flickering and high CPU load

        // Update status label
        // Update progress bar
    }
}

void QGCMAVLinkLogPlayer::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
