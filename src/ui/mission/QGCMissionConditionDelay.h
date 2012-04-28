#ifndef QGCMISSIONCONDITIONDELAY_H
#define QGCMISSIONCONDITIONDELAY_H

#include <QWidget>
#include "WaypointEditableView.h"

namespace Ui {
    class QGCMissionConditionDelay;
}

class QGCMissionConditionDelay : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionConditionDelay(WaypointEditableView* WEV);
    ~QGCMissionConditionDelay();

protected:
    WaypointEditableView* WEV;

private:
    Ui::QGCMissionConditionDelay *ui;
};

#endif // QGCMISSIONCONDITIONDELAY_H
