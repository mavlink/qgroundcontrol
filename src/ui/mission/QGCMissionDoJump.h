#ifndef QGCMISSIONDOJUMP_H
#define QGCMISSIONDOJUMP_H

#include <QWidget>
#include "WaypointEditableView.h"

namespace Ui {
    class QGCMissionDoJump;
}

class QGCMissionDoJump : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionDoJump(WaypointEditableView* WEV);
    ~QGCMissionDoJump();

protected:
    WaypointEditableView* WEV;

private:
    Ui::QGCMissionDoJump *ui;
};

#endif // QGCMISSIONDOJUMP_H
