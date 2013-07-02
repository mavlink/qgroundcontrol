#ifndef FAILSAFECONFIG_H
#define FAILSAFECONFIG_H

#include <QWidget>
#include "ui_FailSafeConfig.h"

class FailSafeConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit FailSafeConfig(QWidget *parent = 0);
    ~FailSafeConfig();
    
private:
    Ui::FailSafeConfig ui;
};

#endif // FAILSAFECONFIG_H
