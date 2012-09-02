#ifndef QGCVEHICLECONFIG_H
#define QGCVEHICLECONFIG_H

#include <QWidget>

namespace Ui {
class QGCVehicleConfig;
}

class QGCVehicleConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCVehicleConfig(QWidget *parent = 0);
    ~QGCVehicleConfig();
    
private:
    Ui::QGCVehicleConfig *ui;
};

#endif // QGCVEHICLECONFIG_H
