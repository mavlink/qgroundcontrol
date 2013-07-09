#include <QDebug>
#include <QDeclarativeContext>
#include <QGraphicsObject>

#include "apmtoolbar.h"

APMToolBar::APMToolBar(QWidget *parent) :
    QDeclarativeView(parent)
{
    // Configure our QML object
    this->rootContext()->setContextProperty("globalObj", this);
    setSource(QUrl::fromLocalFile("qml/ApmToolBar.qml"));
    setResizeMode(QDeclarativeView::SizeRootObjectToView);
}

void APMToolBar::setFlightViewAction(QAction *action)
{
    connect(this, SIGNAL(selectFlightView()), action, SIGNAL(triggered()));
}

void APMToolBar::setFlightPlanViewAction(QAction *action)
{
    connect(this, SIGNAL(selectFlightPlanView()), action, SIGNAL(triggered()));
}

void APMToolBar::setHardwareViewAction(QAction *action)
{
    connect(this, SIGNAL(selectHardwareView()), action, SIGNAL(triggered()));
}

void APMToolBar::setSoftwareViewAction(QAction *action)
{
    connect(this, SIGNAL(selectSoftwareView()), action, SIGNAL(triggered()));
}

void APMToolBar::setSimulationViewAction(QAction *action)
{
    connect(this, SIGNAL(selectSimualtionView()), action, SIGNAL(triggered()));
}

void APMToolBar::setTerminalViewAction(QAction *action)
{
    connect(this, SIGNAL(selectTerminalView()), action, SIGNAL(triggered()));
}

void APMToolBar::selectFlightView()
{
    qDebug() << "APMToolBar: SelectFlightView";
//    emit triggerFlightView();
}

void APMToolBar::selectFlightPlanView()
{
    qDebug() << "APMToolBar: SelectFlightPlanView";
}

void APMToolBar::selectHardwareView()
{
    qDebug() << "APMToolBar: selectHardwareView";
}

void APMToolBar::selectSoftwareView()
{
    qDebug() << "APMToolBar: selectSoftwareView";
}

void APMToolBar::selectSimulationView()
{
    qDebug() << "APMToolBar: selectSimulationView";
}

void APMToolBar::selectTerminalView()
{
    qDebug() << "APMToolBar: selectTerminalView";
}

void APMToolBar::connectMAV()
{
    qDebug() << "APMToolBar: connect";
}

