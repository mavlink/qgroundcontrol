#ifndef QGCMISSIONDOWIDGET_H
#define QGCMISSIONDOWIDGET_H

#include <QWidget>

namespace Ui
{
class QGCMissionDoWidget;
}

class QGCMissionDoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionDoWidget(QWidget *parent = 0);
    ~QGCMissionDoWidget();

private:
    Ui::QGCMissionDoWidget *ui;
};

#endif // QGCMISSIONDOWIDGET_H
