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

    scaledInt = ui->valueSlider->maximum() - ui->valueSlider->minimum();

    ui->editDoneButton->show();
    ui->editMaxLabel->show();
    ui->editMinLabel->show();
    ui->editNameLabel->show();
    ui->editInstructionsLabel->show();
    ui->editRefreshParamsButton->show();
    ui->editSelectParamComboBox->show();
    ui->editSelectComponentComboBox->show();
    ui->editStatusLabel->show();
    ui->editMinSpinBox->show();
    ui->editMaxSpinBox->show();
    connect(ui->editDoneButton, SIGNAL(clicked()), this, SLOT(endEditMode()));
}

QGCParamSlider::~QGCParamSlider()
{
    delete ui;
}

void QGCParamSlider::startEditMode()
{
    ui->editDoneButton->show();
    ui->editMaxLabel->show();
    ui->editMinLabel->show();
    ui->editNameLabel->show();
    ui->editInstructionsLabel->show();
    ui->editRefreshParamsButton->show();
    ui->editSelectParamComboBox->show();
    ui->editSelectComponentComboBox->show();
    ui->editStatusLabel->show();
    ui->editMinSpinBox->show();
    ui->editMaxSpinBox->show();
    isInEditMode = true;
}

void QGCParamSlider::endEditMode()
{
    ui->editDoneButton->hide();
    ui->editMaxLabel->hide();
    ui->editMinLabel->hide();
    ui->editNameLabel->hide();
    ui->editInstructionsLabel->hide();
    ui->editRefreshParamsButton->hide();
    ui->editSelectParamComboBox->hide();
    ui->editSelectComponentComboBox->hide();
    ui->editStatusLabel->hide();
    ui->editMinSpinBox->hide();
    ui->editMaxSpinBox->hide();
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

void QGCParamSlider::setSliderValue(int sliderValue)
{
    parameterValue = scaledIntToFloat(sliderValue);
    QString unit("");
    ui->valueLabel->setText(QString("%1 %2").arg(parameterValue, 0, 'f', 3).arg(unit));
}

/**
 * @brief uas Unmanned system sending the parameter
 * @brief component UAS component sending the parameter
 * @brief parameterName Key/name of the parameter
 * @brief value Value of the parameter
 */
void QGCParamSlider::setParameterValue(int uas, int component, QString parameterName, float value)
{
    Q_UNUSED(uas);
    if (component == this->component && parameterName == this->parameterName)
    {
        parameterValue = value;
        QString unit("");
        ui->valueLabel->setText(QString("%1 %2").arg(value, 0, 'f', 3).arg(unit));
        ui->valueSlider->setValue(floatToScaledInt(value));
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

float QGCParamSlider::scaledIntToFloat(int sliderValue)
{
    return (((double)sliderValue)/scaledInt)*(parameterMax - parameterMin);
}

int QGCParamSlider::floatToScaledInt(float value)
{
    return ((value - parameterMin)/(parameterMax - parameterMin))*scaledInt;
}

void QGCParamSlider::writeSettings(QSettings& settings)
{
    settings.setValue("TYPE", "SLIDER");
    settings.setValue("QGC_PARAM_SLIDER_DESCRIPTION", ui->nameLabel->text());
    //settings.setValue("QGC_PARAM_SLIDER_BUTTONTEXT", ui->actionButton->text());
    settings.setValue("QGC_PARAM_SLIDER_PARAMID", ui->editSelectParamComboBox->currentText());
    settings.setValue("QGC_PARAM_SLIDER_COMPONENTID", ui->editSelectComponentComboBox->currentText());
    settings.setValue("QGC_PARAM_SLIDER_MIN", ui->editMinSpinBox->value());
    settings.setValue("QGC_PARAM_SLIDER_MAX", ui->editMaxSpinBox->value());
    settings.sync();
}

void QGCParamSlider::readSettings(const QSettings& settings)
{
    ui->nameLabel->setText(settings.value("QGC_PARAM_SLIDER_DESCRIPTION").toString());
    //settings.setValue("QGC_PARAM_SLIDER_BUTTONTEXT", ui->actionButton->text());
    ui->editSelectParamComboBox->setEditText(settings.value("QGC_PARAM_SLIDER_PARAMID").toString());
    ui->editSelectComponentComboBox->setEditText(settings.value("QGC_PARAM_SLIDER_COMPONENTID").toString());
    ui->editMinSpinBox->setValue(settings.value("QGC_PARAM_SLIDER_MIN").toFloat());
    ui->editMaxSpinBox->setValue(settings.value("QGC_PARAM_SLIDER_MAX").toFloat());
    qDebug() << "DONE READING SETTINGS";
}
