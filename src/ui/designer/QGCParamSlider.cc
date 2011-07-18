#include <QMenu>
#include <QContextMenuEvent>
#include <QSettings>
#include <QTimer>

#include "QGCParamSlider.h"
#include "ui_QGCParamSlider.h"
#include "UASInterface.h"
#include "UASManager.h"


QGCParamSlider::QGCParamSlider(QWidget *parent) :
    QGCToolWidgetItem("Slider", parent),
    parameterName(""),
    parameterValue(0.0f),
    parameterScalingFactor(0.0),
    parameterMin(0.0f),
    parameterMax(0.0f),
    component(0),
    parameterIndex(-1),
    ui(new Ui::QGCParamSlider)
{
    ui->setupUi(this);
    uas = NULL;

    scaledInt = ui->valueSlider->maximum() - ui->valueSlider->minimum();

    ui->editDoneButton->hide();
    ui->editNameLabel->hide();
    ui->editRefreshParamsButton->hide();
    ui->editSelectParamComboBox->hide();
    ui->editSelectComponentComboBox->hide();
    ui->editStatusLabel->hide();
    ui->editMinSpinBox->hide();
    ui->editMaxSpinBox->hide();
    ui->editLine1->hide();
    ui->editLine2->hide();

    ui->editLine1->setStyleSheet("QWidget { border: 1px solid #66666B; border-radius: 3px; padding: 10px 0px 0px 0px; background: #111122; }");
    ui->editLine2->setStyleSheet("QWidget { border: 1px solid #66666B; border-radius: 3px; padding: 10px 0px 0px 0px; background: #111122; }");

    connect(ui->editDoneButton, SIGNAL(clicked()), this, SLOT(endEditMode()));

    // Sending actions
    connect(ui->writeButton, SIGNAL(clicked()), this, SLOT(sendParameter()));
    connect(ui->editSelectComponentComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectComponent(int)));
    connect(ui->editSelectParamComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectParameter(int)));
    connect(ui->valueSlider, SIGNAL(valueChanged(int)), this, SLOT(setSliderValue(int)));
    connect(ui->valueSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setParamValue(double)));
    connect(ui->editNameLabel, SIGNAL(textChanged(QString)), ui->nameLabel, SLOT(setText(QString)));
    connect(ui->readButton, SIGNAL(clicked()), this, SLOT(requestParameter()));
    connect(ui->editRefreshParamsButton, SIGNAL(clicked()), this, SLOT(refreshParamList()));

    // Set the current UAS if present
    setActiveUAS(UASManager::instance()->getActiveUAS());

    // Get param value
    QTimer::singleShot(1000, this, SLOT(requestParameter()));
}

QGCParamSlider::~QGCParamSlider()
{
    delete ui;
}

void QGCParamSlider::refreshParamList()
{
    ui->editSelectParamComboBox->setEnabled(true);
    ui->editSelectComponentComboBox->setEnabled(true);
    if (uas) {
        uas->getParamManager()->requestParameterList();
    }
}

void QGCParamSlider::setActiveUAS(UASInterface* activeUas)
{
    if (activeUas) {
        if (uas) {
            disconnect(uas, SIGNAL(parameterChanged(int,int,int,int,QString,float)), this, SLOT(setParameterValue(int,int,int,int,QString,float)));
        }

        // Connect buttons and signals
        connect(activeUas, SIGNAL(parameterChanged(int,int,int,int,QString,float)), this, SLOT(setParameterValue(int,int,int,int,QString,float)), Qt::UniqueConnection);
        uas = activeUas;
    }
}

void QGCParamSlider::requestParameter()
{
    if (parameterIndex != -1 && uas) {
        uas->requestParameter(this->component, this->parameterIndex);
    }
}

void QGCParamSlider::setParamValue(double value)
{
    parameterValue = value;
    ui->valueSlider->setValue(floatToScaledInt(value));
}

void QGCParamSlider::selectComponent(int componentIndex)
{
    this->component = ui->editSelectComponentComboBox->itemData(componentIndex).toInt();
}

void QGCParamSlider::selectParameter(int paramIndex)
{
    parameterName = ui->editSelectParamComboBox->itemText(paramIndex);
    parameterIndex = ui->editSelectParamComboBox->itemData(paramIndex).toInt();
}

void QGCParamSlider::startEditMode()
{
    ui->valueSlider->hide();
    ui->valueSpinBox->hide();
    ui->nameLabel->hide();
    ui->writeButton->hide();
    ui->readButton->hide();

    ui->editDoneButton->show();
    ui->editNameLabel->show();
    ui->editRefreshParamsButton->show();
    ui->editSelectParamComboBox->show();
    ui->editSelectComponentComboBox->show();
    ui->editStatusLabel->show();
    ui->editMinSpinBox->show();
    ui->editMaxSpinBox->show();
    ui->writeButton->hide();
    ui->readButton->hide();
    ui->editLine1->show();
    ui->editLine2->show();
    isInEditMode = true;
}

void QGCParamSlider::endEditMode()
{
    // Store component id
    selectComponent(ui->editSelectComponentComboBox->currentIndex());

    // Store parameter name and id
    selectParameter(ui->editSelectParamComboBox->currentIndex());

    // Min/max
    parameterMin = ui->editMinSpinBox->value();
    parameterMax = ui->editMaxSpinBox->value();

    ui->editDoneButton->hide();
    ui->editNameLabel->hide();
    ui->editRefreshParamsButton->hide();
    ui->editSelectParamComboBox->hide();
    ui->editSelectComponentComboBox->hide();
    ui->editStatusLabel->hide();
    ui->editMinSpinBox->hide();
    ui->editMaxSpinBox->hide();
    ui->editLine1->hide();
    ui->editLine2->hide();
    ui->writeButton->show();
    ui->readButton->show();
    ui->valueSlider->show();
    ui->valueSpinBox->show();
    ui->nameLabel->show();
    isInEditMode = false;
    emit editingFinished();
}

void QGCParamSlider::sendParameter()
{
    if (uas) {
        // Set value, param manager handles retransmission
        uas->getParamManager()->setParameter(component, parameterName, parameterValue);
    } else {
        qDebug() << __FILE__ << __LINE__ << "NO UAS SET, DOING NOTHING";
    }
}

void QGCParamSlider::setSliderValue(int sliderValue)
{
    parameterValue = scaledIntToFloat(sliderValue);
    ui->valueSpinBox->setValue(parameterValue);
//    QString unit("");
//    ui->valueLabel->setText(QString("%1 %2").arg(parameterValue, 6, 'f', 6, ' ').arg(unit));
}

/**
 * @brief uas Unmanned system sending the parameter
 * @brief component UAS component sending the parameter
 * @brief parameterName Key/name of the parameter
 * @brief value Value of the parameter
 */
void QGCParamSlider::setParameterValue(int uas, int component, int paramCount, int paramIndex, QString parameterName, float value)
{
    Q_UNUSED(paramCount);
    // Check if this component and parameter are part of the list
    bool found = false;
    for (int i = 0; i< ui->editSelectComponentComboBox->count(); ++i) {
        if (component == ui->editSelectComponentComboBox->itemData(i).toInt()) {
            found = true;
        }
    }

    if (!found) {
        ui->editSelectComponentComboBox->addItem(tr("Component #%1").arg(component), component);
    }

    // Parameter checking
    found = false;
    for (int i = 0; i < ui->editSelectParamComboBox->count(); ++i) {
        if (parameterName == ui->editSelectParamComboBox->itemText(i)) {
            found = true;
        }
    }

    if (!found) {
        ui->editSelectParamComboBox->addItem(parameterName, paramIndex);
    }

    Q_UNUSED(uas);
    if (component == this->component && parameterName == this->parameterName) {
        parameterValue = value;
        ui->valueSpinBox->setValue(value);
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
    float result = (((double)sliderValue)/(double)scaledInt)*(ui->editMaxSpinBox->value() - ui->editMinSpinBox->value());
    //qDebug() << "INT TO FLOAT: CONVERTED" << sliderValue << "TO" << result;
    return result;
}

int QGCParamSlider::floatToScaledInt(float value)
{
    int result = ((value - ui->editMinSpinBox->value())/(ui->editMaxSpinBox->value() - ui->editMinSpinBox->value()))*scaledInt;
    //qDebug() << "FLOAT TO INT: CONVERTED" << value << "TO" << result << "SCALEDINT" << scaledInt;
    return result;
}

void QGCParamSlider::writeSettings(QSettings& settings)
{
    settings.setValue("TYPE", "SLIDER");
    settings.setValue("QGC_PARAM_SLIDER_DESCRIPTION", ui->nameLabel->text());
    //settings.setValue("QGC_PARAM_SLIDER_BUTTONTEXT", ui->actionButton->text());
    settings.setValue("QGC_PARAM_SLIDER_PARAMID", parameterName);
    settings.setValue("QGC_PARAM_SLIDER_PARAMINDEX", parameterIndex);
    settings.setValue("QGC_PARAM_SLIDER_COMPONENTID", component);
    settings.setValue("QGC_PARAM_SLIDER_MIN", ui->editMinSpinBox->value());
    settings.setValue("QGC_PARAM_SLIDER_MAX", ui->editMaxSpinBox->value());
    settings.sync();
}

void QGCParamSlider::readSettings(const QSettings& settings)
{
    ui->nameLabel->setText(settings.value("QGC_PARAM_SLIDER_DESCRIPTION").toString());
    ui->editNameLabel->setText(settings.value("QGC_PARAM_SLIDER_DESCRIPTION").toString());
    //settings.setValue("QGC_PARAM_SLIDER_BUTTONTEXT", ui->actionButton->text());
    parameterIndex = settings.value("QGC_PARAM_SLIDER_PARAMINDEX", parameterIndex).toInt();
    ui->editSelectParamComboBox->addItem(settings.value("QGC_PARAM_SLIDER_PARAMID").toString(), parameterIndex);
    ui->editSelectComponentComboBox->addItem(tr("Component #%1").arg(settings.value("QGC_PARAM_SLIDER_COMPONENTID").toInt()), settings.value("QGC_PARAM_SLIDER_COMPONENTID").toInt());
    ui->editMinSpinBox->setValue(settings.value("QGC_PARAM_SLIDER_MIN").toFloat());
    ui->editMaxSpinBox->setValue(settings.value("QGC_PARAM_SLIDER_MAX").toFloat());
    ui->editSelectParamComboBox->setEnabled(true);
    ui->editSelectComponentComboBox->setEnabled(true);
}
