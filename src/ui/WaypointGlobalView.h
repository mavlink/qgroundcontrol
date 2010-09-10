#ifndef WAYPOINTGLOBALVIEW_H
#define WAYPOINTGLOBALVIEW_H

#include <QWidget>
#include "Waypoint.h"

namespace Ui {
    class WaypointGlobalView;
}

class WaypointGlobalView : public QWidget
{
    Q_OBJECT

public:
    explicit WaypointGlobalView(Waypoint* wp, QWidget *parent = 0);
    ~WaypointGlobalView();

public slots:

    void updateValues(void);
    void remove();
    QString getLatitudString(float lat);
    QString getLongitudString(float lon);
     void changeOrbitalState(int state);


signals:

    void removeWaypoint(Waypoint*);

protected:
    virtual void changeEvent(QEvent *e);

    Waypoint* wp;

private:
    Ui::WaypointGlobalView *ui;

private slots:

};

#endif // WAYPOINTGLOBALVIEW_H
