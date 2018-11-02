#pragma once

#include <QWidget>

#include "QGCHilLink.h"
#include "QGCFlightGearLink.h"
#include "Vehicle.h"

namespace Ui {
class QGCHilJSBSimConfiguration;
}

class QGCHilJSBSimConfiguration : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCHilJSBSimConfiguration(Vehicle* vehicle, QWidget *parent = 0);
    ~QGCHilJSBSimConfiguration();

private slots:
    void on_startButton_clicked();
    void on_stopButton_clicked();

private:
    Vehicle*    _vehicle;

    Ui::QGCHilJSBSimConfiguration *ui;
};

