#ifndef ACCELCALIBRATIONCONFIG_H
#define ACCELCALIBRATIONCONFIG_H

#include <QWidget>
#include "ui_AccelCalibrationConfig.h"
#include "UASManager.h"
#include "UASInterface.h"
class AccelCalibrationConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit AccelCalibrationConfig(QWidget *parent = 0);
    ~AccelCalibrationConfig();
private slots:
    void activeUASSet(UASInterface *uas);
    void calibrateButtonClicked();
    void uasTextMessageReceived(int uasid, int componentid, int severity, QString text);
private:
    int accelAckCount;
    Ui::AccelCalibrationConfig ui;
    UASInterface *m_uas;
};

#endif // ACCELCALIBRATIONCONFIG_H
