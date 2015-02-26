#ifndef QGCWAYPOINTLISTMULTI_H
#define QGCWAYPOINTLISTMULTI_H

#include <QWidget>
#include <QMap>

#include "WaypointList.h"
#include "UASInterface.h"

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
    void _systemDeleted(QObject* uas);
    void _systemCreated(UASInterface* uas);
    void _systemSetActive(UASInterface* uas);

private:
    
    static void*                _offlineUAS;
    QMap<void*, WaypointList*>  _lists;
    Ui::QGCWaypointListMulti*   _ui;
};

#endif // QGCWAYPOINTLISTMULTI_H
