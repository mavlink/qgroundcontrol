#ifndef QGCMISSIONNAVLOITERUNLIM_H
#define QGCMISSIONNAVLOITERUNLIM_H

#include <QWidget>
#include "WaypointEditableView.h"

namespace Ui {
    class QGCMissionNavLoiterUnlim;
}

class QGCMissionNavLoiterUnlim : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionNavLoiterUnlim(WaypointEditableView* WEV);
    ~QGCMissionNavLoiterUnlim();

public slots:
    void updateFrame(MAV_FRAME);

protected:
    WaypointEditableView* WEV;

private:
    Ui::QGCMissionNavLoiterUnlim *ui;
};

#endif // QGCMISSIONNAVLOITERUNLIM_H
