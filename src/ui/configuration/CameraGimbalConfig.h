#ifndef CAMERAGIMBALCONFIG_H
#define CAMERAGIMBALCONFIG_H

#include <QWidget>
#include "AP2ConfigWidget.h"
#include "ui_CameraGimbalConfig.h"

class CameraGimbalConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit CameraGimbalConfig(QWidget *parent = 0);
    ~CameraGimbalConfig();
private slots:
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void updateTilt();
    void updateRoll();
    void updatePan();
    void updateShutter();

private:
    Ui::CameraGimbalConfig ui;
    QString m_shutterPrefix;
    QString m_rollPrefix;
    QString m_tiltPrefix;
    QString m_panPrefix;
};

#endif // CAMERAGIMBALCONFIG_H
