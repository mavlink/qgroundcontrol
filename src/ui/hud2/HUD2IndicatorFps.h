#ifndef HUD2INDICATORFPS_H
#define HUD2INDICATORFPS_H

#include <QWidget>
#include <QTimer>
#include <QPen>
#include <QFont>

class HUD2IndicatorFps : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2IndicatorFps(QWidget *parent = 0);
    void paint(QPainter *painter);
    bool getEnabled(void){return enabled;}

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void updateGeometry(const QSize &size);
    void setEnabled(bool state){enabled = state;}

private slots:
    void calc(void);

private:
    uint frames;
    uint fps;
    QTimer timer;
    QPen pen;
    QFont font;
    bool enabled;
};

#endif // HUD2INDICATORFPS_H
