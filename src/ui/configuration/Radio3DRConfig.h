#ifndef RADIO3DRCONFIG_H
#define RADIO3DRCONFIG_H

#include <QWidget>
#include "ui_Radio3DRConfig.h"

class Radio3DRConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit Radio3DRConfig(QWidget *parent = 0);
    ~Radio3DRConfig();
    
private:
    Ui::Radio3DRConfig ui;
};

#endif // RADIO3DRCONFIG_H
