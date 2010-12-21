#ifndef QGCUNCONNECTEDINFOWIDGET_H
#define QGCUNCONNECTEDINFOWIDGET_H

#include <QWidget>

namespace Ui {
    class QGCUnconnectedInfoWidget;
}

class QGCUnconnectedInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QGCUnconnectedInfoWidget(QWidget *parent = 0);
    ~QGCUnconnectedInfoWidget();

    Ui::QGCUnconnectedInfoWidget *ui;

signals:
    void simulation();
    void addLink();
};

#endif // QGCUNCONNECTEDINFOWIDGET_H
