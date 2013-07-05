#include <QMenu>
#include <QContextMenuEvent>
#include <QSettings>
#include <QTimer>
#include <QToolTip>

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
    ui(new Ui::QGCParamSlider)
{
    valueModLock = false;
    visibleEnabled = true;
    valueModLockParam = false;
    ui->setupUi(this);
    ui->intValueSpinBox->hide();
    ui->valueSlider->setEnabled(false);
    ui->doubleValueSpinBox->setEnabled(false);
    uas = NULL;

    scaledInt = ui->valueSlider->maximum() - ui->valueSlider->minimum();

    ui->editInfoCheckBox->hide();
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
    ui->infoLabel->hide();

    connect(ui->editDoneButton, SIGNAL(clicked()), this, SLOT(endEditMode()));

    // Sending actions
    connect(ui->writeButton, SIGNAL(clicked()), this, SLOT(sendParameter()));
    connect(ui->editSelectComponentComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectComponent(int)));
    connect(ui->editSelectParamComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectParameter(int)));
    connect(ui->valueSlider, SIGNAL(valueChanged(int)), this, SLOT(setSliderValue(int)));
    connect(ui->doubleValueSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setParamValue(double)));
    connect(ui->intValueSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setParamValue(int)));
    connect(ui->editNameLabel, SIGNAL(textChanged(QString)), ui->nameLabel, SLOT(setText(QString)));
    connect(ui->readButton, SIGNAL(clicked()), this, SLOT(requestParameter()));
    connect(ui->editRefreshParamsButton, SIGNAL(clicked()), this, SLOT(refreshParamList()));
    connect(ui->editInfoCheckBox, SIGNAL(clicked(bool)), this, SLOT(showInfo(bool)));
    // connect to self
    connect(ui->infoLabel, SIGNAL(released()), this, SLOT(showTooltip()));
    // Set the current UAS if present
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
}

QGCParamSlider::~QGCParamSlider()
{
    delete ui;
}

void QGCParamSlider::showTooltip()
{
    QWidget* sender = dynamic_cast<QWidget*>(QObject::sender());

    if (sender)
    {
        QPoint point = mapToGlobal(ui->infoLabel->pos());
        QToolTip::showText(point, sender->toolTip());
    }
}

void QGCParamSlider::refreshParamList()
{
    ui->editSelectParamComboBox->setEnabled(true);
    ui->editSelectComponentComboBox->setEnabled(true);
    if (uas)
    {
        uas->getParamManager()->requestParameterList();
        ui->editStatusLabel->setText(tr("Parameter list updating.."));
    }
}

void QGCParamSlider::setActiveUAS(UASInterface* activeUas)
{
    if (activeUas)
    {
        if (uas)
        {
            disconnect(uas, SIGNAL(parameterChanged(int,int,int,int,QString,QVariant)), this, SLOT(setParameterValue(int,int,int,int,QString,QVariant)));
        }

        // Connect buttons and signals
        connect(activeUas, SIGNAL(parameterChanged(int,int,int,int,QString,QVariant)), this, SLOT(setParameterValue(int,int,int,int,QString,QVariant)), Qt::UniqueConnection);
        uas = activeUas;
        // Update current param value
        //requestParameter();
        // Set param info
        QString text = uas->getParamManager()->getParamInfo(parameterName);
        if (text != "")
        {
            ui->infoLabel->setToolTip(text);
            ui->infoLabel->show();
        }
        // Force-uncheck and hide label if no description is available
        if (ui->editInfoCheckBox->isChecked())
        {
            showInfo((text.length() > 0));
        }
    }
}

void QGCParamSlider::requestParameter()
{
    if (!parameterName.isEmpty() && uas)
    {
        uas->getParamManager()->requestParameterUpdate(this->component, this->parameterName);
    }
}

void QGCParamSlider::showInfo(bool enable)
{
    ui->editInfoCheckBox->setChecked(enable);
    ui->infoLabel->setVisible(enable);
}

void QGCParamSlider::setParamValue(double value)
{
    parameterValue = (float)value;
     //disconnect(ui->valueSlider,SIGNAL(valueChanged(int)));
    if (!valueModLock && !valueModLockParam)
    {
        valueModLock = true;
        ui->valueSlider->setValue(floatToScaledInt(value));
    }
    else
    {
        valueModLock = false;
    }
    //connect(ui->valueSlider, SIGNAL(valueChanged(int)), this, SLOT(setSliderValue(int)));
}

void QGCParamSlider::setParamValue(int value)
{
    parameterValue = value;
    // disconnect(ui->valueSlider,SIGNAL(valueChanged(int)));
    if (!valueModLock && !valueModLockParam)
    {
        valueModLock = true;
        ui->valueSlider->setValue(floatToScaledInt(value));
    }
    else
    {
        valueModLock = false;
    }
    //connect(ui->valueSlider, SIGNAL(valueChanged(int)), this, SLOT(setSliderValue(int)));
}

void QGCParamSlider::selectComponent(int componentIndex)
{
    this->component = ui->editSelectComponentComboBox->itemData(componentIndex).toInt();
}

void QGCParamSlider::selectParameter(int paramIndex)
{
    // Set name
    parameterName = ui->editSelectParamComboBox->itemText(paramIndex);

    // Update min and max values if available
    if (uas)
    {
        if (uas->getParamManager())
        {
            // Current value
            //uas->getParamManager()->requestParameterUpdate(component, parameterName);

            // Minimum
            if (uas->getParamManager()->isParamMinKnown(parameterName))
            {
                parameterMin = uas->getParamManager()->getParamMin(parameterName);
                ui->editMinSpinBox->setValue(parameterMin);
            }

            // Maximum
            if (uas->getParamManager()->isParamMaxKnown(parameterName))
            {
                parameterMax = uas->getParamManager()->getParamMax(parameterName);
                ui->editMaxSpinBox->setValue(parameterMax);
            }

            // Description
            //QString text = uas->getParamManager()->getParamInfo(parameterName);
            //ui->infoLabel->setText(text);

            //showInfo(!(text.length() > 0));
        }
    }
}

void QGCParamSlider::startEditMode()
{
    ui->valueSlider->hide();
    ui->doubleValueSpinBox->hide();
    ui->intValueSpinBox->hide();
    ui->nameLabel->hide();
    ui->writeButton->hide();
    ui->readButton->hide();

    ui->editInfoCheckBox->show();
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

    ui->editInfoCheckBox->hide();
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
    switch (parameterValue.type())
    {
    case QVariant::Char:
    case QVariant::Int:
    case QVariant::UInt:
        ui->intValueSpinBox->show();
        break;
    case QMetaType::Float:
        ui->doubleValueSpinBox->show();
        break;
    default:
        qCritical() << "ERROR: NO VALID PARAM TYPE";
        return;
    }
    ui->nameLabel->show();
    isInEditMode = false;
    emit editingFinished();
}

void QGCParamSlider::sendParameter()
{
    if (uas)
    {
        // Set value, param manager handles retransmission
        if (uas->getParamManager())
        {
            uas->getParamManager()->setParameter(component, parameterName, parameterValue);
        }
        else
        {
            qDebug() << "UAS HAS NO PARAM MANAGER, DOING NOTHING";
        }
    }
    else
    {
        qDebug() << __FILE__ << __LINE__ << "NO UAS SET, DOING NOTHING";
    }
}

void QGCParamSlider::setSliderValue(int sliderValue)
{
    if (!valueModLock && !valueModLockParam)
    {
        valueModLock = true;
        switch (parameterValue.type())
        {
        case QVariant::Char:
            parameterValue = QVariant(QChar((unsigned char)scaledIntToFloat(sliderValue)));
            ui->intValueSpinBox->setValue(parameterValue.toInt());
            break;
        case QVariant::Int:
            parameterValue = (int)scaledIntToFloat(sliderValue);
            ui->intValueSpinBox->setValue(parameterValue.toInt());
            break;
        case QVariant::UInt:
            parameterValue = (unsigned int)scaledIntToFloat(sliderValue);
            ui->intValueSpinBox->setValue(parameterValue.toUInt());
            break;
        case QMetaType::Float:
            parameterValue = scaledIntToFloat(sliderValue);
            ui->doubleValueSpinBox->setValue(parameterValue.toFloat());
            break;
        default:
            qCritical() << "ERROR: NO VALID PARAM TYPE";
            valueModLock = false;
            return;
        }
    }
    else
    {
        valueModLock = false;
    }
}

/**
 * @brief uas Unmanned system sending the parameter
 * @brief component UAS component sending the parameter
 * @brief parameterName Key/name of the parameter
 * @brief value Value of the parameter
 */
void QGCParamSlider::setParameterValue(int uas, int component, int paramCount, int paramIndex, QString parameterName, QVariant value)
{
    Q_UNUSED(paramCount);
    if (ui->nameLabel->text() == "Name")
    {
        ui->nameLabel->setText(parameterName);
    }
    // Check if this component and parameter are part of the list
    bool found = false;
    for (int i = 0; i< ui->editSelectComponentComboBox->count(); ++i)
    {
        if (component == ui->editSelectComponentComboBox->itemData(i).toInt())
        {
            found = true;
        }
    }

    if (!found)
    {
        ui->editSelectComponentComboBox->addItem(tr("Component #%1").arg(component), component);
    }

    // Parameter checking
    found = false;
    for (int i = 0; i < ui->editSelectParamComboBox->count(); ++i)
    {
        if (parameterName == ui->editSelectParamComboBox->itemText(i))
        {
            found = true;
        }
    }

    if (!found)
    {
        ui->editSelectParamComboBox->addItem(parameterName, paramIndex);
    }

    if (visibleParam != "")
    {
        if (parameterName == visibleParam)
        {
            if (visibleVal == value.toInt())
            {
                this->uas->requestParameter(this->component,this->parameterName);
                visibleEnabled = true;
                this->show();
            }
            else
            {
                //Disable the component here.
                ui->valueSlider->setEnabled(false);
                ui->intValueSpinBox->setEnabled(false);
                ui->doubleValueSpinBox->setEnabled(false);
                visibleEnabled = false;
                this->hide();
            }
        }
    }
    Q_UNUSED(uas);
    if (component == this->component && parameterName == this->parameterName)
    {
        if (!visibleEnabled)
        {
            return;
        }
        parameterValue = value;
        ui->valueSlider->setEnabled(true);
        valueModLockParam = true;
        switch (value.type())
        {
        case QVariant::Char:
            ui->intValueSpinBox->show();
            ui->intValueSpinBox->setEnabled(true);
            ui->doubleValueSpinBox->hide();
            ui->intValueSpinBox->setValue(value.toUInt());
            ui->intValueSpinBox->setRange(0, UINT8_MAX);
            if (parameterMax == 0 && parameterMin == 0)
            {
                ui->editMaxSpinBox->setValue(UINT8_MAX);
                ui->editMinSpinBox->setValue(0);
            }
            ui->valueSlider->setValue(floatToScaledInt(value.toUInt()));
            break;
        case QVariant::Int:
            ui->intValueSpinBox->show();
            ui->intValueSpinBox->setEnabled(true);
            ui->doubleValueSpinBox->hide();
            ui->intValueSpinBox->setValue(value.toInt());
            ui->intValueSpinBox->setRange(INT32_MIN, INT32_MAX);
            if (parameterMax == 0 && parameterMin == 0)
            {
                ui->editMaxSpinBox->setValue(INT32_MAX);
                ui->editMinSpinBox->setValue(INT32_MIN);
            }
            ui->valueSlider->setValue(floatToScaledInt(value.toInt()));
            break;
        case QVariant::UInt:
            ui->intValueSpinBox->show();
            ui->intValueSpinBox->setEnabled(true);
            ui->doubleValueSpinBox->hide();
            ui->intValueSpinBox->setValue(value.toUInt());
            ui->intValueSpinBox->setRange(0, UINT32_MAX);
            if (parameterMax == 0 && parameterMin == 0)
            {
                ui->editMaxSpinBox->setValue(UINT32_MAX);
                ui->editMinSpinBox->setValue(0);
            }
            ui->valueSlider->setValue(floatToScaledInt(value.toUInt()));
            break;
        case QMetaType::Float:
            ui->doubleValueSpinBox->setValue(value.toFloat());
            ui->doubleValueSpinBox->show();
            ui->doubleValueSpinBox->setEnabled(true);
            ui->intValueSpinBox->hide();
            if (parameterMax == 0 && parameterMin == 0)
            {
                ui->editMaxSpinBox->setValue(10000);
                ui->editMinSpinBox->setValue(0);
            }
            ui->valueSlider->setValue(floatToScaledInt(value.toFloat()));
            break;
        default:
            qCritical() << "ERROR: NO VALID PARAM TYPE";
            valueModLockParam = false;
            return;
        }
        valueModLockParam = false;
        parameterMax = ui->editMaxSpinBox->value();
        parameterMin = ui->editMinSpinBox->value();
    }

    if (paramIndex == paramCount - 1)
    {
        ui->editStatusLabel->setText(tr("Complete parameter list received."));
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
    settings.setValue("QGC_PARAM_SLIDER_COMPONENTID", component);
    settings.setValue("QGC_PARAM_SLIDER_MIN", ui->editMinSpinBox->value());
    settings.setValue("QGC_PARAM_SLIDER_MAX", ui->editMaxSpinBox->value());
    settings.setValue("QGC_PARAM_SLIDER_DISPLAY_INFO", ui->editInfoCheckBox->isChecked());
    settings.sync();
}
void QGCParamSlider::readSettings(const QString& pre,const QVariantMap& settings)
{
    parameterName = settings.value(pre + "QGC_PARAM_SLIDER_PARAMID").toString();
    component = settings.value(pre + "QGC_PARAM_SLIDER_COMPONENTID").toInt();
    ui->nameLabel->setText(settings.value(pre + "QGC_PARAM_SLIDER_DESCRIPTION").toString());
    ui->editNameLabel->setText(settings.value(pre + "QGC_PARAM_SLIDER_DESCRIPTION").toString());
    //settings.setValue("QGC_PARAM_SLIDER_BUTTONTEXT", ui->actionButton->text());
    ui->editSelectParamComboBox->addItem(settings.value(pre + "QGC_PARAM_SLIDER_PARAMID").toString());
    ui->editSelectParamComboBox->setCurrentIndex(ui->editSelectParamComboBox->count()-1);
    ui->editSelectComponentComboBox->addItem(tr("Component #%1").arg(settings.value(pre + "QGC_PARAM_SLIDER_COMPONENTID").toInt()), settings.value(pre + "QGC_PARAM_SLIDER_COMPONENTID").toInt());
    ui->editMinSpinBox->setValue(settings.value(pre + "QGC_PARAM_SLIDER_MIN").toFloat());
    ui->editMaxSpinBox->setValue(settings.value(pre + "QGC_PARAM_SLIDER_MAX").toFloat());
    visibleParam = settings.value(pre+"QGC_PARAM_SLIDER_VISIBLE_PARAM","").toString();
    visibleVal = settings.value(pre+"QGC_PARAM_SLIDER_VISIBLE_VAL",0).toInt();
    parameterMax = ui->editMaxSpinBox->value();
    parameterMin = ui->editMinSpinBox->value();
    //ui->valueSlider->setMaximum(parameterMax);
    //ui->valueSlider->setMinimum(parameterMin);
    showInfo(settings.value(pre + "QGC_PARAM_SLIDER_DISPLAY_INFO", true).toBool());
    ui->editSelectParamComboBox->setEnabled(true);
    ui->editSelectComponentComboBox->setEnabled(true);

    setActiveUAS(UASManager::instance()->getActiveUAS());

    // Get param value after settings have been loaded
    //requestParameter();
}

void QGCParamSlider::readSettings(const QSettings& settings)
{
    QVariantMap map;
    foreach (QString key,settings.allKeys())
    {
        map[key] = settings.value(key);
    }

    readSettings("",map);
    return;
    parameterName = settings.value("QGC_PARAM_SLIDER_PARAMID").toString();
    component = settings.value("QGC_PARAM_SLIDER_COMPONENTID").toInt();
    ui->nameLabel->setText(settings.value("QGC_PARAM_SLIDER_DESCRIPTION").toString());
    ui->editNameLabel->setText(settings.value("QGC_PARAM_SLIDER_DESCRIPTION").toString());
    //settings.setValue("QGC_PARAM_SLIDER_BUTTONTEXT", ui->actionButton->text());
    ui->editSelectParamComboBox->addItem(settings.value("QGC_PARAM_SLIDER_PARAMID").toString());
    ui->editSelectParamComboBox->setCurrentIndex(ui->editSelectParamComboBox->count()-1);
    ui->editSelectComponentComboBox->addItem(tr("Component #%1").arg(settings.value("QGC_PARAM_SLIDER_COMPONENTID").toInt()), settings.value("QGC_PARAM_SLIDER_COMPONENTID").toInt());
    ui->editMinSpinBox->setValue(settings.value("QGC_PARAM_SLIDER_MIN").toFloat());
    ui->editMaxSpinBox->setValue(settings.value("QGC_PARAM_SLIDER_MAX").toFloat());
    visibleParam = settings.value("QGC_PARAM_SLIDER_VISIBLE_PARAM","").toString();
             //QGC_TOOL_WIDGET_ITEMS\1\QGC_PARAM_SLIDER_VISIBLE_PARAM=RC5_FUNCTION
    visibleVal = settings.value("QGC_PARAM_SLIDER_VISIBLE_VAL",0).toInt();
    parameterMax = ui->editMaxSpinBox->value();
    parameterMin = ui->editMinSpinBox->value();
    showInfo(settings.value("QGC_PARAM_SLIDER_DISPLAY_INFO", true).toBool());
    ui->editSelectParamComboBox->setEnabled(true);
    ui->editSelectComponentComboBox->setEnabled(true);

    setActiveUAS(UASManager::instance()->getActiveUAS());

    // Get param value after settings have been loaded
    //requestParameter();
}
