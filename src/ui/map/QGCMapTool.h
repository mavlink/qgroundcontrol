#ifndef QGCMAPTOOL_H
#define QGCMAPTOOL_H

#include <QWidget>

namespace Ui {
    class QGCMapTool;
}

class QGCMapTool : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMapTool(QWidget *parent = 0);
    ~QGCMapTool();

private:
    Ui::QGCMapTool *ui;
};

#endif // QGCMAPTOOL_H
