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
    ui(new Ui::QGCPX4SensorCalibration)
{
    ui->setupUi(this);
    connect(clearAction, SIGNAL(triggered()), ui->textView, SLOT(clear()));

    connect(ui->gyroButton, SIGNAL(clicked()), this, SLOT(gyroButtonClicked()));
    connect(ui->magButton, SIGNAL(clicked()), this, SLOT(magButtonClicked()));
    connect(ui->accelButton, SIGNAL(clicked()), this, SLOT(accelButtonClicked()));
    connect(ui->diffPressureButton, SIGNAL(clicked()), this, SLOT(diffPressureButtonClicked()));

    connect(ui->logCheckBox, SIGNAL(clicked(bool)), ui->textView, SLOT(setVisible(bool)));
    ui->logCheckBox->setChecked(false);
    ui->textView->setVisible(false);

    ui->gyroButton->setEnabled(false);
    ui->magButton->setEnabled(false);
    ui->accelButton->setEnabled(false);

    ui->autopilotComboBox->setEnabled(false);
    ui->magComboBox->setEnabled(false);

    setInstructionImage(":/files/images/px4/calibration/accel_down.png");
    setAutopilotImage(":/files/images/px4/calibration/accel_down.png");
    setGpsImage(":/files/images/px4/calibration/accel_down.png");

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
    if (parameterName.contains("CAL_MAG0_ID")) {
        float id = value.toInt();
        if (id == 0) {
            // no sensor selected
            setMagCalibrated(false);
        } else {
            setMagCalibrated(true);
        }
    }

    // Check gyro calibration naively
    if (parameterName.contains("CAL_GYRO0_ID")) {
        float id = value.toInt();
        if (id == 0) {
            // no sensor selected
            setGyroCalibrated(false);
        } else {
            setGyroCalibrated(true);
        }
    }

    // Check accel calibration naively
    if (parameterName.contains("CAL_ACC0_ID")) {
        float id = value.toFloat();
        if (id == 0) {
            // no sensor selected
            setAccelCalibrated(false);
        } else {
            setAccelCalibrated(true);
        }
    }

    // Check differential pressure calibration naively
    if (parameterName.contains("SENS_DPRES_OFF")) {
      float offset = value.toFloat();
      if (offset < 0.000001f && offset > -0.000001f) {
          // Must be zero, not good
          setDiffPressureCalibrated(false);
      } else {
          setDiffPressureCalibrated(true);
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

void QGCPX4SensorCalibration::setDiffPressureCalibrated(bool calibrated)
{
    if (calibrated) {
        ui->diffPressureLabel->setText(tr("DIFF. PRESSURE CALIBRATED"));
        ui->diffPressureLabel->setStyleSheet("QLabel { color: #FFFFFF;"
                                    "background-color: #20AA20;"
                                    "}");
    } else {
        ui->diffPressureLabel->setText(tr("DIFF. PRESSURE UNCALIBRATED"));
        ui->diffPressureLabel->setStyleSheet("QLabel { color: #FFFFFF;"
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
    Q_UNUSED(index);
    // FIXME: This was referencing a non-existent png. Need to figure out what this was trying to do.
    //setAutopilotImage(QString(":/files/images/px4/calibration/pixhawk_%1.png").arg(index, 2, 10, QChar('0')));
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
    if (autopilotIcon.load(path)) {
        int w = ui->autopilotLabel->width();
        int h = ui->autopilotLabel->height();

        ui->autopilotLabel->setPixmap(autopilotIcon.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        qDebug() << "AutoPilot Icon image did not load" << path;
    }
}

void QGCPX4SensorCalibration::setGpsImage(const QString &path)
{
    if (gpsIcon.load(path)) {
        int w = ui->gpsLabel->width();
        int h = ui->gpsLabel->height();

        ui->gpsLabel->setPixmap(gpsIcon.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        qDebug() << "GPS Icon image did not load" << path;
    }
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
    connect(uas, SIGNAL(systemSpecsChanged(int)), this, SLOT(updateSystemSpecs(int)));
    activeUAS = uas;

    updateSystemSpecs(uas->getUASID());
    
    // If the parameters are ready, we aren't going to get paramterChanged signals. So fake them in order to make the UI work.
    if (uas->getParamManager()->parametersReady()) {
        QVariant value;
        static const char* rgParams[] = { "SENS_BOARD_ROT", "SENS_EXT_MAG_ROT", "SENS_MAG_XOFF", "SENS_GYRO_XOFF", "SENS_ACC_XOFF", "SENS_DPRES_OFF" };
        
        QGCUASParamManagerInterface* paramMgr = uas->getParamManager();
        
        for (size_t i=0; i<sizeof(rgParams)/sizeof(rgParams[0]); i++) {
            QVariant value;
            
            QList<int> compIds = paramMgr->getComponentForParam(rgParams[i]);
            Q_ASSERT(compIds.count() == 1);
            paramMgr->getParameterValue(compIds[0], rgParams[i], value);
            parameterChanged(uas->getUASID(), compIds[0], rgParams[i], value);
        }
    }
}

void QGCPX4SensorCalibration::updateSystemSpecs(int id)
{
    Q_UNUSED(id);

    if (activeUAS->isRotaryWing()) {
        // Users are confused by the config button
        ui->diffPressureButton->hide();
        ui->diffPressureLabel->hide();
    } else {
        ui->diffPressureButton->show();
        ui->diffPressureLabel->show();
    }
}

void QGCPX4SensorCalibration::handleTextMessage(int uasid, int compId, int severity, QString text)
{
    Q_UNUSED(uasid);
    Q_UNUSED(compId);
    Q_UNUSED(severity);

    if (text.startsWith("[cmd]") ||
        text.startsWith("[mavlink pm]"))
        return;

    if (text.contains("progress <")) {
        QString percent = text.split("<").last().split(">").first();
        bool ok;
        int p = percent.toInt(&ok);
        if (ok)
            ui->progressBar->setValue(p);
        return;
    }

    ui->instructionLabel->setText(QString("%1").arg(text));

    if (text.startsWith("Hold still, starting to measure ")) {
        QString axis = text.section(" ", -2, -2);
        setInstructionImage(QString(":/files/images/px4/calibration/accel_%1.png").arg(axis));
    }

    if (text.startsWith("pending: ")) {
        QString axis = text.section(" ", 1, 1);
        setInstructionImage(QString(":/files/images/px4/calibration/accel_%1.png").arg(axis));
    }

    if (text == "rotate in a figure 8 around all axis" /* support for old typo */
            || text == "rotate in a figure 8 around all axes" /* current version */) {
        setInstructionImage(":/files/images/px4/calibration/mag_calibration_figure8.png");
    }

    if (text.endsWith(" calibration: done") || text.endsWith(" calibration: failed")) {
        // XXX use a confirmation image or something
        setInstructionImage(":/files/images/px4/calibration/accel_down.png");
        if (text.endsWith(" calibration: done")) {
            ui->progressBar->setValue(100);
        } else {
            ui->progressBar->setValue(0);
        }

        if (activeUAS) {
            if (text.startsWith("accel ")) {
                activeUAS->requestParameter(0, "SENS_ACC_XOFF");
                activeUAS->requestParameter(0, "SENS_ACC_YOFF");
                activeUAS->requestParameter(0, "SENS_ACC_ZOFF");
                activeUAS->requestParameter(0, "SENS_ACC_XSCALE");
                activeUAS->requestParameter(0, "SENS_ACC_YSCALE");
                activeUAS->requestParameter(0, "SENS_ACC_ZSCALE");
                activeUAS->requestParameter(0, "SENS_BOARD_ROT");
            }
            if (text.startsWith("gyro ")) {
                activeUAS->requestParameter(0, "SENS_GYRO_XOFF");
                activeUAS->requestParameter(0, "SENS_GYRO_YOFF");
                activeUAS->requestParameter(0, "SENS_GYRO_ZOFF");
                activeUAS->requestParameter(0, "SENS_GYRO_XSCALE");
                activeUAS->requestParameter(0, "SENS_GYRO_YSCALE");
                activeUAS->requestParameter(0, "SENS_GYRO_ZSCALE");
                activeUAS->requestParameter(0, "SENS_BOARD_ROT");
            }
            if (text.startsWith("mag ")) {
                activeUAS->requestParameter(0, "SENS_MAG_XOFF");
                activeUAS->requestParameter(0, "SENS_MAG_YOFF");
                activeUAS->requestParameter(0, "SENS_MAG_ZOFF");
                activeUAS->requestParameter(0, "SENS_MAG_XSCALE");
                activeUAS->requestParameter(0, "SENS_MAG_YSCALE");
                activeUAS->requestParameter(0, "SENS_MAG_ZSCALE");
                activeUAS->requestParameter(0, "SENS_EXT_MAG_ROT");
            }

            if (text.startsWith("dpress ")) {
                activeUAS->requestParameter(0, "SENS_DPRES_OFF");
                activeUAS->requestParameter(0, "SENS_DPRES_ANA");
            }
        }
    }

    if (text.endsWith(" calibration: started")) {
        setInstructionImage(":/files/images/px4/calibration/accel_down.png");
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
    setInstructionImage(":/files/images/px4/calibration/accel_down.png");
    activeUAS->executeCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    ui->progressBar->setValue(0);
}

void QGCPX4SensorCalibration::magButtonClicked()
{
    setInstructionImage(":/files/images/px4/calibration/accel_down.png");
    activeUAS->executeCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    ui->progressBar->setValue(0);
}

void QGCPX4SensorCalibration::accelButtonClicked()
{
    setInstructionImage(":/files/images/px4/calibration/accel_down.png");
    activeUAS->executeCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0);
    ui->progressBar->setValue(0);
}

void QGCPX4SensorCalibration::diffPressureButtonClicked()
{
    setInstructionImage(":/files/images/px4/calibration/accel_down.png");
    activeUAS->executeCommand(MAV_CMD_PREFLIGHT_CALIBRATION, 1, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0);
    ui->progressBar->setValue(0);
}

void QGCPX4SensorCalibration::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.addAction(clearAction);
    menu.exec(event->globalPos());
}
