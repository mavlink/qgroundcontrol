#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>

#include "QGCMAVLinkLogPlayer.h"
#include "ui_QGCMAVLinkLogPlayer.h"

QGCMAVLinkLogPlayer::QGCMAVLinkLogPlayer(MAVLinkProtocol* mavlink, QWidget *parent) :
    QWidget(parent),
    lineCounter(0),
    totalLines(0),
    startTime(0),
    endTime(0),
    currentTime(0),
    mavlink(mavlink),
    ui(new Ui::QGCMAVLinkLogPlayer)
{
    ui->setupUi(this);
    ui->gridLayout->setAlignment(Qt::AlignTop);

    // Setup buttons
    connect(ui->selectFileButton, SIGNAL(clicked()), this, SLOT(selectLogFile()));
    connect(ui->pauseButton, SIGNAL(clicked()), this, SLOT(pause()));
    connect(ui->playButton, SIGNAL(clicked()), this, SLOT(play()));

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
    ui->playButton->setChecked(false);
}

void QGCMAVLinkLogPlayer::selectLogFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Specify MAVLink log file name"), QDesktopServices::storageLocation(QDesktopServices::DesktopLocation), tr("MAVLink Logfile (*.mavlink);;"));

    loadLogFile(fileName);
}

void QGCMAVLinkLogPlayer::loadLogFile(const QString& file)
{
    logFile.setFileName(file);

    if (!logFile.open(QFile::ReadOnly))
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(tr("The selected logfile is unreadable"));
        msgBox.setInformativeText(tr("Please make sure that the file %1 is readable or select a different file").arg(file));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        logFile.setFileName("");
    }
    else
    {
        ui->logFileNameLabel->setText(tr("%1").arg(file));
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
void QGCMAVLinkLogPlayer::readLine()
{

    // Ui update: Only every 20 messages
    // to prevent flickering and high CPU load

    // Update status label
    // Update progress bar
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
