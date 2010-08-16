#ifndef QGCPXIMUFIRMWAREUPDATE_H
#define QGCPXIMUFIRMWAREUPDATE_H

#include <QWidget>

namespace Ui {
    class QGCPxImuFirmwareUpdate;
}

class QGCPxImuFirmwareUpdate : public QWidget {
    Q_OBJECT
public:
    QGCPxImuFirmwareUpdate(QWidget *parent = 0);
    ~QGCPxImuFirmwareUpdate();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::QGCPxImuFirmwareUpdate *ui;
};

#endif // QGCPXIMUFIRMWAREUPDATE_H
