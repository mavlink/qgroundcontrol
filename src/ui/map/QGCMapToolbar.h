#ifndef QGCMAPTOOLBAR_H
#define QGCMAPTOOLBAR_H

#include <QWidget>

namespace Ui {
    class QGCMapToolbar;
}

class QGCMapToolbar : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMapToolbar(QWidget *parent = 0);
    ~QGCMapToolbar();

private:
    Ui::QGCMapToolbar *ui;
};

#endif // QGCMAPTOOLBAR_H
