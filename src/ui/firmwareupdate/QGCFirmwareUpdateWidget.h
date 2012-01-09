#ifndef QGCFIRMWAREUPDATEWIDGET_H
#define QGCFIRMWAREUPDATEWIDGET_H

#include <QWidget>

namespace Ui {
class QGCFirmwareUpdateWidget;
}

class QGCFirmwareUpdateWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCFirmwareUpdateWidget(QWidget *parent = 0);
    ~QGCFirmwareUpdateWidget();
    
private:
    Ui::QGCFirmwareUpdateWidget *ui;
};

#endif // QGCFIRMWAREUPDATEWIDGET_H
