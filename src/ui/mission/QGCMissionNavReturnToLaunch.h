#ifndef QGCMISSIONNAVRETURNTOLAUNCH_H
#define QGCMISSIONNAVRETURNTOLAUNCH_H

#include <QWidget>
#include "WaypointEditableView.h"

namespace Ui {
    class QGCMissionNavReturnToLaunch;
}

class QGCMissionNavReturnToLaunch : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionNavReturnToLaunch(WaypointEditableView* WEV);
    ~QGCMissionNavReturnToLaunch();

protected:
    WaypointEditableView* WEV;

private:
    Ui::QGCMissionNavReturnToLaunch *ui;
};

#endif // QGCMISSIONNAVRETURNTOLAUNCH_H
