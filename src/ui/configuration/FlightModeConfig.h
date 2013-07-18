#ifndef FLIGHTMODECONFIG_H
#define FLIGHTMODECONFIG_H

#include <QWidget>
#include "ui_FlightModeConfig.h"
#include "UASInterface.h"
#include "UASManager.h"
#include "AP2ConfigWidget.h"

class FlightModeConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit FlightModeConfig(QWidget *parent = 0);
    ~FlightModeConfig();
private slots:
    void activeUASSet(UASInterface *uas);
    void saveButtonClicked();
    void modeChanged(int sysId, QString status, QString description);
    void remoteControlChannelRawChanged(int chan,float val);
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
private:
    QMap<int,int> roverModeIndexToUiIndex;
    QMap<int,int> planeModeIndexToUiIndex;
    QMap<int,int> roverModeUiIndexToIndex;
    QMap<int,int> planeModeUiIndexToIndex;
    Ui::FlightModeConfig ui;
    UASInterface *m_uas;
};

#endif // FLIGHTMODECONFIG_H
