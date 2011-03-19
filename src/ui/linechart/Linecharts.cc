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
    foreach (UASInterface* sys, systems)
    {
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
    {
        QWidget* prevWidget = currentWidget();
        if (prevWidget)
        {
            LinechartWidget* chart = dynamic_cast<LinechartWidget*>(prevWidget);
            if (chart)
            {
                this->active = true;
                chart->setActive(true);
            }
        }
    }
}

void Linecharts::hideEvent(QHideEvent* event)
{
    // React only to internal (pre-display)
    // events
    Q_UNUSED(event)
    {
        QWidget* prevWidget = currentWidget();
        if (prevWidget)
        {
            LinechartWidget* chart = dynamic_cast<LinechartWidget*>(prevWidget);
            if (chart)
            {
                this->active = false;
                chart->setActive(false);
            }
        }
    }
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
                selectSystem(uas->getUASID());
            }
        }
    }
}
