#ifndef QGCMISSIONCONDITIONWIDGET_H
#define QGCMISSIONCONDITIONWIDGET_H

#include <QWidget>

namespace Ui {
    class QGCMissionConditionWidget;
}

class QGCMissionConditionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMissionConditionWidget(QWidget *parent = 0);
    ~QGCMissionConditionWidget();

private:
    Ui::QGCMissionConditionWidget *ui;
};

#endif // QGCMISSIONCONDITIONWIDGET_H
