#ifndef QGCFIRMWAREUPDATE_H
#define QGCFIRMWAREUPDATE_H

#include <QWidget>

namespace Ui
{
class QGCFirmwareUpdate;
}

class QGCFirmwareUpdate : public QWidget
{
    Q_OBJECT
public:
    QGCFirmwareUpdate(QWidget *parent = 0);
    ~QGCFirmwareUpdate();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::QGCFirmwareUpdate *ui;
};

#endif // QGCFIRMWAREUPDATE_H
