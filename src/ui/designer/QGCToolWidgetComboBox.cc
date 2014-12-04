#include <QMenu>
#include <QContextMenuEvent>
#include <QSettings>
#include <QTimer>
#include <QToolTip>
#include <QDebug>

#include "QGCToolWidgetComboBox.h"
#include "ui_QGCToolWidgetComboBox.h"
#include "UASInterface.h"
#include "UASManager.h"


QGCToolWidgetComboBox::QGCToolWidgetComboBox(QWidget *parent) :
    QGCToolWidgetItem("Combo", parent),
    parameterName(""),
    parameterValue(0.0f),
    parameterScalingFactor(0.0),
    parameterMin(0.0f),
    parameterMax(0.0f),
    componentId(0),
    ui(new Ui::QGCToolWidgetComboBox)
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
    ui->infoLabel->hide();
    ui->editOptionComboBox->setEnabled(false);
    isDisabled = true;
    connect(ui->editDoneButton, SIGNAL(clicked()), this, SLOT(endEditMode()));
    connect(ui->editOptionComboBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(comboBoxIndexChanged(QString)));
    connect(ui->editAddItemButton,SIGNAL(clicked()),this,SLOT(addButtonClicked()));
    connect(ui->editRemoveItemButton,SIGNAL(clicked()),this,SLOT(delButtonClicked()));

    // Sending actions
    connect(ui->writeButton, SIGNAL(clicked()), this, SLOT(setParamPending()));
    connect(ui->editSelectComponentComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectComponent(int)));
    connect(ui->editSelectParamComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectParameter(int)));
    //connect(ui->valueSlider, SIGNAL(valueChanged(int)), this, SLOT(setSliderValue(int)));
    //connect(ui->doubleValueSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setParamValue(double)));
    //connect(ui->intValueSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setParamValue(int)));
    connect(ui->editNameLabel, SIGNAL(textChanged(QString)), ui->nameLabel, SLOT(setText(QString)));
    connect(ui->readButton, SIGNAL(clicked()), this, SLOT(requestParameter()));
    connect(ui->editRefreshParamsButton, SIGNAL(clicked()), this, SLOT(refreshParameter()));
    connect(ui->editInfoCheckBox, SIGNAL(clicked(bool)), this, SLOT(showInfo(bool)));
    // connect to self
    connect(ui->infoLabel, SIGNAL(released()), this, SLOT(showTooltip()));

    init();
}

QGCToolWidgetComboBox::~QGCToolWidgetComboBox()
{
    delete ui;
}

void QGCToolWidgetComboBox::showTooltip()
{
    QWidget* sender = dynamic_cast<QWidget*>(QObject::sender());

    if (sender)
    {
        QPoint point = mapToGlobal(ui->infoLabel->pos());
        QToolTip::showText(point, sender->toolTip());
    }
}

void QGCToolWidgetComboBox::refreshParameter()
{
    ui->editSelectParamComboBox->setEnabled(true);
    ui->editSelectComponentComboBox->setEnabled(true);
    if (uas && !parameterName.isEmpty()) {
        uas->getParamManager()->requestParameterUpdate(componentId,parameterName);
        ui->editStatusLabel->setText(tr("Requesting refresh..."));
    }
}

void QGCToolWidgetComboBox::setActiveUAS(UASInterface* activeUas)
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
        paramMgr = uas->getParamManager();
        // Update current param value
        //requestParameter();
        // Set param info

        QString text = paramMgr->dataModel()->getParamDescription(parameterName);
        if (!text.isEmpty()) {
            ui->infoLabel->setToolTip(text);
            ui->infoLabel->show();
        }
        // Force-uncheck and hide label if no description is available
        if (ui->editInfoCheckBox->isChecked())  {
            showInfo((text.length() > 0));
        }
    }
}

void QGCToolWidgetComboBox::requestParameter()
{
    if (!parameterName.isEmpty() && uas)
    {
        paramMgr->requestParameterUpdate(this->componentId, this->parameterName);
    }
}

void QGCToolWidgetComboBox::showInfo(bool enable)
{
    ui->editInfoCheckBox->setChecked(enable);
    ui->infoLabel->setVisible(enable);
}

void QGCToolWidgetComboBox::selectComponent(int componentIndex)
{
    this->componentId = ui->editSelectComponentComboBox->itemData(componentIndex).toInt();
}

void QGCToolWidgetComboBox::selectParameter(int paramIndex)
{
    // Set name
    parameterName = ui->editSelectParamComboBox->itemText(paramIndex);

    // Update min and max values if available
    if (uas)  {
        UASParameterDataModel* dataModel =  paramMgr->dataModel();
        if (dataModel) {
            // Minimum
            if (dataModel->isParamMinKnown(parameterName)) {
                parameterMin = dataModel->getParamMin(parameterName);
            }

            // Maximum
            if (dataModel->isParamMaxKnown(parameterName)) {
                parameterMax = dataModel->getParamMax(parameterName);
            }

            // Description
            QString text = dataModel->getParamDescription(parameterName);
            //ui->infoLabel->setText(text);
            showInfo(!(text.length() > 0));
        }
    }
}

void QGCToolWidgetComboBox::setEditMode(bool editMode)
{
    if(!editMode) {
        // Store component id
        selectComponent(ui->editSelectComponentComboBox->currentIndex());
        // Store parameter name and id
        selectParameter(ui->editSelectParamComboBox->currentIndex());
    }

    ui->nameLabel->setVisible(!editMode);
    ui->writeButton->setVisible(!editMode);
    ui->readButton->setVisible(!editMode);

    ui->editInfoCheckBox->setVisible(editMode);
    ui->editDoneButton->setVisible(editMode);
    ui->editNameLabel->setVisible(editMode);
    ui->editRefreshParamsButton->setVisible(editMode);
    ui->editSelectParamComboBox->setVisible(editMode);
    ui->editSelectComponentComboBox->setVisible(editMode);
    ui->editStatusLabel->setVisible(editMode);
    ui->writeButton->setVisible(!editMode);
    ui->readButton->setVisible(!editMode);
    ui->editLine1->setVisible(editMode);
    ui->editLine2->setVisible(editMode);
    ui->editAddItemButton->setVisible(editMode);
    ui->editRemoveItemButton->setVisible(editMode);
    ui->editItemValueSpinBox->setVisible(editMode);
    ui->editItemNameLabel->setVisible(editMode);
    ui->itemValueLabel->setVisible(editMode);
    ui->itemNameLabel->setVisible(editMode);
    if (isDisabled)
    {
        ui->editOptionComboBox->setEnabled(editMode);
    }

    QGCToolWidgetItem::setEditMode(editMode);
}

void QGCToolWidgetComboBox::setParamPending()
{
    if (uas)  {
        uas->getParamManager()->setPendingParam(componentId, parameterName, parameterValue);
    }
    else  {
        qWarning() << __FILE__ << __LINE__ << "NO UAS SET, DOING NOTHING";
    }
}


/**
 * @brief uas Unmanned system sending the parameter
 * @brief component UAS component sending the parameter
 * @brief parameterName Key/name of the parameter
 * @brief value Value of the parameter
 */
void QGCToolWidgetComboBox::setParameterValue(int uas, int component, int paramCount, int paramIndex, QString parameterName, QVariant value)
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
    if (visibleParam != "")
    {
        if (parameterName == visibleParam)
        {
            if (visibleVal == value.toInt())
            {
                this->uas->requestParameter(this->componentId,this->parameterName);
                visibleEnabled = true;
                this->show();
            }
            else
            {
                //Disable the component here.
                //ui->valueSlider->setEnabled(false);
                //ui->intValueSpinBox->setEnabled(false);
                //ui->doubleValueSpinBox->setEnabled(false);
                visibleEnabled = false;
                this->hide();
            }
        }
    }
    if (component == this->componentId && parameterName == this->parameterName)
    {
        if (!visibleEnabled)
        {
            return;
        }
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

void QGCToolWidgetComboBox::changeEvent(QEvent *e)
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


void QGCToolWidgetComboBox::writeSettings(QSettings& settings)
{
    settings.setValue("TYPE", "COMBOBOX");
    settings.setValue("QGC_PARAM_COMBOBOX_DESCRIPTION", ui->nameLabel->text());
    //settings.setValue("QGC_PARAM_COMBOBOX_BUTTONTEXT", ui->actionButton->text());
    settings.setValue("QGC_PARAM_COMBOBOX_PARAMID", parameterName);
    settings.setValue("QGC_PARAM_COMBOBOX_COMPONENTID", componentId);
    settings.setValue("QGC_PARAM_COMBOBOX_DISPLAY_INFO", ui->editInfoCheckBox->isChecked());

    settings.setValue("QGC_PARAM_COMBOBOX_COUNT", ui->editOptionComboBox->count());
    for (int i=0;i<ui->editOptionComboBox->count();i++)
    {
        settings.setValue("QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_TEXT",ui->editOptionComboBox->itemText(i));
        settings.setValue("QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_VAL",comboBoxTextToValMap[ui->editOptionComboBox->itemText(i)]);
    }
    settings.sync();
}
void QGCToolWidgetComboBox::readSettings(const QString& pre,const QVariantMap& settings)
{
    parameterName = settings.value(pre + "QGC_PARAM_COMBOBOX_PARAMID").toString();
    componentId = settings.value(pre + "QGC_PARAM_COMBOBOX_COMPONENTID").toInt();
    ui->nameLabel->setText(settings.value(pre + "QGC_PARAM_COMBOBOX_DESCRIPTION").toString());
    ui->editNameLabel->setText(settings.value(pre + "QGC_PARAM_COMBOBOX_DESCRIPTION").toString());
    //settings.setValue("QGC_PARAM_SLIDER_BUTTONTEXT", ui->actionButton->text());
    ui->editSelectParamComboBox->addItem(settings.value(pre + "QGC_PARAM_COMBOBOX_PARAMID").toString());
    ui->editSelectParamComboBox->setCurrentIndex(ui->editSelectParamComboBox->count()-1);
    ui->editSelectComponentComboBox->addItem(tr("Component #%1").arg(settings.value(pre + "QGC_PARAM_COMBOBOX_COMPONENTID").toInt()), settings.value(pre + "QGC_PARAM_COMBOBOX_COMPONENTID").toInt());
    showInfo(settings.value(pre + "QGC_PARAM_COMBOBOX_DISPLAY_INFO", true).toBool());
    ui->editSelectParamComboBox->setEnabled(true);
    ui->editSelectComponentComboBox->setEnabled(true);
    visibleParam = settings.value(pre+"QGC_PARAM_COMBOBOX_VISIBLE_PARAM","").toString();
    visibleVal = settings.value(pre+"QGC_PARAM_COMBOBOX_VISIBLE_VAL",0).toInt();
    QString type = settings.value(pre + "QGC_PARAM_COMBOBOX_TYPE","PARAM").toString();
    int num = settings.value(pre + "QGC_PARAM_COMBOBOX_COUNT").toInt();
    for (int i=0;i<num;i++)
    {
        QString pixmapfn = settings.value(pre + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_IMG","").toString();
        if (pixmapfn != "")
        {
            comboBoxIndexToPixmap[i] = QPixmap(pixmapfn);
        }
        ui->editOptionComboBox->addItem(settings.value(pre + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_TEXT").toString());
        //qDebug() << "Adding val:" << settings.value(pre + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_TEXT").toString() << settings.value(pre + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_VAL").toInt();
        comboBoxTextToValMap[settings.value(pre + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_TEXT").toString()] = settings.value(pre + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_VAL").toInt();
        if (type == "INDIVIDUAL")
        {
            comboBoxTextToParamMap[settings.value(pre + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_TEXT").toString()] = settings.value(pre + "QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_PARAM").toString();
            ui->editOptionComboBox->setEnabled(true);
        }
    }

    setActiveUAS(UASManager::instance()->getActiveUAS());

    // Get param value after settings have been loaded
   // requestParameter();
}
void QGCToolWidgetComboBox::readSettings(const QSettings& settings)
{
    QVariantMap map;
    foreach (QString key,settings.allKeys())
    {
        map[key] = settings.value(key);
    }

    readSettings("",map);

    //parameterName = settings.value("QGC_PARAM_COMBOBOX_PARAMID").toString();
    //component = settings.value("QGC_PARAM_COMBOBOX_COMPONENTID").toInt();
    //ui->nameLabel->setText(settings.value("QGC_PARAM_COMBOBOX_DESCRIPTION").toString());
    //ui->editNameLabel->setText(settings.value("QGC_PARAM_COMBOBOX_DESCRIPTION").toString());
    //ui->editSelectParamComboBox->addItem(settings.value("QGC_PARAM_COMBOBOX_PARAMID").toString());
    //ui->editSelectParamComboBox->setCurrentIndex(ui->editSelectParamComboBox->count()-1);
    //ui->editSelectComponentComboBox->addItem(tr("Component #%1").arg(settings.value("QGC_PARAM_COMBOBOX_COMPONENTID").toInt()), settings.value("QGC_PARAM_COMBOBOX_COMPONENTID").toInt());
    //showInfo(settings.value("QGC_PARAM_COMBOBOX_DISPLAY_INFO", true).toBool());
    //ui->editSelectParamComboBox->setEnabled(true);
    //ui->editSelectComponentComboBox->setEnabled(true);


    /*int num = settings.value("QGC_PARAM_COMBOBOX_COUNT").toInt();
    for (int i=0;i<num;i++)
    {
        QString pixmapfn = settings.value("QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_IMG","").toString();
        if (pixmapfn != "")
        {
            comboBoxIndexToPixmap[i] = QPixmap(pixmapfn);
        }
        ui->editOptionComboBox->addItem(settings.value("QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_TEXT").toString());
        qDebug() << "Adding val:" << settings.value("QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i)).toString() << settings.value("QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_VAL").toInt();
        comboBoxTextToValMap[settings.value("QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_TEXT").toString()] = settings.value("QGC_PARAM_COMBOBOX_ITEM_" + QString::number(i) + "_VAL").toInt();
    }*/

    //setActiveUAS(UASManager::instance()->getActiveUAS());*/

    // Get param value after settings have been loaded
    //requestParameter();
}
void QGCToolWidgetComboBox::addButtonClicked()
{
    ui->editOptionComboBox->addItem(ui->editItemNameLabel->text());
    comboBoxTextToValMap[ui->editItemNameLabel->text()] = ui->editItemValueSpinBox->value();
}

void QGCToolWidgetComboBox::delButtonClicked()
{
    int index = ui->editOptionComboBox->currentIndex();
    comboBoxTextToValMap.remove(ui->editOptionComboBox->currentText());
    ui->editOptionComboBox->removeItem(index);
}
void QGCToolWidgetComboBox::comboBoxIndexChanged(QString val)
{
    ui->imageLabel->setPixmap(comboBoxIndexToPixmap[ui->editOptionComboBox->currentIndex()]);
    if (comboBoxTextToParamMap.contains(ui->editOptionComboBox->currentText()))
    {
        parameterName = comboBoxTextToParamMap.value(ui->editOptionComboBox->currentText());
    }
    switch (static_cast<int>(parameterValue.type()))
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
