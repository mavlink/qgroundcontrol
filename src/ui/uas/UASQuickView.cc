#include "UASQuickView.h"
#include "UASQuickViewItemSelect.h"
#include "UASQuickViewTextItem.h"
#include "MultiVehicleManager.h"
#include "UAS.h"
#include "QGCApplication.h"

#include <QMetaMethod>
#include <QDebug>
#include <QSettings>
#include <QInputDialog>

UASQuickView::UASQuickView(QWidget *parent) : QWidget(parent),
    uas(NULL)
{
    quickViewSelectDialog=0;
    m_columnCount=2;
    m_currentColumn=0;
    ui.setupUi(this);

    ui.horizontalLayout->setMargin(0);
    m_verticalLayoutList.append(new QVBoxLayout());
    ui.horizontalLayout->addItem(m_verticalLayoutList[0]);

    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &UASQuickView::_activeVehicleChanged);
    _activeVehicleChanged(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle());
    this->setContextMenuPolicy(Qt::ActionsContextMenu);

    loadSettings();

    //If we don't have any predefined settings, set some defaults.
    if (uasPropertyValueMap.size() == 0)
    {
        valueEnabled("altitudeAMSL");
        valueEnabled("altitudeAMSLFT");
        valueEnabled("altitudeRelative");
        valueEnabled("groundSpeed");
        valueEnabled("distToWaypoint");
    }

    QAction *action = new QAction("Add/Remove Items",this);
    action->setCheckable(false);
    connect(action,&QAction::triggered,this, &UASQuickView::addActionTriggered);
    this->addAction(action);

    QAction *columnaction = new QAction("Set Column Count",this);
    columnaction->setCheckable(false);
    connect(columnaction,&QAction::triggered,this,&UASQuickView::columnActionTriggered);
    this->addAction(columnaction);

    updateTimer = new QTimer(this);
    connect(updateTimer,&QTimer::timeout,this,&UASQuickView::updateTimerTick);
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
    int newcolumns = QInputDialog::getInt(
        this,"Columns","Enter number of columns", m_columnCount, 1, 10, 1, &ok);
    if (!ok)
    {
        return;
    }
    m_columnCount = newcolumns;
    sortItems(newcolumns);
    saveSettings();
}

void UASQuickView::addActionTriggered()
{
    if (quickViewSelectDialog)
    {
        quickViewSelectDialog->show();
        return;
    }
    quickViewSelectDialog = new UASQuickViewItemSelect();
    connect(quickViewSelectDialog,&UASQuickViewItemSelect::destroyed,this,&UASQuickView::selectDialogClosed);
    connect(quickViewSelectDialog,&UASQuickViewItemSelect::valueDisabled,this,&UASQuickView::valueDisabled);
    connect(quickViewSelectDialog,&UASQuickViewItemSelect::valueEnabled,this,&UASQuickView::valueEnabled);

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
}
void UASQuickView::loadSettings()
{
    QSettings settings;
    m_columnCount = settings.value("UAS_QUICK_VIEW_COLUMNS",1).toInt();
    int size = settings.beginReadArray("UAS_QUICK_VIEW_ITEMS");
    for (int i = 0; i < size; i++)
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
    // Item list has all the widgets available, now re-add them to the layouts.
    for (int i = 0; i < m_verticalLayoutList.size(); i++)
    {
        ui.horizontalLayout->removeItem(m_verticalLayoutList[i]);
        m_verticalLayoutList[i]->deleteLater(); //removeItem de-parents the item.
    }
    m_verticalLayoutList.clear();

    // Create a vertical layout for every intended column
    for (int i = 0; i < columncount; i++)
    {
        QVBoxLayout *layout = new QVBoxLayout();
        ui.horizontalLayout->addItem(layout);
        m_verticalLayoutList.append(layout);
        layout->setMargin(0);
    }

    //Cycle through all items and add them to the layout
    int currcol = 0;
    for (int i = 0; i < itemlist.size(); i++)
    {
        m_verticalLayoutList[currcol]->addWidget(itemlist[i]);
        currcol++;
        if (currcol >= columncount)
        {
            currcol = 0;
        }
    }
    m_currentColumn = currcol;
    QApplication::processEvents();
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
    if(minpixelsize < 6)
        minpixelsize = 6;
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

void UASQuickView::_activeVehicleChanged(Vehicle* vehicle)
{
    if (uas || !vehicle) {
        return;
    }
    this->uas = vehicle->uas();
    connect(uas, SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)),this,SLOT(valueChanged(int,QString,QString,QVariant,quint64)));
}
void UASQuickView::addSource(MAVLinkDecoder *decoder)
{
    connect(decoder,SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)),this,SLOT(valueChanged(int,QString,QString,QVariant,quint64)));
}

void UASQuickView::valueChanged(const int uasId, const QString& name, const QString& unit, const QVariant &variant, const quint64 msec)
{
    Q_UNUSED(uasId);
    Q_UNUSED(unit);
    Q_UNUSED(msec);
    
    bool ok;
    double value = variant.toDouble(&ok);
    QMetaType::Type metaType = static_cast<QMetaType::Type>(variant.type());
    if(!ok || metaType == QMetaType::QString || metaType == QMetaType::QByteArray)
        return;

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
