#ifndef WAYPOINTVIEWONLYVIEW_H
#define WAYPOINTVIEWONLYVIEW_H

#include <QtGui/QWidget>
#include "Waypoint.h"
#include <iostream>

namespace Ui {
    class WaypointViewOnlyView;
}

class WaypointViewOnlyView : public QWidget
{
    Q_OBJECT

public:
    explicit WaypointViewOnlyView(Waypoint* wp, QWidget *parent = 0);
    ~WaypointViewOnlyView();

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
    void hightlightDesiredCurrent(bool hightlight_on);
    Ui::WaypointViewOnlyView *m_ui;
};

#endif // WAYPOINTVIEWONLYVIEW_H
