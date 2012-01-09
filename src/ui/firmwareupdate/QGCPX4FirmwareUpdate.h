#ifndef QGCPX4FIRMWAREUPDATE_H
#define QGCPX4FIRMWAREUPDATE_H

#include <QWidget>

namespace Ui {
class QGCPX4FirmwareUpdate;
}

class QGCPX4FirmwareUpdate : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCPX4FirmwareUpdate(QWidget *parent = 0);
    ~QGCPX4FirmwareUpdate();
    
private:
    Ui::QGCPX4FirmwareUpdate *ui;
};

#endif // QGCPX4FIRMWAREUPDATE_H
