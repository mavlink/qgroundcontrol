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
    void showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        emit visibilityChanged(true);
    }

    void hideEvent(QHideEvent* event)
    {
        QWidget::hideEvent(event);
        emit visibilityChanged(false);
    }

private:
    Ui::QGCFirmwareUpdate *ui;

signals:
    void visibilityChanged(bool visible);
};

#endif // QGCFIRMWAREUPDATE_H
