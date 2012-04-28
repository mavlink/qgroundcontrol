#ifndef QGCMISSIONNAVWAYPOINT_H
#define QGCMISSIONNAVWAYPOINT_H

#include <QWidget>
#include "WaypointEditableView.h"

namespace Ui {
    class QGCMissionNavWaypoint;
}

class QGCMissionNavWaypoint : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionNavWaypoint(WaypointEditableView* WEV);
    ~QGCMissionNavWaypoint();

public slots:
    void updateFrame(MAV_FRAME);

protected:
    WaypointEditableView* WEV;

private:
    Ui::QGCMissionNavWaypoint *ui;
};

#endif // QGCMISSIONNAVWAYPOINT_H
