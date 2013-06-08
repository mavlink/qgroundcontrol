#include "UASQuickView.h"
#include <QMetaMethod>
#include <QDebug>
UASQuickView::UASQuickView(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::UASQuickView)
{
    m_ui->setupUi(this);

    connect(UASManager::instance(),SIGNAL(activeUASSet(UASInterface*)),this,SLOT(setActiveUAS(UASInterface*)));
    connect(UASManager::instance(),SIGNAL(UASCreated(UASInterface*)),this,SLOT(addUAS(UASInterface*)));
    if (UASManager::instance()->getActiveUAS())
    {
        addUAS(UASManager::instance()->getActiveUAS());
    }
    this->setContextMenuPolicy(Qt::ActionsContextMenu);

    addDefaultActions();

    updateTimer = new QTimer(this);
    connect(updateTimer,SIGNAL(timeout()),this,SLOT(updateTimerTick()));
}

void UASQuickView::addDefaultActions()
{
    {
        QAction *action = new QAction(tr("latitude"),this);
        action->setCheckable(true);
        action->setChecked(true);
        connect(action,SIGNAL(toggled(bool)),this,SLOT(actionTriggered(bool)));
        this->addAction(action);
        UASQuickViewItem *item = new UASQuickViewItem(this);
        item->setTitle(tr("latitude"));
        this->layout()->addWidget(item);
        uasPropertyToLabelMap["latitude"] = item;
    }

    {
        QAction *action = new QAction(tr("longitude"),this);
        action->setCheckable(true);
        action->setChecked(true);
        connect(action,SIGNAL(toggled(bool)),this,SLOT(actionTriggered(bool)));
        this->addAction(action);
        UASQuickViewItem *item = new UASQuickViewItem(this);
        item->setTitle(tr("longitude"));
        this->layout()->addWidget(item);
        uasPropertyToLabelMap["longitude"] = item;
    }

    {
        QAction *action = new QAction(tr("altitude"),this);
        action->setCheckable(true);
        action->setChecked(true);
        connect(action,SIGNAL(toggled(bool)),this,SLOT(actionTriggered(bool)));
        this->addAction(action);
        UASQuickViewItem *item = new UASQuickViewItem(this);
        item->setTitle(tr("altitude"));
        this->layout()->addWidget(item);
        uasPropertyToLabelMap["altitude"] = item;
    }

    {
        QAction *action = new QAction(tr("satelliteCount"),this);
        action->setCheckable(true);
        action->setChecked(true);
        connect(action,SIGNAL(toggled(bool)),this,SLOT(actionTriggered(bool)));
        this->addAction(action);
        UASQuickViewItem *item = new UASQuickViewItem(this);
        item->setTitle(tr("satelliteCount"));
        this->layout()->addWidget(item);
        uasPropertyToLabelMap["satelliteCount"] = item;
    }

    {
        QAction *action = new QAction(tr("distToWaypoint"),this);
        action->setCheckable(true);
        action->setChecked(true);
        connect(action,SIGNAL(toggled(bool)),this,SLOT(actionTriggered(bool)));
        this->addAction(action);
        UASQuickViewItem *item = new UASQuickViewItem(this);
        item->setTitle(tr("distToWaypoint"));
        this->layout()->addWidget(item);
        uasPropertyToLabelMap["distToWaypoint"] = item;
    }
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
    // Clean up from the old UAS
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

    // And re-add the default actions.
    addDefaultActions();

    // And connect the new one if it exists.
    if (this->uas)
    {
        // Monitor new UAS for changes
        connect(this->uas,SIGNAL(valueChanged(int,QString,QString,QVariant,quint64)),this,SLOT(valueChanged(int,QString,QString,QVariant,quint64)));

        // Populate a right-click menu for selecting which properties to display
        qDebug() << "UASInfoWidget property count:" << uas->metaObject()->propertyCount();
        for (int i=0;i<this->uas->metaObject()->propertyCount();i++)
        {
            if (this->uas->metaObject()->property(i).hasNotifySignal())
            {
                qDebug() << "Property:" << i << this->uas->metaObject()->property(i).name();
                uasPropertyList.append(this->uas->metaObject()->property(i).name());
                if (!uasPropertyToLabelMap.contains(this->uas->metaObject()->property(i).name()))
                {
                    QAction *action = new QAction(QString(this->uas->metaObject()->property(i).name()),this);
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

        // And periodically update the view.
        updateTimer->start(1000);
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
    //qDebug() << "Value changed:" << type << val;
   // uasPropertyValueMap[type] = val;
}
