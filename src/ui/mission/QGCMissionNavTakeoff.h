#ifndef QGCMISSIONNAVTAKEOFF_H
#define QGCMISSIONNAVTAKEOFF_H

#include <QWidget>
#include "WaypointEditableView.h"

namespace Ui {
    class QGCMissionNavTakeoff;
}

class QGCMissionNavTakeoff : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionNavTakeoff(WaypointEditableView* WEV);
    ~QGCMissionNavTakeoff();

public slots:
    void updateFrame(MAV_FRAME);

protected:
    WaypointEditableView* WEV;

private:
    Ui::QGCMissionNavTakeoff *ui;
};

#endif // QGCMISSIONNAVTAKEOFF_H
