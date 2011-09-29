#ifndef WAYPOINTVIEWVIEWONLY_H
#define WAYPOINTVIEWVIEWONLY_H

#include <QtGui/QWidget>
#include "Waypoint.h"
#include <iostream>

namespace Ui {
    class WaypointViewViewOnly;
}

class WaypointViewViewOnly : public QWidget
{
    Q_OBJECT

public:
    explicit WaypointViewViewOnly(Waypoint* wp, QWidget *parent = 0);
    ~WaypointViewViewOnly();

public slots:
void changedCurrent(int state);
void changedAutoContinue(int state);
void updateValues(void);
void setCurrent(bool state);

signals:
    void changeCurrentWaypoint(quint16);
    void changeAutoContinue(quint16, bool);

protected:
Waypoint* wp;

private:
    Ui::WaypointViewViewOnly *m_ui;
};

#endif // WAYPOINTVIEWVIEWONLY_H
