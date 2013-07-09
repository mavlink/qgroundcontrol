#ifndef APMTOOLBAR_H
#define APMTOOLBAR_H

#include <QAction>
#include <QDeclarativeView>

class APMToolBar : public QDeclarativeView
{
    Q_OBJECT
public:
    explicit APMToolBar(QWidget *parent = 0);

    void setFlightViewAction(QAction *action);
    void setFlightPlanViewAction(QAction *action);
    void setHardwareViewAction(QAction *action);
    void setSoftwareViewAction(QAction *action);
    void setSimulationViewAction(QAction *action);
    void setTerminalViewAction(QAction *action);
    
signals:
    void triggerFlightView();
    void triggerFlightPlanView();
    void triggerHardwareView();
    void triggerSoftwareView();
    void triggerSimulationView();
    void triggerTerminalView();

public slots:
//signals:
    void selectFlightView();
    void selectFlightPlanView();
    void selectHardwareView();
    void selectSoftwareView();
    void selectSimulationView();
    void selectTerminalView();

public slots:
    void connectMAV();
};

#endif // APMTOOLBAR_H
