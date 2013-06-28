#ifndef OSDCONFIG_H
#define OSDCONFIG_H

#include <QWidget>
#include "AP2ConfigWidget.h"
#include "ui_OsdConfig.h"

class OsdConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit OsdConfig(QWidget *parent = 0);
    ~OsdConfig();
private slots:
    void enableButtonClicked();
private:
    Ui::OsdConfig ui;
};

#endif // OSDCONFIG_H
