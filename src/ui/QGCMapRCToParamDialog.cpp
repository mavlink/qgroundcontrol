/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/// @file
///     @author Thomas Gubler <thomasgubler@gmail.com>

#include "QGCMapRCToParamDialog.h"
#include "ui_QGCMapRCToParamDialog.h"

#include <QDebug>
#include <QTimer>
#include <QEventLoop>
#include <QShowEvent>
#include <QPushButton>

QGCMapRCToParamDialog::QGCMapRCToParamDialog(QString param_id, UASInterface *mav, MultiVehicleManager* multiVehicleManager, QWidget *parent)
    : QDialog(parent)
    , param_id(param_id)
    , mav(mav)
    , _multiVehicleManager(multiVehicleManager)
    , ui(new Ui::QGCMapRCToParamDialog)
{
    ui->setupUi(this);
    
    // only enable ok button when param was refreshed
    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(false);

    ui->paramIdLabel->setText(param_id);

    // refresh the parameter from onboard to make sure the current value is used
    AutoPilotPlugin* autopilot = _multiVehicleManager->getVehicleById(mav->getUASID())->autopilotPlugin();
    Q_ASSERT(autopilot);
    connect(autopilot->getParameterFact(FactSystem::defaultComponentId, param_id), &Fact::valueChanged, this, &QGCMapRCToParamDialog::_parameterUpdated);
    autopilot->refreshParameter(FactSystem::defaultComponentId, param_id);
}

QGCMapRCToParamDialog::~QGCMapRCToParamDialog()
{
    delete ui;
}

void QGCMapRCToParamDialog::accept() {
    emit mapRCToParamDialogResult(param_id,
            (float)ui->scaleDoubleSpinBox->value(),
            (float)ui->value0DoubleSpinBox->value(),
            (quint8)ui->rcParamChannelComboBox->currentIndex(),
            (float)ui->minValueDoubleSpinBox->value(),
            (float)ui->maxValueDoubleSpinBox->value());

    QDialog::accept();
}

void QGCMapRCToParamDialog::_parameterUpdated(QVariant value)
{
    Q_UNUSED(value);
    
    ui->infoLabel->setText("Parameter value is up to date");
    ui->value0DoubleSpinBox->setValue(value.toDouble());
    ui->value0DoubleSpinBox->setEnabled(true);
    
    connect(this, &QGCMapRCToParamDialog::mapRCToParamDialogResult, mav, &UASInterface::sendMapRCToParam);
    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(true);
}
