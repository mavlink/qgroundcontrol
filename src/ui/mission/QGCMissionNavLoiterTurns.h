#ifndef QGCMISSIONNAVLOITERTURNS_H
#define QGCMISSIONNAVLOITERTURNS_H

#include <QWidget>
#include "WaypointEditableView.h"

namespace Ui {
    class QGCMissionNavLoiterTurns;
}

class QGCMissionNavLoiterTurns : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionNavLoiterTurns(WaypointEditableView* WEV);
    ~QGCMissionNavLoiterTurns();

public slots:
    void updateFrame(MAV_FRAME);

protected:
    WaypointEditableView* WEV;

private:
    Ui::QGCMissionNavLoiterTurns *ui;
};

#endif // QGCMISSIONNAVLOITERTURNS_H
