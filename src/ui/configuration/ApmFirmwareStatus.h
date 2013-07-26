#ifndef APMFIRMWARESTATUS_H
#define APMFIRMWARESTATUS_H

#include <QWidget>
#include "ui_ApmFirmwareStatus.h"

class ApmFirmwareStatus : public QWidget
{
    Q_OBJECT
    
public:
    explicit ApmFirmwareStatus(QWidget *parent = 0);
    ~ApmFirmwareStatus();
    void passMessage(QString msg);
    void setStatus(QString message);
    void resetProgress();
    void progressTick();
    
private:
    Ui::ApmFirmwareStatus ui;
};

#endif // APMFIRMWARESTATUS_H
