#pragma once

#include <QMap>

#include "QGCDockWidget.h"
#include "QGCUASFileView.h"
#include "UAS.h"

namespace Ui
{
class QGCUASFileViewMulti;
}

class QGCUASFileViewMulti : public QGCDockWidget
{
    Q_OBJECT

public:
    explicit QGCUASFileViewMulti(const QString& title, QAction* action, QWidget *parent = 0);
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

