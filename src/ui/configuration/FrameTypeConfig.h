#ifndef FRAMETYPECONFIG_H
#define FRAMETYPECONFIG_H

#include <QWidget>
#include "ui_FrameTypeConfig.h"

class FrameTypeConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit FrameTypeConfig(QWidget *parent = 0);
    ~FrameTypeConfig();
    
private:
    Ui::FrameTypeConfig ui;
};

#endif // FRAMETYPECONFIG_H
