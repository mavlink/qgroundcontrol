#ifndef QGCMISSIONNAVSWEEP_H
#define QGCMISSIONNAVSWEEP_H

#include <QWidget>
#include "WaypointEditableView.h"

namespace Ui {
    class QGCMissionNavSweep;
}

class QGCMissionNavSweep : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionNavSweep(WaypointEditableView* WEV);
    ~QGCMissionNavSweep();

public slots:
    void updateFrame(MAV_FRAME);

protected:
    WaypointEditableView* WEV;

private:
    Ui::QGCMissionNavSweep *ui;
};

#endif // QGCMISSIONNAVSWEEP_H
