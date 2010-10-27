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
    void getLatitudeGradoMin(float lat, int *gradoLat, float *minLat, QString *dirLat);
    void getLongitudGradoMin(float lon, int *gradoLon, float *minLon, QString *dirLon);
     void changeOrbitalState(int state);
     void updateCoordValues(float lat, float lon);



    //update latitude
     void updateLatitudeWP(int value);
     void updateLatitudeMinuteWP(double value);
     void changeDirectionLatitudeWP();

     //update longitude
     void updateLongitudeWP(int value);
     void updateLongitudeMinuteWP(double value);
     void changeDirectionLongitudeWP();



signals:

    void removeWaypoint(Waypoint*);
    void changePositionWP(Waypoint*);


protected:
    virtual void changeEvent(QEvent *e);

    Waypoint* wp;

private:
    Ui::WaypointGlobalView *ui;

private slots:

};

#endif // WAYPOINTGLOBALVIEW_H
