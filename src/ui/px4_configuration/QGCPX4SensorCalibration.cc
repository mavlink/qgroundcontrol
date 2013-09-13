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

    ui->autopilotComboBox->setEnabled(false);
    ui->magComboBox->setEnabled(false);

    setInstructionImage(":/files/images/px4/calibration/accel_z-.png");
    setAutopilotImage(":/files/images/px4/calibration/accel_z-.png");
    setGpsImage(":/files/images/px4/calibration/accel_z-.png");

    // Fill combo boxes
    ui->autopilotComboBox->addItem(tr("Default Orientation"), 0);
    ui->autopilotComboBox->addItem(tr("ROTATION_YAW_45"), 1);
    ui->autopilotComboBox->addItem(tr("ROTATION_YAW_90"), 2);
    ui->autopilotComboBox->addItem(tr("ROTATION_YAW_135"), 3);
    ui->autopilotComboBox->addItem(tr("ROTATION_YAW_180"), 4);
    ui->autopilotComboBox->addItem(tr("ROTATION_YAW_225"), 5);
    ui->autopilotComboBox->addItem(tr("ROTATION_YAW_270"), 6);
    ui->autopilotComboBox->addItem(tr("ROTATION_YAW_315"), 7);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_180"), 8);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_180_YAW_45"), 9);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_180_YAW_90"), 10);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_180_YAW_135"), 11);
    ui->autopilotComboBox->addItem(tr("ROTATION_PITCH_180"), 12);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_180_YAW_225"), 13);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_180_YAW_270"), 14);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_180_YAW_315"), 15);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_90"), 16);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_90_YAW_45"), 17);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_90_YAW_90"), 18);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_90_YAW_135"), 19);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_270"), 20);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_270_YAW_45"), 21);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_270_YAW_90"), 22);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_270_YAW_135"), 23);
    ui->autopilotComboBox->addItem(tr("ROTATION_PITCH_90"), 24);
    ui->autopilotComboBox->addItem(tr("ROTATION_PITCH_270"), 25);
    ui->autopilotComboBox->addItem(tr("ROTATION_PITCH_180_YAW_90"), 26);
    ui->autopilotComboBox->addItem(tr("ROTATION_PITCH_180_YAW_270"), 27);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_90_PITCH_90"), 28);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_180_PITCH_90"), 29);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_270_PITCH_90"), 30);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_90_PITCH_180"), 31);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_270_PITCH_180"), 32);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_90_PITCH_270"), 33);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_180_PITCH_270"), 34);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_270_PITCH_270"), 35);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_90_PITCH_180_YAW_90"), 36);
    ui->autopilotComboBox->addItem(tr("ROTATION_ROLL_90_YAW_270"), 37);

    ui->magComboBox->addItem(tr("Default Orientation"), 0);
    ui->magComboBox->addItem(tr("ROTATION_YAW_45"), 1);
    ui->magComboBox->addItem(tr("ROTATION_YAW_90"), 2);
    ui->magComboBox->addItem(tr("ROTATION_YAW_135"), 3);
    ui->magComboBox->addItem(tr("ROTATION_YAW_180"), 4);
    ui->magComboBox->addItem(tr("ROTATION_YAW_225"), 5);
    ui->magComboBox->addItem(tr("ROTATION_YAW_270"), 6);
    ui->magComboBox->addItem(tr("ROTATION_YAW_315"), 7);
    ui->magComboBox->addItem(tr("ROTATION_ROLL_180"), 8);
    ui->magComboBox->addItem(tr("ROTATION_ROLL_180_YAW_45"), 9);
    ui->magComboBox->addItem(tr("ROTATION_ROLL_180_YAW_90"), 10);
    ui->magComboBox->addItem(tr("ROTATION_ROLL_180_YAW_135"), 11);
    ui->magComboBox->addItem(tr("ROTATION_PITCH_180"), 12);
    ui->magComboBox->addItem(tr("ROTATION_ROLL_180_YAW_225"), 13);
    ui->magComboBox->addItem(tr("ROTATION_ROLL_180_YAW_270"), 14);
    ui->magComboBox->addItem(tr("ROTATION_ROLL_180_YAW_315"), 15);
    ui->magComboBox->addItem(tr("ROTATION_ROLL_90"), 16);
    ui->magComboBox->addItem(tr("ROTATION_ROLL_90_YAW_45"), 17);
    ui->magComboBox->addItem(tr("ROTATION_ROLL_90_YAW_90"), 18);
    ui->magComboBox->addItem(tr("ROTATION_ROLL_90_YAW_135"), 19);
    ui->magComboBox->addItem(tr("ROTATION_ROLL_270"), 20);
    ui->magComboBox->addItem(tr("ROTATION_ROLL_270_YAW_45"), 21);
    ui->magComboBox->addItem(tr("ROTATION_ROLL_270_YAW_90"), 22);
    ui->magComboBox->addItem(tr("ROTATION_ROLL_270_YAW_135"), 23);
    ui->magComboBox->addItem(tr("ROTATION_PITCH_90"), 24);
    ui->magComboBox->addItem(tr("ROTATION_PITCH_270"), 25);

    setObjectName("PX4_SENSOR_CALIBRATION");

    setStyleSheet("QScrollArea { border: 0px; } QPlainTextEdit { border: 0px }");

    setActiveUAS(UASManager::instance()->getActiveUAS());
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
    ui->progressBar->setValue(0);

    connect(ui->autopilotComboBox, SIGNAL(activated(int)), this, SLOT(setAutopilotOrientation(int)));
    connect(ui->magComboBox, SIGNAL(activated(int)), this, SLOT(setGpsOrientation(int)));

    updateIcons();
}

QGCPX4SensorCalibration::~QGCPX4SensorCalibration()
{
    delete ui;
}

void QGCPX4SensorCalibration::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    Q_UNUSED(uas);
    Q_UNUSED(component);

    int index = (int)value.toFloat();

    if (parameterName.contains("SENS_BOARD_ROT"))
    {
        ui->autopilotComboBox->setCurrentIndex(index);
        setAutopilotImage(index);
        ui->autopilotComboBox->setEnabled(true);
    }

    if (parameterName.contains("SENS_EXT_MAG_ROT"))
    {
        ui->magComboBox->setCurrentIndex(index);
        setGpsImage(index);
        ui->magComboBox->setEnabled(true);
    }

    // Check mag calibration naively
    if (parameterName.contains("SENS_MAG_XOFF")) {
        float offset = value.toFloat();
        if (offset < 0.000001f && offset > -0.000001f) {
            // Must be zero, not good
            setMagCalibrated(false);
        } else {
            setMagCalibrated(true);
        }
    }

    // Check gyro calibration naively
    if (parameterName.contains("SENS_GYRO_XOFF")) {
        float offset = value.toFloat();
        if (offset < 0.000001f && offset > -0.000001f) {
            // Must be zero, not good
            setGyroCalibrated(false);
        } else {
            setGyroCalibrated(true);
        }
    }

    // Check accel calibration naively
    if (parameterName.contains("SENS_ACC_XOFF")) {
        float offset = value.toFloat();
        if (offset < 0.000001f && offset > -0.000001f) {
            // Must be zero, not good
            setAccelCalibrated(false);
        } else {
            setAccelCalibrated(true);
        }
    }
}

void QGCPX4SensorCalibration::setMagCalibrated(bool calibrated)
{
    if (calibrated) {
        ui->magLabel->setText(tr("MAG CALIBRATED"));
        ui->magLabel->setStyleSheet("QLabel { color: #FFFFFF;"
                                    "background-color: #20AA20;"
                                    "}");
    } else {
        ui->magLabel->setText(tr("MAG UNCALIBRATED"));
        ui->magLabel->setStyleSheet("QLabel { color: #FFFFFF;"
                                    "background-color: #FF0037;"
                                    "}");
    }
}

void QGCPX4SensorCalibration::setGyroCalibrated(bool calibrated)
{
    if (calibrated) {
        ui->gyroLabel->setText(tr("GYRO CALIBRATED"));
        ui->gyroLabel->setStyleSheet("QLabel { color: #FFFFFF;"
                                    "background-color: #20AA20;"
                                    "}");
    } else {
        ui->gyroLabel->setText(tr("GYRO UNCALIBRATED"));
        ui->gyroLabel->setStyleSheet("QLabel { color: #FFFFFF;"
                                    "background-color: #FF0037;"
                                    "}");
    }
}

void QGCPX4SensorCalibration::setAccelCalibrated(bool calibrated)
{
    if (calibrated) {
        ui->accelLabel->setText(tr("ACCEL CALIBRATED"));
        ui->accelLabel->setStyleSheet("QLabel { color: #FFFFFF;"
                                    "background-color: #20AA20;"
                                    "}");
    } else {
        ui->accelLabel->setText(tr("ACCEL UNCALIBRATED"));
        ui->accelLabel->setStyleSheet("QLabel { color: #FFFFFF;"
                                    "background-color: #FF0037;"
                                    "}");
    }
}

void QGCPX4SensorCalibration::setInstructionImage(const QString &path)
{
    instructionIcon.load(path);

    int w = ui->iconLabel->width();
    int h = ui->iconLabel->height();

    ui->iconLabel->setPixmap(instructionIcon.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void QGCPX4SensorCalibration::setAutopilotImage(int index)
{
    setAutopilotImage(QString(":/files/images/px4/calibration/pixhawk_%1.png").arg(index, 2, 10, QChar('0')));
}

void QGCPX4SensorCalibration::setGpsImage(int index)
{
    setGpsImage(QString(":/files/images/px4/calibration/3dr_gps/gps_%1.png").arg(index, 2, 10, QChar('0')));
}

void QGCPX4SensorCalibration::setAutopilotOrientation(int index)
{
    if (activeUAS) {
        activeUAS->getParamManager()->setPendingParam(0, "SENS_BOARD_ROT", (int)index);
        activeUAS->getParamManager()->sendPendingParameters(true);
    }
}

void QGCPX4SensorCalibration::setGpsOrientation(int index)
{
    if (activeUAS) {
        activeUAS->getParamManager()->setPendingParam(0, "SENS_EXT_MAG_ROT", (int)index);
        activeUAS->getParamManager()->sendPendingParameters(true);
    }
}

void QGCPX4SensorCalibration::setAutopilotImage(const QString &path)
{
    autopilotIcon.load(path);

    int w = ui->autopilotLabel->width();
    int h = ui->autopilotLabel->height();

    ui->autopilotLabel->setPixmap(autopilotIcon.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void QGCPX4SensorCalibration::setGpsImage(const QString &path)
{
    gpsIcon.load(path);

    int w = ui->gpsLabel->width();
    int h = ui->gpsLabel->height();

    ui->gpsLabel->setPixmap(gpsIcon.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void QGCPX4SensorCalibration::updateIcons()
{
    int w = ui->iconLabel->width();
    int h = ui->iconLabel->height();
    ui->iconLabel->setPixmap(instructionIcon.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    int wa = ui->autopilotLabel->width();
    int ha = ui->autopilotLabel->height();
    ui->autopilotLabel->setPixmap(autopilotIcon.scaled(wa, ha, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    int wg = ui->gpsLabel->width();
    int hg = ui->gpsLabel->height();
    ui->gpsLabel->setPixmap(gpsIcon.scaled(wg, hg, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void QGCPX4SensorCalibration::resizeEvent(QResizeEvent* event)
{
    updateIcons();
    QWidget::resizeEvent(event);
}

void QGCPX4SensorCalibration::setActiveUAS(UASInterface* uas)
{
    if (!uas)
        return;

    if (activeUAS) {
        disconnect(activeUAS, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(handleTextMessage(int,int,int,QString)));
        disconnect(activeUAS, SIGNAL(parameterChanged(int,int,QString,QVariant)), this, SLOT(parameterChanged(int,int,QString,QVariant)));
        ui->textView->clear();
    }

    ui->gyroButton->setEnabled(true);
    ui->magButton->setEnabled(true);
    ui->accelButton->setEnabled(true);

    connect(uas, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(handleTextMessage(int,int,int,QString)));
    connect(uas, SIGNAL(parameterChanged(int,int,QString,QVariant)), this, SLOT(parameterChanged(int,int,QString,QVariant)));
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
        if (activeUAS) {
            activeUAS->requestParameter(0, "SENS_ACC_XOFF");
            activeUAS->requestParameter(0, "SENS_BOARD_ROT");
        }
    }

    if (text.contains("gyro calibration done")) {
        gyroStarted = false;
        // XXX use a confirmation image or something
        setInstructionImage(":/files/images/px4/calibration/accel_z-.png");
        if (activeUAS)
            activeUAS->requestParameter(0, "SENS_GYRO_XOFF");
    }

    if (text.contains("mag calibration done")
            || text.contains("magnetometer calibration completed")) {
        magStarted = false;
        // XXX use a confirmation image or something
        setInstructionImage(":/files/images/px4/calibration/accel_z-.png");
        if (activeUAS) {
            activeUAS->requestParameter(0, "SENS_MAG_XOFF");
            activeUAS->requestParameter(0, "SENS_EXT_MAG_ROT");
        }
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
