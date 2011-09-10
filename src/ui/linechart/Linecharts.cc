#include <QShowEvent>

#include "Linecharts.h"
#include "UASManager.h"

#include "MainWindow.h"

Linecharts::Linecharts(QWidget *parent) :
    QStackedWidget(parent),
    plots(),
    active(true)
{
    this->setVisible(false);
    // Get current MAV list
    QList<UASInterface*> systems = UASManager::instance()->getUASList();

    // Add each of them
    foreach (UASInterface* sys, systems) {
        addSystem(sys);
    }
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)),
            this, SLOT(addSystem(UASInterface*)));
    connect(UASManager::instance(), SIGNAL(activeUASSet(int)),
            this, SLOT(selectSystem(int)));
    connect(this, SIGNAL(logfileWritten(QString)),
            MainWindow::instance(), SLOT(loadDataView(QString)));
}

void Linecharts::showEvent(QShowEvent* event)
{
    // React only to internal (pre-display)
    // events
    Q_UNUSED(event)
    QWidget* prevWidget = currentWidget();
    if (prevWidget)
    {
        LinechartWidget* chart = dynamic_cast<LinechartWidget*>(prevWidget);
        if (chart) {
            this->active = true;
            chart->setActive(true);
        }
    }
    QWidget::showEvent(event);
    emit visibilityChanged(true);
}

void Linecharts::hideEvent(QHideEvent* event)
{
    // React only to internal (pre-display)
    // events
    Q_UNUSED(event)
    QWidget* prevWidget = currentWidget();
    if (prevWidget) {
        LinechartWidget* chart = dynamic_cast<LinechartWidget*>(prevWidget);
        if (chart) {
            this->active = false;
            chart->setActive(false);
        }
    }
    QWidget::hideEvent(event);
    emit visibilityChanged(false);
}

void Linecharts::selectSystem(int systemid)
{
    QWidget* prevWidget = currentWidget();
    if (prevWidget)
    {
        LinechartWidget* chart = dynamic_cast<LinechartWidget*>(prevWidget);
        if (chart)
        {
            chart->setActive(false);
            chart->setActiveSystem(systemid);
        }
    }
    QWidget* widget = plots.value(systemid, NULL);
    if (widget)
    {
        setCurrentWidget(widget);
        LinechartWidget* chart = dynamic_cast<LinechartWidget*>(widget);
        if (chart)
        {
            chart->setActive(true);
            chart->setActiveSystem(systemid);
        }
    }
}

void Linecharts::addSystem(UASInterface* uas)
{
    if (!plots.contains(uas->getUASID()))
    {
        LinechartWidget* widget = new LinechartWidget(uas->getUASID(), this);
        addWidget(widget);
        plots.insert(uas->getUASID(), widget);
        // Values without unit
        //connect(uas, SIGNAL(valueChanged(int,QString,double,quint64)), widget, SLOT(appendData(int,QString,double,quint64)));
        // Values with unit as double
        connect(uas, SIGNAL(valueChanged(int,QString,QString,double,quint64)), widget, SLOT(appendData(int,QString,QString,double,quint64)));
        // Values with unit as integer
        connect(uas, SIGNAL(valueChanged(int,QString,QString,int,quint64)), widget, SLOT(appendData(int,QString,QString,int,quint64)));

        connect(widget, SIGNAL(logfileWritten(QString)), this, SIGNAL(logfileWritten(QString)));
        // Set system active if this is the only system
        if (active)
        {
            if (plots.size() == 1)
            {
                // FIXME XXX HACK
                // Connect generic sources
                for (int i = 0; i < genericSources.count(); ++i)
                {
                    connect(genericSources[i], SIGNAL(valueChanged(int,QString,QString,int,quint64)), plots.values().first(), SLOT(appendData(int,QString,QString,int,quint64)));
                    connect(genericSources[i], SIGNAL(valueChanged(int,QString,QString,unsigned int,quint64)), plots.values().first(), SLOT(appendData(int,QString,QString,unsigned int,quint64)));
                    connect(genericSources[i], SIGNAL(valueChanged(int,QString,QString,quint64,quint64)), plots.values().first(), SLOT(appendData(int,QString,QString,quint64,quint64)));
                    connect(genericSources[i], SIGNAL(valueChanged(int,QString,QString,qint64,quint64)), plots.values().first(), SLOT(appendData(int,QString,QString,qint64,quint64)));
                    connect(genericSources[i], SIGNAL(valueChanged(int,QString,QString,double,quint64)), plots.values().first(), SLOT(appendData(int,QString,QString,double,quint64)));
                }
                // Select system
                selectSystem(uas->getUASID());
            }
        }
    }
}

void Linecharts::addSource(QObject* obj)
{
    genericSources.append(obj);
    // FIXME XXX HACK
    if (plots.size() > 0)
    {
        // Connect generic source
        connect(obj, SIGNAL(valueChanged(int,QString,QString,int,quint64)), plots.values().first(), SLOT(appendData(int,QString,QString,int,quint64)));
        connect(obj, SIGNAL(valueChanged(int,QString,QString,unsigned int,quint64)), plots.values().first(), SLOT(appendData(int,QString,QString,unsigned int,quint64)));
        connect(obj, SIGNAL(valueChanged(int,QString,QString,quint64,quint64)), plots.values().first(), SLOT(appendData(int,QString,QString,quint64,quint64)));
        connect(obj, SIGNAL(valueChanged(int,QString,QString,qint64,quint64)), plots.values().first(), SLOT(appendData(int,QString,QString,qint64,quint64)));
        connect(obj, SIGNAL(valueChanged(int,QString,QString,double,quint64)), plots.values().first(), SLOT(appendData(int,QString,QString,double,quint64)));
    }
}
