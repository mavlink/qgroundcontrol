#include "UASQuickView.h"
#include <QMetaMethod>
#include <QDebug>
UASQuickView::UASQuickView(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    connect(UASManager::instance(),SIGNAL(activeUASSet(UASInterface*)),this,SLOT(setActiveUAS(UASInterface*)));
    connect(UASManager::instance(),SIGNAL(UASCreated(UASInterface*)),this,SLOT(addUAS(UASInterface*)));
    if (UASManager::instance()->getActiveUAS())
    {
        addUAS(UASManager::instance()->getActiveUAS());
    }
    this->setContextMenuPolicy(Qt::ActionsContextMenu);


    {
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

    updateTimer = new QTimer(this);
    connect(updateTimer,SIGNAL(timeout()),this,SLOT(updateTimerTick()));
    updateTimer->start(1000);
}
void UASQuickView::updateTimerTick()
{
    for (int i=0;i<uasPropertyList.size();i++)
    {
        if (uasPropertyValueMap.contains(uasPropertyList[i]) && uasPropertyToLabelMap.contains(uasPropertyList[i]))
        {
            uasPropertyToLabelMap[uasPropertyList[i]]->setValue(uasPropertyValueMap.value(uasPropertyList[i],0));
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
    uasPropertyList.clear();
    qDebug() << "UASInfoWidget property count:" << uas->metaObject()->propertyCount();
    for (int i=0;i<uas->metaObject()->propertyCount();i++)
    {
        if (uas->metaObject()->property(i).hasNotifySignal())
        {
            qDebug() << "Property:" << i << uas->metaObject()->property(i).name();
            uasPropertyList.append(uas->metaObject()->property(i).name());
            if (!uasPropertyToLabelMap.contains(uas->metaObject()->property(i).name()))
            {
                QAction *action = new QAction(QString(uas->metaObject()->property(i).name()),this);
                action->setCheckable(true);
                connect(action,SIGNAL(toggled(bool)),this,SLOT(actionTriggered(bool)));
                this->addAction(action);
            }
            qDebug() << "Signature:" << uas->metaObject()->property(i).notifySignal().signature();
            int val = this->metaObject()->indexOfMethod("valChanged(double,QString)");
            if (val != -1)
            {

                if (!connect(uas,uas->metaObject()->property(i).notifySignal(),this,this->metaObject()->method(val)))
                {
                    qDebug() << "Error connecting signal";
                }
            }
        }
    }
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
        UASQuickViewItem *item = new UASQuickViewItem(this);
        item->setTitle(senderlabel->text());
        ui.verticalLayout->addWidget(item);
        uasPropertyToLabelMap[senderlabel->text()] = item;
    }
    else
    {
        ui.verticalLayout->removeWidget(uasPropertyToLabelMap[senderlabel->text()]);
        uasPropertyToLabelMap.remove(senderlabel->text());
        uasPropertyToLabelMap[senderlabel->text()]->deleteLater();
    }
}

void UASQuickView::valChanged(double val,QString type)
{
    //qDebug() << "Value changed:" << type << val;
    uasPropertyValueMap[type] = val;
}
