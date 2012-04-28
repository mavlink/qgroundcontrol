#ifndef QGCMISSIONDOFINISHSEARCH_H
#define QGCMISSIONDOFINISHSEARCH_H

#include <QWidget>
#include "WaypointEditableView.h"
namespace Ui {
    class QGCMissionDoFinishSearch;
}

class QGCMissionDoFinishSearch : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionDoFinishSearch(WaypointEditableView* WEV);
    ~QGCMissionDoFinishSearch();

protected:
    WaypointEditableView* WEV;

private:
    Ui::QGCMissionDoFinishSearch *ui;
};

#endif // QGCMISSIONDOFINISHSEARCH_H
