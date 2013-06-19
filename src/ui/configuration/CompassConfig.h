#ifndef COMPASSCONFIG_H
#define COMPASSCONFIG_H

#include <QWidget>
#include "ui_CompassConfig.h"

class CompassConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit CompassConfig(QWidget *parent = 0);
    ~CompassConfig();
    
private:
    Ui::CompassConfig ui;
};

#endif // COMPASSCONFIG_H
