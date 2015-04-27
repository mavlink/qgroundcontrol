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
#include "AutoPilotPluginManager.h"

#include <QDebug>
#include <QTimer>
#include <QEventLoop>
#include <QShowEvent>
#include <QPushButton>

QGCMapRCToParamDialog::QGCMapRCToParamDialog(QString param_id,
        UASInterface *mav, QWidget *parent) :
    QDialog(parent),
    param_id(param_id),
    mav(mav),
    ui(new Ui::QGCMapRCToParamDialog)
{
    ui->setupUi(this);
    


    // only enable ok button when param was refreshed
    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(false);

    ui->paramIdLabel->setText(param_id);

    // Refresh the param
    ParamLoader *paramLoader = new ParamLoader(param_id, mav);
    paramLoader->moveToThread(&paramLoadThread);
    connect(&paramLoadThread, &QThread::finished, paramLoader, &QObject::deleteLater);
    connect(this, &QGCMapRCToParamDialog::refreshParam, paramLoader, &ParamLoader::load);
    connect(paramLoader, &ParamLoader::paramLoaded, this, &QGCMapRCToParamDialog::paramLoaded);
    paramLoadThread.start();
    emit refreshParam();
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

void QGCMapRCToParamDialog::paramLoaded(bool success, float value, QString message)
{
    paramLoadThread.quit();
    if (success) {
        ui->infoLabel->setText("Parameter value is up to date");
        ui->value0DoubleSpinBox->setValue(value);
        ui->value0DoubleSpinBox->setEnabled(true);

        connect(this, &QGCMapRCToParamDialog::mapRCToParamDialogResult,
                mav, &UASInterface::sendMapRCToParam);
        QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
        okButton->setEnabled(true);
    } else {
        qDebug() << "Error while reading param" << param_id;
        ui->infoLabel->setText("Error while refreshing param (" + message + ")");
    }
}

ParamLoader::ParamLoader(QString paramName, UASInterface* uas, QObject* parent) :
    QObject(parent),
    _uas(uas),
    _paramName(paramName),
    _paramReceived(false)
{
    _autopilot = AutoPilotPluginManager::instance()->getInstanceForAutoPilotPlugin(_uas);
    Q_ASSERT(_autopilot);
}

void ParamLoader::load()
{
    connect(_autopilot->getParameterFact(_paramName), &Fact::valueChanged, this, &ParamLoader::_parameterUpdated);
    
    // refresh the parameter from onboard to make sure the current value is used
    _autopilot->refreshParameter(FactSystem::defaultComponentId, _paramName);
}

void ParamLoader::_parameterUpdated(QVariant value)
{
    Q_UNUSED(value);
    
    emit paramLoaded(true, _autopilot->getParameterFact(_paramName)->value().toFloat(), "");
}
