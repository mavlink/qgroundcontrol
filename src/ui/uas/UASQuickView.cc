#include "UASQuickView.h"
#include "UASQuickViewItemSelect.h"
#include "UASQuickViewTextItem.h"
#include <QMetaMethod>
#include <QSettings>
#include <QInputDialog>
UASQuickView::UASQuickView(QWidget *parent) : QWidget(parent)
{
    quickViewSelectDialog=0;
    m_columnCount=2;
    m_currentColumn=0;
    ui.setupUi(this);

    ui.horizontalLayout->setMargin(0);
    //ui.horizontalLayout->setSpacing(0);
    m_verticalLayoutList.append(new QVBoxLayout());
    m_verticalLayoutList[0]->setMargin(0);
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
    m_columnCount = 2;
    if (uasPropertyValueMap.size() == 0)
    {
        valueEnabled("altitudeAMSL");
        valueEnabled("altitudeRelative");
        valueEnabled("groundSpeed");
        valueEnabled("distToWaypoint");
    }

    QAction *action = new QAction("Add/Remove Items",this);
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
void UASQuickView::replaceSingleItem(QString olditem)
{
    UASQuickViewItemSelect *itemSelect = new UASQuickViewItemSelect(true);
    itemSelect->setAttribute(Qt::WA_DeleteOnClose,true);
    connect(itemSelect,SIGNAL(valueSwapped(QString,QString)),this,SLOT(replaceSingleItemSelected(QString,QString)));
    for (QMap<QString,double>::const_iterator i = uasPropertyValueMap.constBegin();i!=uasPropertyValueMap.constEnd();i++)
    {
        if (i.key().contains(olditem))
        {
            itemSelect->addItem(i.key(),true);
        }
        else
        {
            itemSelect->addItem(i.key(),false);
        }
    }
    itemSelect->show();
}
void UASQuickView::replaceSingleItemSelected(QString newitem,QString olditem)
{
    if (uasPropertyToLabelMap.contains(newitem) && uasPropertyToLabelMap.contains(olditem))
    {
        //Newitem exists, swap the two items.
        UASQuickViewItem *olditemptr = uasPropertyToLabelMap[olditem];
        UASQuickViewItem *newitemptr = uasPropertyToLabelMap[newitem];
        int oldindex = m_PropertyToLayoutIndexMap[olditem];
        int newindex = m_PropertyToLayoutIndexMap[newitem];

        uasPropertyToLabelMap.remove(olditem);
        uasPropertyToLabelMap.remove(newitem);
        uasPropertyToLabelMap[olditem] = newitemptr;
        uasPropertyToLabelMap[newitem] = olditemptr;


        uasPropertyValueMap.remove(olditem);
        uasPropertyValueMap.remove(newitem);
        uasPropertyValueMap[newitem] = 0;
        uasPropertyValueMap[olditem] = 0;
        olditemptr->setTitle(newitem);
        newitemptr->setTitle(olditem);

        m_PropertyToLayoutIndexMap.remove(olditem);
        m_PropertyToLayoutIndexMap.remove(newitem);
        m_PropertyToLayoutIndexMap[newitem] = oldindex;
        m_PropertyToLayoutIndexMap[olditem] = newindex;

        saveSettings();
    }
    else if (uasPropertyToLabelMap.contains(olditem))
    {
        UASQuickViewItem *item = uasPropertyToLabelMap[olditem];
        uasPropertyToLabelMap.remove(olditem);
        uasPropertyToLabelMap[newitem] = item;
        uasPropertyValueMap.remove(olditem);
        uasPropertyValueMap[newitem] = 0;
        item->setTitle(newitem);
        int index = m_PropertyToLayoutIndexMap[olditem];
        m_PropertyToLayoutIndexMap.remove(olditem);
        m_PropertyToLayoutIndexMap[newitem] = index;

        uasEnabledPropertyList.removeOne(olditem);
        uasEnabledPropertyList.append(newitem);
        saveSettings();
    }

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
    m_columnCount = settings.value("UAS_QUICK_VIEW_COLUMNS",1).toInt();
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
    sortItems(m_columnCount);
}

void UASQuickView::valueEnabled(QString value)
{
    UASQuickViewItem *item = new UASQuickViewTextItem(this);
    connect(item,SIGNAL(showSelectDialog(QString)),this,SLOT(replaceSingleItem(QString)));
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
    for (int i=0;i<uasEnabledPropertyList.size();i++)
    {
        m_verticalLayoutList[m_PropertyToLayoutIndexMap[uasEnabledPropertyList[i]]]->removeWidget(uasPropertyToLabelMap[uasEnabledPropertyList[i]]);
        m_PropertyToLayoutIndexMap.remove(uasEnabledPropertyList[i]);
        itemlist.append(uasPropertyToLabelMap[uasEnabledPropertyList[i]]);
    }
    /*for (QMap<QString,UASQuickViewItem*>::const_iterator i = uasPropertyToLabelMap.constBegin();i!=uasPropertyToLabelMap.constEnd();i++)
    {
        m_verticalLayoutList[m_PropertyToLayoutIndexMap[i.key()]]->removeWidget(i.value());
        m_PropertyToLayoutIndexMap.remove(i.key());
        itemlist.append(i.value());
    }*/
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
        layout->setMargin(0);
        layout->setSpacing(0);
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
//    QApplication::processEvents();
    recalculateItemTextSizing();
}
void UASQuickView::resizeEvent(QResizeEvent *evt)
{
    Q_UNUSED(evt);
    recalculateItemTextSizing();
}
void UASQuickView::recalculateItemTextSizing()
{
    int minpixelsize = 65535;
    for (QMap<QString,UASQuickViewItem*>::const_iterator i = uasPropertyToLabelMap.constBegin();i!=uasPropertyToLabelMap.constEnd();i++)
    {
        int tempmin = i.value()->minValuePixelSize();
        if (tempmin < minpixelsize)
        {
            minpixelsize = tempmin;
        }
    }
    for (QMap<QString,UASQuickViewItem*>::const_iterator i = uasPropertyToLabelMap.constBegin();i!=uasPropertyToLabelMap.constEnd();i++)
    {
        i.value()->setValuePixelSize(minpixelsize);
    }
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
        uasEnabledPropertyList.removeOne(value);
        sortItems(m_columnCount);
        item->deleteLater();
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
    connect(uas,SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)),this,
            SLOT(valueChanged(int,QString,QString,QVariant,quint64)));

}
void UASQuickView::addSource(MAVLinkDecoder *decoder)
{
    Q_UNUSED(decoder);
    //connect(decoder,SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)),this,SLOT(valueChanged(int,QString,QString,QVariant,quint64)));
}
void UASQuickView::valueChanged(const int uasId, const QString& name, const QString& unit, const QVariant& value, const quint64 msec)
{
    Q_UNUSED(msec);

    if (this->uas->getUASID() != uasId)
    {
        //This message is for the non active UAS
        return;
    }
    QString propername = name.mid(name.indexOf(":")+1);
    if (!uasPropertyValueMap.contains(propername +" ("+unit+")"))
    {
        if (quickViewSelectDialog)
        {
            quickViewSelectDialog->addItem(propername +" ("+unit+")");
        }
    }
    bool ok = false;
    uasPropertyValueMap[propername +" ("+unit+")"] = value.toDouble(&ok);
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
    }
    else
    {
        valueDisabled(senderlabel->text());
    }
}

void UASQuickView::valChanged(double val,QString type)
{
     Q_UNUSED(val);
     Q_UNUSED(type);
   // uasPropertyValueMap[type] = val;
}
