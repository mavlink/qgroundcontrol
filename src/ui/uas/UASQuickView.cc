#include "UASQuickView.h"
#include <QMetaMethod>
#include <QDebug>
#include "UASQuickViewItemSelect.h"
#include "UASQuickViewTextItem.h"
#include <QSettings>

UASQuickView::UASQuickView(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::UASQuickView)
{
    m_ui->setupUi(this);
    quickViewSelectDialog=0;

    connect(UASManager::instance(),SIGNAL(activeUASSet(UASInterface*)),this,SLOT(setActiveUAS(UASInterface*)));
    connect(UASManager::instance(),SIGNAL(UASCreated(UASInterface*)),this,SLOT(addUAS(UASInterface*)));
    if (UASManager::instance()->getActiveUAS())
    {
        addUAS(UASManager::instance()->getActiveUAS());
    }
    this->setContextMenuPolicy(Qt::ActionsContextMenu);

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
    m_ui->verticalLayout->addWidget(item);
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
        m_ui->verticalLayout->removeWidget(item);
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
    if (this->uas)
    {
        uasPropertyList.clear();
        uasPropertyValueMap.clear();
        foreach (UASQuickViewItem* i, uasPropertyToLabelMap.values())
        {
            i->deleteLater();
        }
        uasPropertyToLabelMap.clear();

        updateTimer->stop();
        foreach (QAction* i, this->actions())
        {
            i->deleteLater();
        }
    }

    // Update the UAS to point to the new one.
    this->uas = uas;

    if (this->uas)
    {
        connect(uas,SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)),this,SLOT(valueChanged(int,QString,QString,QVariant,quint64)));
        updateTimer->start(1000);
    }
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
    Q_UNUSED(uasId);
    Q_UNUSED(unit);
    Q_UNUSED(msec);
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
    Q_UNUSED(uasId);
    Q_UNUSED(unit);
    Q_UNUSED(msec);
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
    Q_UNUSED(uasId);
    Q_UNUSED(unit);
    Q_UNUSED(msec);
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
    Q_UNUSED(uasId);
    Q_UNUSED(unit);
    Q_UNUSED(msec);
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
    Q_UNUSED(uasId);
    Q_UNUSED(unit);
    Q_UNUSED(msec);
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
    Q_UNUSED(uasId);
    Q_UNUSED(unit);
    Q_UNUSED(msec);
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
    Q_UNUSED(uasId);
    Q_UNUSED(unit);
    Q_UNUSED(msec);
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
    Q_UNUSED(uasId);
    Q_UNUSED(unit);
    Q_UNUSED(msec);
    if (!uasPropertyValueMap.contains(name))
    {
        if (quickViewSelectDialog)
        {
            quickViewSelectDialog->addItem(name);
        }

        // And periodically update the view.
        updateTimer->start(1000);
    }
    uasPropertyValueMap[name] = value;
}

void UASQuickView::valueChanged(const int uasId, const QString& name, const QString& unit, const double value, const quint64 msec)
{
    Q_UNUSED(uasId);
    Q_UNUSED(unit);
    Q_UNUSED(msec);
    if (!uasPropertyValueMap.contains(name))
    {
        if (quickViewSelectDialog)
        {
            quickViewSelectDialog->addItem(name);
        }
    }
    uasPropertyValueMap[name] = value;
}

void UASQuickView::valueChanged(const int uasId, const QString& name, const QString& unit, const QVariant value,const quint64 msec)
{
    Q_UNUSED(uasId);
    Q_UNUSED(unit);
    Q_UNUSED(msec);
    uasPropertyValueMap[name] = value.toDouble();
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
        this->layout()->addWidget(item);
        uasPropertyToLabelMap[senderlabel->text()] = item;
    }
    else
    {
        this->layout()->removeWidget(uasPropertyToLabelMap[senderlabel->text()]);
        uasPropertyToLabelMap[senderlabel->text()]->deleteLater();
        uasPropertyToLabelMap.remove(senderlabel->text());

    }
}

void UASQuickView::valueChanged(const int uasid, const QString& name, const QString& unit, const QVariant value,const quint64 msecs)
{
    Q_UNUSED(uasid);
    Q_UNUSED(unit);
    Q_UNUSED(msecs);
    uasPropertyValueMap[name] = value.toDouble();
}

void UASQuickView::valChanged(double val,QString type)
{
    Q_UNUSED(val);
    Q_UNUSED(type);
}
