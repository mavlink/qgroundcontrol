#ifndef UASCONTROLPARAMETERS_H
#define UASCONTROLPARAMETERS_H

#include <QWidget>
#include "UASManager.h"
#include "SlugsMAV.h"
#include <QTimer>
#include <QTabWidget>

namespace Ui {
    class UASControlParameters;
}

class UASControlParameters : public QWidget
{
    Q_OBJECT

public:
    explicit UASControlParameters(QWidget *parent = 0);
    ~UASControlParameters();

public slots:
    void changedMode(int mode);
    void activeUasSet(UASInterface* uas);
    void updateGlobalPosition(UASInterface*,double,double,double,quint64);
    void speedChanged(UASInterface*,double,double,double,quint64);
    void updateAttitude(UASInterface* uas, double roll, double pitch, double yaw, quint64 time);
    void setCommands();
    void getCommands();
    void updateMode(int uas,QString mode,QString description);

private:
    Ui::UASControlParameters *ui;
    QTimer* refreshTimerGet;
    UASInterface* activeUAS;
    double speed;
    double roll;
    double altitude;
    QString mode;
    QString REDcolorStyle;
};

#endif // UASCONTROLPARAMETERS_H
