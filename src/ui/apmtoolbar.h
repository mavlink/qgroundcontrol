#ifndef APMTOOLBAR_H
#define APMTOOLBAR_H

#include <QAction>
#include <QDeclarativeView>
#include "UASInterface.h"

class LinkInterface;

class APMToolBar : public QDeclarativeView
{
    Q_OBJECT
public:
    explicit APMToolBar(QWidget *parent = 0);
    ~APMToolBar();

    void setFlightViewAction(QAction *action);
    void setFlightPlanViewAction(QAction *action);
    void setHardwareViewAction(QAction *action);
    void setSoftwareViewAction(QAction *action);
    void setSimulationViewAction(QAction *action);
    void setTerminalViewAction(QAction *action);
    void setConnectMAVAction(QAction *action);
    
signals:
    void triggerFlightView();
    void triggerFlightPlanView();
    void triggerHardwareView();
    void triggerSoftwareView();
    void triggerSimulationView();
    void triggerTerminalView();

    void MAVConnected(bool connected);

public slots:
    void armingChanged(bool armed);
    void activeUasSet(UASInterface *uas);
    void selectFlightView();
    void selectFlightPlanView();
    void selectHardwareView();
    void selectSoftwareView();
    void selectSimulationView();
    void selectTerminalView();

    void connectMAV();
    void showConnectionDialog();
    void setConnection(bool connection);

    void updateLinkDisplay(LinkInterface *newLink);

private:
    UASInterface *m_uas;
};

#endif // APMTOOLBAR_H
