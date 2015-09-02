#ifndef QGCUASFILEVIEWMULTI_H
#define QGCUASFILEVIEWMULTI_H

#include <QWidget>
#include <QMap>

#include "QGCUASFileView.h"
#include "UAS.h"

namespace Ui
{
class QGCUASFileViewMulti;
}

class QGCUASFileViewMulti : public QWidget
{
    Q_OBJECT

public:
    explicit QGCUASFileViewMulti(QWidget *parent = 0);
    ~QGCUASFileViewMulti();

protected:
    void changeEvent(QEvent *e);
    QMap<UAS*, QGCUASFileView*> lists;
    
private slots:
    void _vehicleAdded(Vehicle* vehicle);
    void _vehicleRemoved(Vehicle* vehicle);
    void _activeVehicleChanged(Vehicle* vehicle);

private:
    Ui::QGCUASFileViewMulti *ui;
};

#endif // QGCUASFILEVIEWMULTI_H
