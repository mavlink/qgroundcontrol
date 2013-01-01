#ifndef HUD2PITCHLINEPOS_H
#define HUD2PITCHLINEPOS_H

#include <QWidget>
#include <QPen>
#include <QLine>

#include "HUD2Data.h"

class HUD2PitchLinePos : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2PitchLinePos(int *gap, QWidget *parent = 0);
    void paint(QPainter *painter);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void updateGeometry(const QSize *size);
    void setColor(QColor color);

private:
    const int *gap; /* space between right and left parts */
    const HUD2data *huddata;
    QPen pen;
    QLine lines[4];

protected:

};

#endif // HUD2PITCHLINEPOS_H
