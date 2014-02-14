#ifndef QGCHILFLIGHTGEARCONFIGURATION_H
#define QGCHILFLIGHTGEARCONFIGURATION_H

#include <QWidget>

#include "QGCHilLink.h"
#include "QGCFlightGearLink.h"
#include "UAS.h"

namespace Ui {
class QGCHilFlightGearConfiguration;
}

class QGCHilFlightGearConfiguration : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCHilFlightGearConfiguration(UAS* mav, QWidget *parent = 0);
    ~QGCHilFlightGearConfiguration();

protected:
    UAS* mav;
    
private slots:
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_barometerOffsetLineEdit_textChanged(const QString& baroOffset);

private:
    Ui::QGCHilFlightGearConfiguration *ui;

signals:
     void barometerOffsetChanged(float barometerOffsetkPa);
};

#endif // QGCHILFLIGHTGEARCONFIGURATION_H
