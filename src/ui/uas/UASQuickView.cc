#include "UASQuickView.h"
#include <QMetaMethod>
#include <QDebug>
#include "UASQuickViewItemSelect.h"
#include "UASQuickViewTextItem.h"
#include <QSettings>
#include <QInputDialog>
UASQuickView::UASQuickView(QWidget *parent) : QWidget(parent)
{
    quickViewSelectDialog=0;
    m_columnCount=2;
    m_currentColumn=0;
    ui.setupUi(this);

    m_verticalLayoutList.append(new QVBoxLayout());
    ui.horizontalLayout->addItem(m_verticalLayoutList[0]);

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
        valueEnabled("distToWP");
        valueEnabled("yaw");
        valueEnabled("roll");
    }

    QAction *action = new QAction("Add Item",this);
    action->setCheckable(false);
    connect(action,SIGNAL(triggered()),this,SLOT(actionTriggered()));
    this->addAction(action);

    QAction *columnaction = new QAction("Set Column Count",this);
    columnaction->setCheckable(false);
    connect(columnaction,SIGNAL(triggered()),this,SLOT(columnActionTriggered()));
    this->addAction(columnaction);

    updateTimer = new QTimer(this);
    connect(updateTimer,SIGNAL(timeout()),this,SLOT(updateTimerTick()));
    updateTimer->start(1000);

}
UASQuickView::~UASQuickView()
{
    if (quickViewSelectDialog)
    {
        delete quickViewSelectDialog;
    }
}
void UASQuickView::columnActionTriggered()
{
    bool ok = false;
    int newcolumns = QInputDialog::getInt(this,"Columns","Enter number of columns",1,0,100,1,&ok);
    if (!ok)
    {
        return;
    }
    m_columnCount = newcolumns;
    sortItems(newcolumns);
    saveSettings();
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
    settings.setValue("UAS_QUICK_VIEW_COLUMNS",m_columnCount);
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
    settings.endArray();
    m_columnCount = settings.value("UAS_QUICK_VIEW_COLUMNS",1).toInt();
    sortItems(m_columnCount);
}

void UASQuickView::valueEnabled(QString value)
{
    UASQuickViewItem *item = new UASQuickViewTextItem(this);
    item->setTitle(value);
    //ui.verticalLayout->addWidget(item);
    //m_currentColumn
    m_verticalLayoutList[m_currentColumn]->addWidget(item);
    m_PropertyToLayoutIndexMap[value] = m_currentColumn;
    m_currentColumn++;
    if (m_currentColumn >= m_columnCount-1)
    {
        m_currentColumn = 0;
    }
    uasPropertyToLabelMap[value] = item;
    uasEnabledPropertyList.append(value);

    if (!uasPropertyValueMap.contains(value))
    {
        uasPropertyValueMap[value] = 0;
    }
    saveSettings();
    item->show();
    sortItems(m_columnCount);

}
void UASQuickView::sortItems(int columncount)
{
    QList<QWidget*> itemlist;
    for (QMap<QString,UASQuickViewItem*>::const_iterator i = uasPropertyToLabelMap.constBegin();i!=uasPropertyToLabelMap.constEnd();i++)
    {
        m_verticalLayoutList[m_PropertyToLayoutIndexMap[i.key()]]->removeWidget(i.value());
        m_PropertyToLayoutIndexMap.remove(i.key());
        itemlist.append(i.value());
    }
    //Item list has all the widgets availble, now re-add them to the layouts.
    for (int i=0;i<m_verticalLayoutList.size();i++)
    {
        ui.horizontalLayout->removeItem(m_verticalLayoutList[i]);
        m_verticalLayoutList[i]->deleteLater(); //removeItem de-parents the item.
    }
    m_verticalLayoutList.clear();

    //Create a vertical layout for every intended column
    for (int i=0;i<columncount;i++)
    {
        QVBoxLayout *layout = new QVBoxLayout();
        ui.horizontalLayout->addItem(layout);
        m_verticalLayoutList.append(layout);
    }

    //Cycle through all items and add them to the layout
    int currcol = 0;
    for (int i=0;i<itemlist.size();i++)
    {
        m_verticalLayoutList[currcol]->addWidget(itemlist[i]);
        currcol++;
        if (currcol >= columncount)
        {
            currcol = 0;
        }
    }
    m_currentColumn = currcol;
}

void UASQuickView::valueDisabled(QString value)
{
    if (uasPropertyToLabelMap.contains(value))
    {
        UASQuickViewItem *item = uasPropertyToLabelMap[value];
        uasPropertyToLabelMap.remove(value);
        item->hide();
        //ui.verticalLayout->removeWidget(item);
        //layout->removeWidget(item);
        m_verticalLayoutList[m_PropertyToLayoutIndexMap[value]]->removeWidget(item);
        sortItems(m_columnCount);
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
        valueEnabled(senderlabel->text());
        /*UASQuickViewItem *item = new UASQuickViewTextItem(this);
        item->setTitle(senderlabel->text());
        layout->addWidget(item);
        //ui.verticalLayout->addWidget(item);
        m_currentColumn++;
        if (m_currentColumn >= m_verticalLayoutList.size())
        {
            m_currentColumn = 0;
        }
        uasPropertyToLabelMap[senderlabel->text()] = item;*/


    }
    else
    {
        valueDisabled(senderlabel->text());
        /*layout->removeWidget(uasPropertyToLabelMap[senderlabel->text()]);
        uasPropertyToLabelMap[senderlabel->text()]->deleteLater();
        uasPropertyToLabelMap.remove(senderlabel->text());*/

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
