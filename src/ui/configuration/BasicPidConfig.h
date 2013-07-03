#ifndef BASICPIDCONFIG_H
#define BASICPIDCONFIG_H

#include <QWidget>
#include "ui_BasicPidConfig.h"

class BasicPidConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit BasicPidConfig(QWidget *parent = 0);
    ~BasicPidConfig();
    
private:
    Ui::BasicPidConfig ui;
};

#endif // BASICPIDCONFIG_H
