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
    // XXX: This might be a bad idea sending a message every time the value changes
    connect(ui->spinBox_rawSensor, SIGNAL(valueChanged(int)), mav, SLOT(enableRawSensorDataTransmission(int)));
    connect(ui->spinBox_controller, SIGNAL(valueChanged(int)), mav, SLOT(enableRawControllerDataTransmission(int)));
    connect(ui->spinBox_extended, SIGNAL(valueChanged(int)), mav, SLOT(enableExtendedSystemStatusTransmission(int)));
    connect(ui->spinBox_rc, SIGNAL(valueChanged(int)), mav, SLOT(enableRCChannelDataTransmission(int)));
    connect(ui->spinBox_position, SIGNAL(valueChanged(int)), mav, SLOT(enablePositionTransmission(int)));
    connect(ui->spinBox_extra1, SIGNAL(valueChanged(int)), mav, SLOT(enableExtra1Transmission(int)));
    connect(ui->spinBox_extra2, SIGNAL(valueChanged(int)), mav, SLOT(enableExtra2Transmission(int)));
    connect(ui->spinBox_extra3, SIGNAL(valueChanged(int)), mav, SLOT(enableExtra3Transmission(int)));

    // Calibration
    connect(ui->rcCalButton, SIGNAL(clicked()), mav, SLOT(startRadioControlCalibration()));
    connect(ui->magCalButton, SIGNAL(clicked()), mav, SLOT(startMagnetometerCalibration()));
    connect(ui->pressureCalButton, SIGNAL(clicked()), mav, SLOT(startPressureCalibration()));
    connect(ui->gyroCalButton, SIGNAL(clicked()), mav, SLOT(startGyroscopeCalibration()));
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
