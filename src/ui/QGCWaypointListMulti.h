#ifndef QGCWAYPOINTLISTMULTI_H
#define QGCWAYPOINTLISTMULTI_H

#include <QWidget>
#include <QMap>

#include "WaypointList.h"
#include "MultiVehicleManager.h"

namespace Ui
{
class QGCWaypointListMulti;
}

class QGCWaypointListMulti : public QWidget
{
    Q_OBJECT

public:
    explicit QGCWaypointListMulti(QWidget *parent = 0);
    ~QGCWaypointListMulti();

protected:
    // Override from Widget
    virtual void changeEvent(QEvent *e);
    
private slots:
    void _vehicleRemoved(Vehicle* vehicle);
    void _vehicleAdded(Vehicle* vehicle);
    void _activeVehicleChanged(Vehicle* vehicle);

private:
    
    static void*                _offlineUAS;
    QMap<void*, WaypointList*>  _lists;
    Ui::QGCWaypointListMulti*   _ui;
};

#endif // QGCWAYPOINTLISTMULTI_H
