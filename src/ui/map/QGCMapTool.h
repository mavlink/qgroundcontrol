#ifndef QGCMAPTOOL_H
#define QGCMAPTOOL_H

#include <QWidget>
#include <QMenu>

namespace Ui {
    class QGCMapTool;
}

class QGCMapTool : public QWidget
{
    Q_OBJECT

public:
    explicit QGCMapTool(QWidget *parent = 0);
    ~QGCMapTool();

public slots:
    /** @brief Update slider zoom from map change */
    void setZoom(int zoom);

private:
    Ui::QGCMapTool *ui;
};

#endif // QGCMAPTOOL_H
