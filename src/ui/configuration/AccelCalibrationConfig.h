#ifndef ACCELCALIBRATIONCONFIG_H
#define ACCELCALIBRATIONCONFIG_H

#include <QWidget>
#include "ui_AccelCalibrationConfig.h"
#include "UASManager.h"
#include "UASInterface.h"
#include "AP2ConfigWidget.h"

class AccelCalibrationConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit AccelCalibrationConfig(QWidget *parent = 0);
    ~AccelCalibrationConfig();
protected:
    void hideEvent(QHideEvent *evt);
private slots:
    void activeUASSet(UASInterface *uas);
    void calibrateButtonClicked();
    void uasTextMessageReceived(int uasid, int componentid, int severity, QString text);
private:
    int m_accelAckCount;
    Ui::AccelCalibrationConfig ui;
};

#endif // ACCELCALIBRATIONCONFIG_H
