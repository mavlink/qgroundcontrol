#ifndef FLIGHTMODECONFIG_H
#define FLIGHTMODECONFIG_H

#include <QWidget>
#include "ui_FlightModeConfig.h"
#include "UASInterface.h"
#include "UASManager.h"

class FlightModeConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit FlightModeConfig(QWidget *parent = 0);
    ~FlightModeConfig();
private slots:
    void setActiveUAS(UASInterface *uas);
    void modeChanged(int sysId, QString status, QString description);
    void remoteControlChannelRawChanged(int chan,float val);
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
private:
    QMap<int,int> roverModeIndexToUiIndex;
    QMap<int,int> planeModeIndexToUiIndex;
    Ui::FlightModeConfig ui;
    UASInterface *m_uas;
};

#endif // FLIGHTMODECONFIG_H
