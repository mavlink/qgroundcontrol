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

public slots:
    void systemDeleted(QObject* uas);
    void systemCreated(UASInterface* uas);
    void systemSetActive(int uas);

protected:
    quint16 offline_uas_id;
    void changeEvent(QEvent *e);
    QMap<int, WaypointList*> lists;

private:
    Ui::QGCWaypointListMulti *ui;
};

#endif // QGCWAYPOINTLISTMULTI_H
