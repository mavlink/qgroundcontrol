#include <QMenu>
#include <QContextMenuEvent>
#include <QSettings>
#include <QTimer>
#include <QToolTip>

#include "QGCComboBox.h"
#include "ui_QGCComboBox.h"
#include "UASInterface.h"
#include "UASManager.h"


QGCComboBox::QGCComboBox(QWidget *parent) :
    QGCToolWidgetItem("Combo", parent),
    parameterName(""),
    parameterValue(0.0f),
    parameterScalingFactor(0.0),
    parameterMin(0.0f),
    parameterMax(0.0f),
    component(0),
    ui(new Ui::QGCComboBox)
{
    ui->setupUi(this);
    uas = NULL;


    ui->editInfoCheckBox->hide();
    ui->editDoneButton->hide();
    ui->editNameLabel->hide();
    ui->editRefreshParamsButton->hide();
    ui->editSelectParamComboBox->hide();
    ui->editSelectComponentComboBox->hide();
    ui->editStatusLabel->hide();
    ui->editLine1->hide();
    ui->editLine2->hide();
    ui->editAddItemButton->hide();
    ui->editRemoveItemButton->hide();
    ui->editItemValueSpinBox->hide();
    ui->editItemNameLabel->hide();
    ui->itemValueLabel->hide();
    ui->itemNameLabel->hide();
    ui->editOptionComboBox->setEnabled(false);
    isDisabled = true;
    //ui->editLine1->setStyleSheet("QWidget { border: 1px solid #66666B; border-radius: 3px; padding: 10px 0px 0px 0px; background: #111122; }");
   // ui->editLine2->setStyleSheet("QWidget { border: 1px solid #66666B; border-radius: 3px; padding: 10px 0px 0px 0px; background: #111122; }");
    connect(ui->editDoneButton, SIGNAL(clicked()), this, SLOT(endEditMode()));
    connect(ui->editOptionComboBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(comboBoxIndexChanged(QString)));
    connect(ui->editAddItemButton,SIGNAL(clicked()),this,SLOT(addButtonClicked()));
    connect(ui->editRemoveItemButton,SIGNAL(clicked()),this,SLOT(delButtonClicked()));

    // Sending actions
    connect(ui->writeButton, SIGNAL(clicked()), this, SLOT(sendParameter()));
    connect(ui->editSelectComponentComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectComponent(int)));
    connect(ui->editSelectParamComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectParameter(int)));
    //connect(ui->valueSlider, SIGNAL(valueChanged(int)), this, SLOT(setSliderValue(int)));
    //connect(ui->doubleValueSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setParamValue(double)));
    //connect(ui->intValueSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setParamValue(int)));
    connect(ui->editNameLabel, SIGNAL(textChanged(QString)), ui->nameLabel, SLOT(setText(QString)));
    connect(ui->readButton, SIGNAL(clicked()), this, SLOT(requestParameter()));
    connect(ui->editRefreshParamsButton, SIGNAL(clicked()), this, SLOT(refreshParamList()));
    connect(ui->editInfoCheckBox, SIGNAL(clicked(bool)), this, SLOT(showInfo(bool)));
    // connect to self
    connect(ui->infoLabel, SIGNAL(released()), this, SLOT(showTooltip()));
    // Set the current UAS if present

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

}

QGCComboBox::~QGCComboBox()
{
    delete ui;
}

void QGCComboBox::showTooltip()
{
    QWidget* sender = dynamic_cast<QWidget*>(QObject::sender());

    if (sender)
    {
        QPoint point = mapToGlobal(pos());
        QToolTip::showText(point, sender->toolTip());
    }
}

void QGCComboBox::refreshParamList()
{
    ui->editSelectParamComboBox->setEnabled(true);
    ui->editSelectComponentComboBox->setEnabled(true);
    if (uas)
    {
        uas->getParamManager()->requestParameterList();
        ui->editStatusLabel->setText(tr("Parameter list updating.."));
    }
}

void QGCComboBox::setActiveUAS(UASInterface* activeUas)
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
        ui->infoLabel->setToolTip(text);
        // Force-uncheck and hide label if no description is available
        if (ui->editInfoCheckBox->isChecked())
        {
            showInfo((text.length() > 0));
        }
    }
}

void QGCComboBox::requestParameter()
{
    if (!parameterName.isEmpty() && uas)
    {
        uas->getParamManager()->requestParameterUpdate(this->component, this->parameterName);
    }
}

void QGCComboBox::showInfo(bool enable)
{
    ui->editInfoCheckBox->setChecked(enable);
    ui->infoLabel->setVisible(enable);
}

void QGCComboBox::selectComponent(int componentIndex)
{
    this->component = ui->editSelectComponentComboBox->itemData(componentIndex).toInt();
}

void QGCComboBox::selectParameter(int paramIndex)
{
    // Set name
    parameterName = ui->editSelectParamComboBox->itemText(paramIndex);

    // Update min and max values if available
    if (uas)
    {
        if (uas->getParamManager())
        {
            // Current value
            uas->getParamManager()->requestParameterUpdate(component, parameterName);

            // Minimum
            if (uas->getParamManager()->isParamMinKnown(parameterName))
            {
                parameterMin = uas->getParamManager()->getParamMin(parameterName);
            }

            // Maximum
            if (uas->getParamManager()->isParamMaxKnown(parameterName))
            {
                parameterMax = uas->getParamManager()->getParamMax(parameterName);
            }

            // Description
            QString text = uas->getParamManager()->getParamInfo(parameterName);
            ui->infoLabel->setText(text);
            showInfo(!(text.length() > 0));
        }
    }
}

void QGCComboBox::startEditMode()
{
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
    ui->writeButton->hide();
    ui->readButton->hide();
    ui->editLine1->show();
    ui->editLine2->show();
    ui->editAddItemButton->show();
    ui->editRemoveItemButton->show();
    ui->editItemValueSpinBox->show();
    ui->editItemNameLabel->show();
    ui->itemValueLabel->show();
    ui->itemNameLabel->show();
    if (isDisabled)
    {
        ui->editOptionComboBox->setEnabled(true);
    }
    isInEditMode = true;
}

void QGCComboBox::endEditMode()
{
    // Store component id
    selectComponent(ui->editSelectComponentComboBox->currentIndex());

    // Store parameter name and id
    selectParameter(ui->editSelectParamComboBox->currentIndex());

    // Min/max

    ui->editInfoCheckBox->hide();
    ui->editDoneButton->hide();
    ui->editNameLabel->hide();
    ui->editRefreshParamsButton->hide();
    ui->editSelectParamComboBox->hide();
    ui->editSelectComponentComboBox->hide();
    ui->editStatusLabel->hide();
    ui->editLine1->hide();
    ui->editLine2->hide();
    ui->writeButton->show();
    ui->readButton->show();
    ui->editAddItemButton->hide();
    ui->editRemoveItemButton->hide();
    ui->editItemValueSpinBox->hide();
    ui->editItemNameLabel->hide();
    ui->itemValueLabel->hide();
    ui->itemNameLabel->hide();
    ui->nameLabel->show();
    if (isDisabled)
    {
        ui->editOptionComboBox->setEnabled(false);
    }
    isInEditMode = false;
    emit editingFinished();
}

void QGCComboBox::sendParameter()
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


/**
 * @brief uas Unmanned system sending the parameter
 * @brief component UAS component sending the parameter
 * @brief parameterName Key/name of the parameter
 * @brief value Value of the parameter
 */
void QGCComboBox::setParameterValue(int uas, int component, int paramCount, int paramIndex, QString parameterName, QVariant value)
{
    Q_UNUSED(paramCount);
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

    Q_UNUSED(uas);

     //comboBoxTextToValMap[ui->editItemNameLabel->text()] = ui->editItemValueSpinBox->value();

    if (component == this->component && parameterName == this->parameterName)
    {
        ui->editOptionComboBox->setEnabled(true);
        isDisabled = false;
        for (int i=0;i<ui->editOptionComboBox->count();i++)
        {
            if (comboBoxTextToValMap[ui->editOptionComboBox->itemText(i)] == value.toInt())
            {
                ui->editOptionComboBox->setCurrentIndex(i);
                break;
            }
        }
    }

    if (paramIndex == paramCount - 1)
    {
        ui->editStatusLabel->setText(tr("Complete parameter list received."));
    }
}

void QGCComboBox::changeEvent(QEvent *e)
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


void QGCComboBox::writeSettings(QSettings& settings)
{
    settings.setValue("TYPE", "COMBOBOX");
    settings.setValue("QGC_PARAM_COMBOBOX_DESCRIPTION", ui->nameLabel->text());
    //settings.setValue("QGC_PARAM_COMBOBOX_BUTTONTEXT", ui->actionButton->text());
    settings.setValue("QGC_PARAM_COMBOBOX_PARAMID", parameterName);
    settings.setValue("QGC_PARAM_COMBOBOX_COMPONENTID", component);
    settings.setValue("QGC_PARAM_COMBOBOX_DISPLAY_INFO", ui->editInfoCheckBox->isChecked());

    settings.setValue("QGC_PARAM_COMBOBOX_COUNT", ui->editOptionComboBox->count());
    for (int i=0;i<ui->editOptionComboBox->count();i++)
    {
        settings.setValue("QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_TEXT",ui->editOptionComboBox->itemText(i));
        settings.setValue("QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_VAL",comboBoxTextToValMap[ui->editOptionComboBox->itemText(i)]);
    }
    settings.sync();
}
void QGCComboBox::readSettings(const QString& pre,const QVariantMap& settings)
{
    parameterName = settings.value(pre + "QGC_PARAM_COMBOBOX_PARAMID").toString();
    component = settings.value(pre + "QGC_PARAM_COMBOBOX_COMPONENTID").toInt();
    ui->nameLabel->setText(settings.value(pre + "QGC_PARAM_COMBOBOX_DESCRIPTION").toString());
    ui->editNameLabel->setText(settings.value(pre + "QGC_PARAM_COMBOBOX_DESCRIPTION").toString());
    //settings.setValue("QGC_PARAM_SLIDER_BUTTONTEXT", ui->actionButton->text());
    ui->editSelectParamComboBox->addItem(settings.value(pre + "QGC_PARAM_COMBOBOX_PARAMID").toString());
    ui->editSelectParamComboBox->setCurrentIndex(ui->editSelectParamComboBox->count()-1);
    ui->editSelectComponentComboBox->addItem(tr("Component #%1").arg(settings.value(pre + "QGC_PARAM_COMBOBOX_COMPONENTID").toInt()), settings.value(pre + "QGC_PARAM_COMBOBOX_COMPONENTID").toInt());
    showInfo(settings.value(pre + "QGC_PARAM_COMBOBOX_DISPLAY_INFO", true).toBool());
    ui->editSelectParamComboBox->setEnabled(true);
    ui->editSelectComponentComboBox->setEnabled(true);


    int num = settings.value(pre + "QGC_PARAM_COMBOBOX_COUNT").toInt();
    for (int i=0;i<num;i++)
    {
        ui->editOptionComboBox->addItem(settings.value(pre + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_TEXT").toString());
        //qDebug() << "Adding val:" << settings.value(pre + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_TEXT").toString() << settings.value(pre + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_VAL").toInt();
        comboBoxTextToValMap[settings.value(pre + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_TEXT").toString()] = settings.value(pre + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_VAL").toInt();
    }

    setActiveUAS(UASManager::instance()->getActiveUAS());

    // Get param value after settings have been loaded
   // requestParameter();
}
void QGCComboBox::readSettings(const QSettings& settings)
{
    parameterName = settings.value("QGC_PARAM_COMBOBOX_PARAMID").toString();
    component = settings.value("QGC_PARAM_COMBOBOX_COMPONENTID").toInt();
    ui->nameLabel->setText(settings.value("QGC_PARAM_COMBOBOX_DESCRIPTION").toString());
    ui->editNameLabel->setText(settings.value("QGC_PARAM_COMBOBOX_DESCRIPTION").toString());
    //settings.setValue("QGC_PARAM_SLIDER_BUTTONTEXT", ui->actionButton->text());
    ui->editSelectParamComboBox->addItem(settings.value("QGC_PARAM_COMBOBOX_PARAMID").toString());
    ui->editSelectParamComboBox->setCurrentIndex(ui->editSelectParamComboBox->count()-1);
    ui->editSelectComponentComboBox->addItem(tr("Component #%1").arg(settings.value("QGC_PARAM_COMBOBOX_COMPONENTID").toInt()), settings.value("QGC_PARAM_COMBOBOX_COMPONENTID").toInt());
    showInfo(settings.value("QGC_PARAM_COMBOBOX_DISPLAY_INFO", true).toBool());
    ui->editSelectParamComboBox->setEnabled(true);
    ui->editSelectComponentComboBox->setEnabled(true);


    int num = settings.value("QGC_PARAM_COMBOBOX_COUNT").toInt();
    for (int i=0;i<num;i++)
    {
        ui->editOptionComboBox->addItem(settings.value("QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_TEXT").toString());
        qDebug() << "Adding val:" << settings.value("QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i)).toString() << settings.value("QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_VAL").toInt();
        comboBoxTextToValMap[settings.value("QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_TEXT").toString()] = settings.value("QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_VAL").toInt();
    }

    setActiveUAS(UASManager::instance()->getActiveUAS());

    // Get param value after settings have been loaded
    requestParameter();
}
void QGCComboBox::addButtonClicked()
{
    ui->editOptionComboBox->addItem(ui->editItemNameLabel->text());
    comboBoxTextToValMap[ui->editItemNameLabel->text()] = ui->editItemValueSpinBox->value();
}

void QGCComboBox::delButtonClicked()
{
    int index = ui->editOptionComboBox->currentIndex();
    comboBoxTextToValMap.remove(ui->editOptionComboBox->currentText());
    ui->editOptionComboBox->removeItem(index);
}
void QGCComboBox::comboBoxIndexChanged(QString val)
{
    switch (parameterValue.type())
    {
    case QVariant::Char:
        parameterValue = QVariant(QChar((unsigned char)comboBoxTextToValMap[val]));
        break;
    case QVariant::Int:
        parameterValue = (int)comboBoxTextToValMap[val];
        break;
    case QVariant::UInt:
        parameterValue = (unsigned int)comboBoxTextToValMap[val];
        break;
    case QMetaType::Float:
        parameterValue =(float)comboBoxTextToValMap[val];
        break;
    default:
        qCritical() << "ERROR: NO VALID PARAM TYPE";
        return;
    }
}
