#ifndef HUD2ALTIMETER_H
#define HUD2ALTIMETER_H

#include <QWidget>

#include "HUD2Dial.h"

class HUD2Altimeter : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2Altimeter(HUD2Data &huddata, QWidget *parent);
    void paint(QPainter *painter);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void updateGeometry(const QSize &size);
    
private:
    HUD2Dial *dial;
    HUD2Data &huddata;
};

#endif // HUD2ALTIMETER_H
