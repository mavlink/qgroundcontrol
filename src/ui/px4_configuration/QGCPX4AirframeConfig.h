#ifndef QGCPX4AIRFRAMECONFIG_H
#define QGCPX4AIRFRAMECONFIG_H

#include <QWidget>

namespace Ui {
class QGCPX4AirframeConfig;
}

class QGCPX4AirframeConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCPX4AirframeConfig(QWidget *parent = 0);
    ~QGCPX4AirframeConfig();
    
private:
    Ui::QGCPX4AirframeConfig *ui;
};

#endif // QGCPX4AIRFRAMECONFIG_H
