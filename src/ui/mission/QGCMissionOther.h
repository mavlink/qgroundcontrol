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

protected:
    WaypointEditableView* WEV;

private:
    Ui::QGCMissionOther *ui;
};

#endif // QGCMISSIONOTHER_H
