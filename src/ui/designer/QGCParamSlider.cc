#include <QMenu>
#include <QContextMenuEvent>
#include <QSettings>

#include "QGCParamSlider.h"
#include "ui_QGCParamSlider.h"
#include "UASInterface.h"


QGCParamSlider::QGCParamSlider(QWidget *parent) :
    QGCToolWidgetItem("Slider", parent),
    parameterName(""),
    parameterValue(0.0f),
    parameterScalingFactor(0.0),
    parameterMin(0.0f),
    parameterMax(0.0f),
    component(0),
    ui(new Ui::QGCParamSlider)
{
    ui->setupUi(this);
    endEditMode();
    connect(ui->doneButton, SIGNAL(clicked()), this, SLOT(endEditMode()));
}

QGCParamSlider::~QGCParamSlider()
{
    delete ui;
}

void QGCParamSlider::startEditMode()
{
    ui->doneButton->show();
    ui->maxLabel->show();
    ui->minLabel->show();
    ui->nameLineEdit->show();
    ui->instructionsLabel->show();
    ui->refreshParamsButton->show();
    ui->selectParamComboBox->show();
    ui->minSpinBox->show();
    ui->maxSpinBox->show();
    ui->typeComboBox->show();
    isInEditMode = true;
}

void QGCParamSlider::endEditMode()
{
    ui->doneButton->hide();
    ui->maxLabel->hide();
    ui->minLabel->hide();
    ui->nameLineEdit->hide();
    ui->instructionsLabel->hide();
    ui->refreshParamsButton->hide();
    ui->selectParamComboBox->hide();
    ui->minSpinBox->hide();
    ui->maxSpinBox->hide();
    ui->typeComboBox->hide();
    isInEditMode = false;
    emit editingFinished();
}

void QGCParamSlider::sendParameter()
{
    if (QGCToolWidgetItem::uas)
    {
        QGCToolWidgetItem::uas->setParameter(component, parameterName, parameterValue);
    }
    else
    {
        qDebug() << __FILE__ << __LINE__ << "NO UAS SET, DOING NOTHING";
    }
}

void QGCParamSlider::changeEvent(QEvent *e)
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

void QGCParamSlider::writeSettings(QSettings& settings)
{
  Q_UNUSED(settings);

}

void QGCParamSlider::readSettings(const QSettings& settings)
{
  Q_UNUSED(settings);
}
