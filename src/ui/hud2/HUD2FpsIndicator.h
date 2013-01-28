#ifndef HUD2FPSINDICATOR_H
#define HUD2FPSINDICATOR_H

#include <QWidget>
#include <QTimer>
#include <QPen>
#include <QFont>

class HUD2FpsIndicator : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2FpsIndicator(QWidget *parent = 0);
    void paint_static(QPainter *painter);
    void paint_dynamic(QPainter *painter);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void updateGeometry(const QSize &size);

private slots:
    void calc(void);

private:
    uint frames;
    uint fps;
    QTimer timer;
    QPen pen;
    QFont font;
};

#endif // HUD2FPSINDICATOR_H
