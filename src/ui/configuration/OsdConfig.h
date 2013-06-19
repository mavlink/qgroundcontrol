#ifndef OSDCONFIG_H
#define OSDCONFIG_H

#include <QWidget>
#include "ui_OsdConfig.h"

class OsdConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit OsdConfig(QWidget *parent = 0);
    ~OsdConfig();
    
private:
    Ui::OsdConfig ui;
};

#endif // OSDCONFIG_H
