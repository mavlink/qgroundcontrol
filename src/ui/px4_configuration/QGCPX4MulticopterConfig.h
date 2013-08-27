#ifndef QGCPX4MULTICOPTERCONFIG_H
#define QGCPX4MULTICOPTERCONFIG_H

#include <QWidget>

namespace Ui {
class QGCPX4MulticopterConfig;
}

class QGCPX4MulticopterConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCPX4MulticopterConfig(QWidget *parent = 0);
    ~QGCPX4MulticopterConfig();
    
private:
    Ui::QGCPX4MulticopterConfig *ui;
};

#endif // QGCPX4MULTICOPTERCONFIG_H
