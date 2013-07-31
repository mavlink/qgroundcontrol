#ifndef FAILSAFECONFIG_H
#define FAILSAFECONFIG_H

#include <QWidget>
#include "ui_FailSafeConfig.h"
#include "AP2ConfigWidget.h"
class FailSafeConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit FailSafeConfig(QWidget *parent = 0);
    ~FailSafeConfig();
private slots:
    void activeUASSet(UASInterface *uas);
    void remoteControlChannelRawChanges(int chan,float value);
    void hilActuatorsChanged(uint64_t time, float act1, float act2, float act3, float act4, float act5, float act6, float act7, float act8);
    void armingChanged(bool armed);
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void batteryFailChecked(bool checked);
    void fsLongClicked(bool checked);
    void fsShortClicked(bool checked);
    void gcsChecked(bool checked);
    void throttleActionChecked(bool checked);
    void throttleChecked(bool checked);
    void throttlePwmChanged();
    void throttleFailSafeChanged(int index);
    void gpsStatusChanged(UASInterface* uas,int fixtype);
private:
    Ui::FailSafeConfig ui;
};

#endif // FAILSAFECONFIG_H
