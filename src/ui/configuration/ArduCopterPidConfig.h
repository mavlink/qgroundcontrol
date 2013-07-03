#ifndef ARDUCOPTERPIDCONFIG_H
#define ARDUCOPTERPIDCONFIG_H

#include <QWidget>

namespace Ui {
class ArduCopterPidConfig;
}

class ArduCopterPidConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit ArduCopterPidConfig(QWidget *parent = 0);
    ~ArduCopterPidConfig();
    
private:
    Ui::ArduCopterPidConfig *ui;
};

#endif // ARDUCOPTERPIDCONFIG_H
