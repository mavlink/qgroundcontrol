#include "QGCPX4SensorCalibration.h"
#include "ui_QGCPX4SensorCalibration.h"
#include <UASManager.h>
#include <QMenu>
#include <QScrollBar>
#include <QDebug>

QGCPX4SensorCalibration::QGCPX4SensorCalibration(QWidget *parent) :
    QWidget(parent),
    activeUAS(NULL),
    clearAction(new QAction(tr("Clear Text"), this)),
    accelStarted(false),
    gyroStarted(false),
    magStarted(false),
    ui(new Ui::QGCPX4SensorCalibration)
{
    accelAxes << "x+";
    accelAxes << "x-";
    accelAxes << "y+";
    accelAxes << "y-";
    accelAxes << "z+";
    accelAxes << "z-";


    ui->setupUi(this);
    connect(clearAction, SIGNAL(triggered()), ui->textView, SLOT(clear()));

    connect(ui->gyroButton, SIGNAL(clicked()), this, SLOT(gyroButtonClicked()));
    connect(ui->magButton, SIGNAL(clicked()), this, SLOT(magButtonClicked()));
    connect(ui->accelButton, SIGNAL(clicked()), this, SLOT(accelButtonClicked()));

    ui->gyroButton->setEnabled(false);
    ui->magButton->setEnabled(false);
    ui->accelButton->setEnabled(false);

    setInstructionImage(":/files/images/px4/calibration/accel_z-.png");

    setObjectName("PX4_SENSOR_CALIBRATION");

    setStyleSheet("QScrollArea { border: 0px; } QPlainTextEdit { border: 0px }");

    setActiveUAS(UASManager::instance()->getActiveUAS());
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
    ui->progressBar->setValue(0);
}

QGCPX4SensorCalibration::~QGCPX4SensorCalibration()
{
    delete ui;
}

void QGCPX4SensorCalibration::setInstructionImage(const QString &path)
{
    instructionIcon.load(path);

    int w = ui->iconLabel->width();
    int h = ui->iconLabel->height();

    ui->iconLabel->setPixmap(instructionIcon.scaled(w, h, Qt::KeepAspectRatio));
}

void QGCPX4SensorCalibration::resizeEvent(QResizeEvent* event)
{

    int w = ui->iconLabel->width();
    int h = ui->iconLabel->height();

    ui->iconLabel->setPixmap(instructionIcon.scaled(w, h, Qt::KeepAspectRatio));

    QWidget::resizeEvent(event);
}

void QGCPX4SensorCalibration::setActiveUAS(UASInterface* uas)
{
    if (!uas)
        return;

    if (activeUAS) {
        disconnect(uas, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(handleTextMessage(int,int,int,QString)));
        ui->textView->clear();
    }

    ui->gyroButton->setEnabled(true);
    ui->magButton->setEnabled(true);
    ui->accelButton->setEnabled(true);

    connect(uas, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(handleTextMessage(int,int,int,QString)));
    activeUAS = uas;
}

void QGCPX4SensorCalibration::handleTextMessage(int uasid, int compId, int severity, QString text)
{
    if (text.startsWith("[cmd]") ||
        text.startsWith("[mavlink pm]"))
        return;

    if (text.contains("progress")) {
        qDebug() << "PROGRESS:" << text;
        QString percent = text.split("<").last().split(">").first();
        qDebug() << "PERCENT CANDIDATE" << percent;
        bool ok;
        int p = percent.toInt(&ok);
        if (ok)
            ui->progressBar->setValue(p);
        return;
    }


    if (text.contains("accel")) {
        qDebug() << "ACCEL" << text;

        if (text.startsWith("accel meas started: ")) {
            QString axis = text.split("meas started: ").last().right(2);
            qDebug() << "AXIS" << axis << "STR" << text;
            setInstructionImage(QString(":/files/images/px4/calibration/accel_%1.png").arg(axis));

        }
    }

    if (text.startsWith("meas result for")) {

        QString axis = text.split("meas result for ").last().left(2);

        qDebug() << "ACCELDONE AXIS" << axis << "STR" << text;

        for (int i = 0; i < 6; i++)
        {
            if (accelAxes[i] == axis)
                accelDone[i] = true;

            if (!accelDone[i]) {
                setInstructionImage(QString(":/files/images/px4/calibration/accel_%1.png").arg(accelAxes[i]));
                ui->calibrationExplanationLabel->setText(tr("Axis %1 completed. Please rotate to a different axis, e.g. the one shown here.").arg(axis));
            }
        }
    }

    if (text.contains("please rotate in a figure 8")) {
        ui->calibrationExplanationLabel->setText(tr("Rotate the system around all its axes, e.g. in a figure eight."));
        setInstructionImage(":/files/images/px4/calibration/mag_calibration_figure8.png");
    }

    if (text.contains("accel calibration done")) {
        accelStarted = false;
        // XXX use a confirmation image or something
        setInstructionImage(":/files/images/px4/calibration/accel_z-.png");
        ui->calibrationExplanationLabel->setText(tr("Accelerometer calibration completed. Parameters permanently stored."));
    }

    if (text.contains("gyro calibration done")) {
        gyroStarted = false;
        // XXX use a confirmation image or something
        setInstructionImage(":/files/images/px4/calibration/accel_z-.png");
        ui->calibrationExplanationLabel->setText(tr("Gyroscope calibration completed. Parameters permanently stored."));
    }

    if (text.contains("mag calibration done")) {
        magStarted = false;
        // XXX use a confirmation image or something
        setInstructionImage(":/files/images/px4/calibration/accel_z-.png");
        ui->calibrationExplanationLabel->setText(tr("Magnetometer calibration completed. Parameters permanently stored."));
    }

    ui->instructionLabel->setText(QString("%1").arg(text));


    // XXX color messages according to severity

    QPlainTextEdit *msgWidget = ui->textView;

    //turn off updates while we're appending content to avoid breaking the autoscroll behavior
    msgWidget->setUpdatesEnabled(false);
    QScrollBar *scroller = msgWidget->verticalScrollBar();

    msgWidget->appendHtml(QString("%4").arg(text));

    // Ensure text area scrolls correctly
    scroller->setValue(scroller->maximum());
    msgWidget->setUpdatesEnabled(true);
}

void QGCPX4SensorCalibration::gyroButtonClicked()
{
    setInstructionImage(":/files/images/px4/calibration/accel_z-.png");
    activeUAS->executeCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    ui->progressBar->setValue(0);
    ui->calibrationExplanationLabel->setText(tr("Please do not move the system at all and wait for calibration to complete."));
}

void QGCPX4SensorCalibration::magButtonClicked()
{
    setInstructionImage(":/files/images/px4/calibration/accel_z-.png");
    activeUAS->executeCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    ui->progressBar->setValue(0);
    ui->calibrationExplanationLabel->setText(tr("Please put the system in a rest position and wait for instructions."));
}

void QGCPX4SensorCalibration::accelButtonClicked()
{
    setInstructionImage(":/files/images/px4/calibration/accel_z-.png");
    activeUAS->executeCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0);
    ui->progressBar->setValue(0);
    accelStarted = true;
    accelDone[0] = false;
    accelDone[1] = false;
    accelDone[2] = false;
    accelDone[3] = false;
    accelDone[4] = false;
    accelDone[5] = false;

    ui->calibrationExplanationLabel->setText(tr("Please hold the system very still in the shown orientations. Start with the one shown."));
}

void QGCPX4SensorCalibration::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.addAction(clearAction);
    menu.exec(event->globalPos());
}
