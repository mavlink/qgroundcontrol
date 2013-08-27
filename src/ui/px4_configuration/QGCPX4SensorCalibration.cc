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
    Q_UNUSED(uasid);
    Q_UNUSED(compId);
    Q_UNUSED(severity);

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

    ui->instructionLabel->setText(QString("%1").arg(text));


    if (text.contains("accel")) {
        qDebug() << "ACCEL" << text;

        if (text.startsWith("accel measurement started: ")) {
            QString axis = text.split("measurement started: ").last().left(2);
            qDebug() << "AXIS" << axis << "STR" << text;
            setInstructionImage(QString(":/files/images/px4/calibration/accel_%1.png").arg(axis));

        }
    }

    if (text.startsWith("directions left")) {
        for (int i = 0; i < 6; i++)
        {
            if (!text.contains(accelAxes[i])) {
                qDebug() << "FINISHED" << accelAxes[i];
                accelDone[i] = true;
            }
        }
    }

    if (text.startsWith("result for")) {

        QString axis = text.split("result for ").last().left(2);

        qDebug() << "ACCELDONE AXIS" << axis << "STR" << text;

        for (int i = 0; i < 6; i++)
        {
            if (axis == accelAxes[i])
                accelDone[i] = true;

            if (!accelDone[i]) {
                qDebug() << "NEW AXIS: " << accelAxes[i];
                setInstructionImage(QString(":/files/images/px4/calibration/accel_%1.png").arg(accelAxes[i]));
                ui->instructionLabel->setText(tr("Axis %1 completed. Please rotate like shown here.").arg(axis));
                break;
            }
        }
    }

    if (text.contains("please rotate in a figure 8")) {
        setInstructionImage(":/files/images/px4/calibration/mag_calibration_figure8.png");
    }

    if (text.contains("accel calibration done")) {
        accelStarted = false;
        // XXX use a confirmation image or something
        setInstructionImage(":/files/images/px4/calibration/accel_z-.png");
    }

    if (text.contains("gyro calibration done")) {
        gyroStarted = false;
        // XXX use a confirmation image or something
        setInstructionImage(":/files/images/px4/calibration/accel_z-.png");
    }

    if (text.contains("mag calibration done")
            || text.contains("magnetometer calibration completed")) {
        magStarted = false;
        // XXX use a confirmation image or something
        setInstructionImage(":/files/images/px4/calibration/accel_z-.png");
    }

    if (text.contains("accel calibration started")) {
        accelStarted = true;
        setInstructionImage(":/files/images/px4/calibration/accel_z-.png");
    }

    if (text.contains("gyro calibration started")) {
        gyroStarted = true;
        setInstructionImage(":/files/images/px4/calibration/accel_z-.png");
    }

    if (text.contains("mag calibration started")) {
        magStarted = false;
        setInstructionImage(":/files/images/px4/calibration/accel_z-.png");
    }


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
    ui->instructionLabel->setText(tr("Please do not move the system at all."));
}

void QGCPX4SensorCalibration::magButtonClicked()
{
    setInstructionImage(":/files/images/px4/calibration/accel_z-.png");
    activeUAS->executeCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    ui->progressBar->setValue(0);
    ui->instructionLabel->setText(tr("Please put the system in a rest position."));
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

    ui->instructionLabel->setText(tr("Please hold the system very still in the shown orientations."));
}

void QGCPX4SensorCalibration::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.addAction(clearAction);
    menu.exec(event->globalPos());
}
