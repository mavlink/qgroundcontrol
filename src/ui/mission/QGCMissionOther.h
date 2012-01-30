#ifndef QGCMISSIONOTHER_H
#define QGCMISSIONOTHER_H

#include <QWidget>
#include "WaypointEditableView.h"

namespace Ui {
    class QGCMissionOther;
}

class QGCMissionOther : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionOther(WaypointEditableView* WEV);
    ~QGCMissionOther();
    Ui::QGCMissionOther *ui;

protected:
    WaypointEditableView* WEV;

private:
};

#endif // QGCMISSIONOTHER_H
