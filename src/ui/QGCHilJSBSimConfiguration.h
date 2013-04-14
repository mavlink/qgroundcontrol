#ifndef QGCHILJSBSIMCONFIGURATION_H
#define QGCHILJSBSIMCONFIGURATION_H

#include <QWidget>

#include "QGCHilLink.h"
#include "QGCFlightGearLink.h"
#include "UAS.h"

namespace Ui {
class QGCHilJSBSimConfiguration;
}

class QGCHilJSBSimConfiguration : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCHilJSBSimConfiguration(UAS* mav, QWidget *parent = 0);
    ~QGCHilJSBSimConfiguration();

protected:
    UAS* mav;
    
private slots:
    void on_startButton_clicked();
    void on_stopButton_clicked();

private:
    Ui::QGCHilJSBSimConfiguration *ui;
};

#endif // QGCHILJSBSIMCONFIGURATION_H
