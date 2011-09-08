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

signals:
    void visibilityChanged(bool visible);

protected:
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
    Ui::QGCMapTool *ui;
};

#endif // QGCMAPTOOL_H
