/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Implementation of QGCSensorSettingsWidget
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include "QGCSensorSettingsWidget.h"
#include "ui_QGCSensorSettingsWidget.h"

QGCSensorSettingsWidget::QGCSensorSettingsWidget(UASInterface* uas, QWidget *parent) :
    QWidget(parent),
    mav(uas),
    ui(new Ui::QGCSensorSettingsWidget)
{
    ui->setupUi(this);
    // Set up delay timers
     delayedSendRawSensorTimer.setInterval(800);
     delayedSendControllerTimer.setInterval(800);
     delayedSendExtendedTimer.setInterval(800);
     delayedSendRCTimer.setInterval(800);
     delayedSendPositionTimer.setInterval(800);
     delayedSendExtra1Timer.setInterval(800);
     delayedSendExtra2Timer.setInterval(800);
     delayedSendExtra3Timer.setInterval(800);

     connect(&delayedSendRawSensorTimer, SIGNAL(timeout()), this, SLOT(sendRawSensor()));
     connect(&delayedSendControllerTimer, SIGNAL(timeout()), this, SLOT(sendController()));
     connect(&delayedSendExtendedTimer, SIGNAL(timeout()), this, SLOT(sendExtended()));
     connect(&delayedSendRCTimer, SIGNAL(timeout()), this, SLOT(sendRC()));
     connect(&delayedSendPositionTimer, SIGNAL(timeout()), this, SLOT(sendPosition()));
     connect(&delayedSendExtra1Timer, SIGNAL(timeout()), this, SLOT(sendExtra1()));
     connect(&delayedSendExtra2Timer, SIGNAL(timeout()), this, SLOT(sendExtra2()));
    connect(&delayedSendExtra3Timer, SIGNAL(timeout()), this, SLOT(sendExtra3()));

    // Connect UI
    connect(ui->spinBox_rawSensor, SIGNAL(valueChanged(int)), this, SLOT(delayedSendRawSensor(int)));//mav, SLOT(enableRawSensorDataTransmission(int)));
    connect(ui->spinBox_controller, SIGNAL(valueChanged(int)), this, SLOT(delayedSendController(int)));
    connect(ui->spinBox_extended, SIGNAL(valueChanged(int)), this, SLOT(delayedSendExtended(int)));
    connect(ui->spinBox_rc, SIGNAL(valueChanged(int)), this, SLOT(delayedSendRC(int)));
    connect(ui->spinBox_position, SIGNAL(valueChanged(int)), this, SLOT(delayedSendPosition(int)));
    connect(ui->spinBox_extra1, SIGNAL(valueChanged(int)), this, SLOT(delayedSendExtra1(int)));
    connect(ui->spinBox_extra2, SIGNAL(valueChanged(int)), this, SLOT(delayedSendExtra2(int)));
    connect(ui->spinBox_extra3, SIGNAL(valueChanged(int)), this, SLOT(delayedSendExtra3(int)));

    // Calibration
    connect(ui->rcCalButton, SIGNAL(clicked()), mav, SLOT(startRadioControlCalibration()));
    connect(ui->magCalButton, SIGNAL(clicked()), mav, SLOT(startMagnetometerCalibration()));
    connect(ui->pressureCalButton, SIGNAL(clicked()), mav, SLOT(startPressureCalibration()));
    connect(ui->gyroCalButton, SIGNAL(clicked()), mav, SLOT(startGyroscopeCalibration()));

    // Hide the calibration stuff - done in custom widgets anyway
    ui->groupBox_3->hide();
}

void QGCSensorSettingsWidget::delayedSendRawSensor(int rate)
{
    Q_UNUSED(rate);
    delayedSendRawSensorTimer.start();
}

void QGCSensorSettingsWidget::delayedSendController(int rate)
{
    Q_UNUSED(rate);
    delayedSendControllerTimer.start();
}

void QGCSensorSettingsWidget::delayedSendExtended(int rate)
{
    Q_UNUSED(rate);
    delayedSendExtendedTimer.start();
}

void QGCSensorSettingsWidget::delayedSendRC(int rate)
{
    Q_UNUSED(rate);
    delayedSendRCTimer.start();
}

void QGCSensorSettingsWidget::delayedSendPosition(int rate)
{
    Q_UNUSED(rate);
    delayedSendPositionTimer.start();
}

void QGCSensorSettingsWidget::delayedSendExtra1(int rate)
{
    Q_UNUSED(rate);
    delayedSendExtra1Timer.start();
}

void QGCSensorSettingsWidget::delayedSendExtra2(int rate)
{
    Q_UNUSED(rate);
    delayedSendExtra2Timer.start();
}

void QGCSensorSettingsWidget::delayedSendExtra3(int rate)
{
    Q_UNUSED(rate);
    delayedSendExtra3Timer.start();
}

void QGCSensorSettingsWidget::sendRawSensor()
{
    delayedSendRawSensorTimer.stop();
    mav->enableRawSensorDataTransmission(ui->spinBox_rawSensor->value());
}

void QGCSensorSettingsWidget::sendController()
{
    delayedSendControllerTimer.stop();
    mav->enableRawControllerDataTransmission(ui->spinBox_controller->value());
}

void QGCSensorSettingsWidget::sendExtended()
{
    delayedSendExtendedTimer.stop();
    mav->enableExtendedSystemStatusTransmission(ui->spinBox_extended->value());
}

void QGCSensorSettingsWidget::sendRC()
{
    delayedSendRCTimer.stop();
    mav->enableRCChannelDataTransmission(ui->spinBox_rc->value());
}

void QGCSensorSettingsWidget::sendPosition()
{
    delayedSendPositionTimer.stop();
    mav->enablePositionTransmission(ui->spinBox_position->value());
}

void QGCSensorSettingsWidget::sendExtra1()
{
    delayedSendExtra1Timer.stop();
    mav->enableExtra1Transmission(ui->spinBox_extra1->value());
}

void QGCSensorSettingsWidget::sendExtra2()
{
    delayedSendExtra2Timer.stop();
    mav->enableExtra2Transmission(ui->spinBox_extra2->value());
}

void QGCSensorSettingsWidget::sendExtra3()
{
    delayedSendExtra3Timer.stop();
    mav->enableExtra3Transmission(ui->spinBox_extra3->value());
}

QGCSensorSettingsWidget::~QGCSensorSettingsWidget()
{
    delete ui;
}

void QGCSensorSettingsWidget::changeEvent(QEvent *e)
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
