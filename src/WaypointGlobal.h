#ifndef WAYPOINTGLOBAL_H
#define WAYPOINTGLOBAL_H

#include "Waypoint.h"
#include <QPointF>

class WaypointGlobal: public Waypoint {
    Q_OBJECT

public:
    WaypointGlobal(const QPointF coordinate);

    public slots:

//    void set_latitud(double latitud);
//    void set_longitud(double longitud);
//    double get_latitud();
//    double get_longitud();

private:
    QPointF coordinateWP;





};

#endif // WAYPOINTGLOBAL_H
