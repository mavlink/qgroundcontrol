#ifndef QGCMISSIONDOSTARTSEARCH_H
#define QGCMISSIONDOSTARTSEARCH_H

#include <QWidget>
#include "WaypointEditableView.h"

namespace Ui {
    class QGCMissionDoStartSearch;
}

class QGCMissionDoStartSearch : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionDoStartSearch(WaypointEditableView* WEV);
    ~QGCMissionDoStartSearch();

protected:
    WaypointEditableView* WEV;

private:
    Ui::QGCMissionDoStartSearch *ui;
};

#endif // QGCMISSIONDOSTARTSEARCH_H
