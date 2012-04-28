#ifndef QGCMISSIONNAVLOITERTIME_H
#define QGCMISSIONNAVLOITERTIME_H

#include <QWidget>
#include "WaypointEditableView.h"

namespace Ui {
    class QGCMissionNavLoiterTime;
}

class QGCMissionNavLoiterTime : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionNavLoiterTime(WaypointEditableView* WEV);
    ~QGCMissionNavLoiterTime();

public slots:
    void updateFrame(MAV_FRAME);

protected:
    WaypointEditableView* WEV;

private:
    Ui::QGCMissionNavLoiterTime *ui;
};

#endif // QGCMISSIONNAVLOITERTIME_H
