#pragma once

#include <QWidget>

namespace Ui {
class QGCPluginHost;
}

class QGCPluginHost : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCPluginHost(QWidget *parent = 0);
    ~QGCPluginHost();
    
private:
    Ui::QGCPluginHost *ui;
};

