/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Thomas Gubler <thomasgubler@gmail.com>

#include "QGCMapRCToParamDialog.h"
#include "ui_QGCMapRCToParamDialog.h"
#include "ParameterManager.h"

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

    // refresh the parameter from onboard to make sure the current value is used
    Vehicle* vehicle = _multiVehicleManager->getVehicleById(mav->getUASID());
    Fact* paramFact = vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, param_id);
    
    ui->minValueDoubleSpinBox->setValue(paramFact->rawMin().toDouble());
    ui->maxValueDoubleSpinBox->setValue(paramFact->rawMax().toDouble());

    // only enable ok button when param was refreshed
    QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(false);

    ui->paramIdLabel->setText(param_id);

    connect(paramFact, &Fact::vehicleUpdated, this, &QGCMapRCToParamDialog::_parameterUpdated);
    vehicle->parameterManager()->refreshParameter(FactSystem::defaultComponentId, param_id);
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
