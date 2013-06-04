#include "UASQuickView.h"
#include <QMetaMethod>
#include <QDebug>
#include "UASQuickViewItemSelect.h"
#include "UASQuickViewTextItem.h"
#include <QSettings>
UASQuickView::UASQuickView(QWidget *parent) : QWidget(parent)
{
    quickViewSelectDialog=0;
    ui.setupUi(this);
    connect(UASManager::instance(),SIGNAL(activeUASSet(UASInterface*)),this,SLOT(setActiveUAS(UASInterface*)));
    connect(UASManager::instance(),SIGNAL(UASCreated(UASInterface*)),this,SLOT(addUAS(UASInterface*)));
    if (UASManager::instance()->getActiveUAS())
    {
        addUAS(UASManager::instance()->getActiveUAS());
    }
    this->setContextMenuPolicy(Qt::ActionsContextMenu);


    /*{
        QAction *action = new QAction("latitude",this);
        action->setCheckable(true);
        action->setChecked(true);
        connect(action,SIGNAL(toggled(bool)),this,SLOT(actionTriggered(bool)));
        this->addAction(action);
        UASQuickViewItem *item = new UASQuickViewItem(this);
        item->setTitle("latitude");
        ui.verticalLayout->addWidget(item);
        uasPropertyToLabelMap["latitude"] = item;
    }

    {
        QAction *action = new QAction("longitude",this);
        action->setCheckable(true);
        action->setChecked(true);
        connect(action,SIGNAL(toggled(bool)),this,SLOT(actionTriggered(bool)));
        this->addAction(action);
        UASQuickViewItem *item = new UASQuickViewItem(this);
        item->setTitle("longitude");
        ui.verticalLayout->addWidget(item);
        uasPropertyToLabelMap["longitude"] = item;
    }

    {
        QAction *action = new QAction("altitude",this);
        action->setCheckable(true);
        action->setChecked(true);
        connect(action,SIGNAL(toggled(bool)),this,SLOT(actionTriggered(bool)));
        this->addAction(action);
        UASQuickViewItem *item = new UASQuickViewItem(this);
        item->setTitle("altitude");
        ui.verticalLayout->addWidget(item);
        uasPropertyToLabelMap["altitude"] = item;
    }

    {
        QAction *action = new QAction("satelliteCount",this);
        action->setCheckable(true);
        action->setChecked(true);
        connect(action,SIGNAL(toggled(bool)),this,SLOT(actionTriggered(bool)));
        this->addAction(action);
        UASQuickViewItem *item = new UASQuickViewItem(this);
        item->setTitle("satelliteCount");
        ui.verticalLayout->addWidget(item);
        uasPropertyToLabelMap["satelliteCount"] = item;
    }

    {
        QAction *action = new QAction("distToWaypoint",this);
        action->setCheckable(true);
        action->setChecked(true);
        connect(action,SIGNAL(toggled(bool)),this,SLOT(actionTriggered(bool)));
        this->addAction(action);
        UASQuickViewItem *item = new UASQuickViewItem(this);
        item->setTitle("distToWaypoint");
        ui.verticalLayout->addWidget(item);
        uasPropertyToLabelMap["distToWaypoint"] = item;
    }*/

    loadSettings();
    //If we don't have any predefined settings, set some defaults.
    if (uasPropertyValueMap.size() == 0)
    {
        valueEnabled("altitude");
        valueEnabled("groundSpeed");
        valueEnabled("distToWaypoint");
        valueEnabled("yaw");
        valueEnabled("roll");
    }

    QAction *action = new QAction("Add Item",this);
    action->setCheckable(false);
    connect(action,SIGNAL(triggered()),this,SLOT(actionTriggered()));
    this->addAction(action);

    updateTimer = new QTimer(this);
    connect(updateTimer,SIGNAL(timeout()),this,SLOT(updateTimerTick()));
    updateTimer->start(1000);
}
void UASQuickView::actionTriggered()
{
    if (quickViewSelectDialog)
    {
        quickViewSelectDialog->show();
        return;
    }
    quickViewSelectDialog = new UASQuickViewItemSelect();
    connect(quickViewSelectDialog,SIGNAL(destroyed()),this,SLOT(selectDialogClosed()));
    connect(quickViewSelectDialog,SIGNAL(valueDisabled(QString)),this,SLOT(valueDisabled(QString)));
    connect(quickViewSelectDialog,SIGNAL(valueEnabled(QString)),this,SLOT(valueEnabled(QString)));
    quickViewSelectDialog->setAttribute(Qt::WA_DeleteOnClose,true);
    for (QMap<QString,double>::const_iterator i = uasPropertyValueMap.constBegin();i!=uasPropertyValueMap.constEnd();i++)
    {
        quickViewSelectDialog->addItem(i.key(),uasEnabledPropertyList.contains(i.key()));
    }
    quickViewSelectDialog->show();
}
void UASQuickView::saveSettings()
{
    QSettings settings;
    settings.beginWriteArray("UAS_QUICK_VIEW_ITEMS");
    int count = 0;
    for (QMap<QString,UASQuickViewItem*>::const_iterator i = uasPropertyToLabelMap.constBegin();i!=uasPropertyToLabelMap.constEnd();i++)
    {
        settings.setArrayIndex(count++);
        settings.setValue("name",i.key());
        settings.setValue("type","text");
    }
    settings.endArray();
    settings.sync();
}
void UASQuickView::loadSettings()
{
    QSettings settings;
    int size = settings.beginReadArray("UAS_QUICK_VIEW_ITEMS");
    for (int i=0;i<size;i++)
    {
        settings.setArrayIndex(i);
        QString nameval = settings.value("name").toString();
        QString typeval = settings.value("type").toString();
        if (typeval == "text" && !uasPropertyToLabelMap.contains(nameval))
        {
            valueEnabled(nameval);
        }
    }
}

void UASQuickView::valueEnabled(QString value)
{
    UASQuickViewItem *item = new UASQuickViewTextItem(this);
    item->setTitle(value);
    ui.verticalLayout->addWidget(item);
    uasPropertyToLabelMap[value] = item;
    uasEnabledPropertyList.append(value);
    if (!uasPropertyValueMap.contains(value))
    {
        uasPropertyValueMap[value] = 0;
    }
    saveSettings();

}

void UASQuickView::valueDisabled(QString value)
{
    if (uasPropertyToLabelMap.contains(value))
    {
        UASQuickViewItem *item = uasPropertyToLabelMap[value];
        uasPropertyToLabelMap.remove(value);
        item->hide();
        ui.verticalLayout->removeWidget(item);
        item->deleteLater();
        uasEnabledPropertyList.removeOne(value);
        saveSettings();
    }
}

void UASQuickView::selectDialogClosed()
{
    quickViewSelectDialog = 0;
}

void UASQuickView::updateTimerTick()
{
    //uasPropertyValueMap
    for (QMap<QString,UASQuickViewItem*>::const_iterator i = uasPropertyToLabelMap.constBegin(); i != uasPropertyToLabelMap.constEnd();i++)
    {
        if (uasPropertyValueMap.contains(i.key()))
        {
            i.value()->setValue(uasPropertyValueMap[i.key()]);
        }
    }
}

void UASQuickView::addUAS(UASInterface* uas)
{
    if (uas)
    {
        if (!this->uas)
        {
            setActiveUAS(uas);
        }
    }
}

void UASQuickView::setActiveUAS(UASInterface* uas)
{
    if (!uas)
    {
        return;
    }
    this->uas = uas;
    connect(uas,SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)),this,SLOT(valueChanged(int,QString,QString,QVariant,quint64)));
    //connect(uas,SIGNAL())
}
void UASQuickView::addSource(MAVLinkDecoder *decoder)
{
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,double,quint64)),this,SLOT(valueChanged(int,QString,QString,double,quint64)));
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,qint8,quint64)),this,SLOT(valueChanged(int,QString,QString,qint8,quint64)));
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,qint16,quint64)),this,SLOT(valueChanged(int,QString,QString,qint16,quint64)));
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,qint32,quint64)),this,SLOT(valueChanged(int,QString,QString,qint32,quint64)));
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,qint64,quint64)),this,SLOT(valueChanged(int,QString,QString,qint64,quint64)));
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,quint8,quint64)),this,SLOT(valueChanged(int,QString,QString,quint8,quint64)));
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,quint16,quint64)),this,SLOT(valueChanged(int,QString,QString,quint16,quint64)));
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,quint32,quint64)),this,SLOT(valueChanged(int,QString,QString,quint32,quint64)));
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,quint64,quint64)),this,SLOT(valueChanged(int,QString,QString,quint64,quint64)));
}
void UASQuickView::valueChanged(const int uasId, const QString& name, const QString& unit, const quint8 value, const quint64 msec)
{
    if (!uasPropertyValueMap.contains(name))
    {
        if (quickViewSelectDialog)
        {
            quickViewSelectDialog->addItem(name);
        }
    }
    uasPropertyValueMap[name] = value;
}

void UASQuickView::valueChanged(const int uasId, const QString& name, const QString& unit, const qint8 value, const quint64 msec)
{
    if (!uasPropertyValueMap.contains(name))
    {
        if (quickViewSelectDialog)
        {
            quickViewSelectDialog->addItem(name);
        }
    }
    uasPropertyValueMap[name] = value;
}
void UASQuickView::valueChanged(const int uasId, const QString& name, const QString& unit, const quint16 value, const quint64 msec)
{
    if (!uasPropertyValueMap.contains(name))
    {
        if (quickViewSelectDialog)
        {
            quickViewSelectDialog->addItem(name);
        }
    }
    uasPropertyValueMap[name] = value;
}
void UASQuickView::valueChanged(const int uasId, const QString& name, const QString& unit, const qint16 value, const quint64 msec)
{
    if (!uasPropertyValueMap.contains(name))
    {
        if (quickViewSelectDialog)
        {
            quickViewSelectDialog->addItem(name);
        }
    }
    uasPropertyValueMap[name] = value;
}
void UASQuickView::valueChanged(const int uasId, const QString& name, const QString& unit, const quint32 value, const quint64 msec)
{
    if (!uasPropertyValueMap.contains(name))
    {
        if (quickViewSelectDialog)
        {
            quickViewSelectDialog->addItem(name);
        }
    }
    uasPropertyValueMap[name] = value;
}
void UASQuickView::valueChanged(const int uasId, const QString& name, const QString& unit, const qint32 value, const quint64 msec)
{
    if (!uasPropertyValueMap.contains(name))
    {
        if (quickViewSelectDialog)
        {
            quickViewSelectDialog->addItem(name);
        }
    }
    uasPropertyValueMap[name] = value;
}
void UASQuickView::valueChanged(const int uasId, const QString& name, const QString& unit, const quint64 value, const quint64 msec)
{
    if (!uasPropertyValueMap.contains(name))
    {
        if (quickViewSelectDialog)
        {
            quickViewSelectDialog->addItem(name);
        }
    }
    uasPropertyValueMap[name] = value;
}
void UASQuickView::valueChanged(const int uasId, const QString& name, const QString& unit, const qint64 value, const quint64 msec)
{
    if (!uasPropertyValueMap.contains(name))
    {
        if (quickViewSelectDialog)
        {
            quickViewSelectDialog->addItem(name);
        }
    }
    uasPropertyValueMap[name] = value;
}
void UASQuickView::valueChanged(const int uasId, const QString& name, const QString& unit, const double value, const quint64 msec)
{
    if (!uasPropertyValueMap.contains(name))
    {
        if (quickViewSelectDialog)
        {
            quickViewSelectDialog->addItem(name);
        }
    }
    uasPropertyValueMap[name] = value;
}

void UASQuickView::actionTriggered(bool checked)
{
    QAction *senderlabel = qobject_cast<QAction*>(sender());
    if (!senderlabel)
    {
        return;
    }
    if (checked)
    {
        UASQuickViewItem *item = new UASQuickViewTextItem(this);
        item->setTitle(senderlabel->text());
        ui.verticalLayout->addWidget(item);
        uasPropertyToLabelMap[senderlabel->text()] = item;
    }
    else
    {
        ui.verticalLayout->removeWidget(uasPropertyToLabelMap[senderlabel->text()]);
        uasPropertyToLabelMap[senderlabel->text()]->deleteLater();
        uasPropertyToLabelMap.remove(senderlabel->text());

    }
}
void UASQuickView::valueChanged(const int uasid, const QString& name, const QString& unit, const QVariant value,const quint64 msecs)
{
    uasPropertyValueMap[name] = value.toDouble();
}

void UASQuickView::valChanged(double val,QString type)
{
    //qDebug() << "Value changed:" << type << val;
   // uasPropertyValueMap[type] = val;
}
