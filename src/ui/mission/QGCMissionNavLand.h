#ifndef QGCMISSIONNAVLAND_H
#define QGCMISSIONNAVLAND_H

#include <QWidget>
#include "WaypointEditableView.h"

namespace Ui {
    class QGCMissionNavLand;
}

class QGCMissionNavLand : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionNavLand(WaypointEditableView* WEV);
    ~QGCMissionNavLand();

public slots:
    void updateFrame(MAV_FRAME);

protected:
    WaypointEditableView* WEV;

private:
    Ui::QGCMissionNavLand *ui;
};

#endif // QGCMISSIONNAVLAND_H
