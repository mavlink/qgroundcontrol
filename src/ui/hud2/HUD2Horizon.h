#ifndef HUDHORIZON_H
#define HUDHORIZON_H

#include <QWidget>
#include <QPen>
#include <QLine>

#include "HUD2Data.h"
#include "HUD2PitchLinePos.h"

class HUD2Horizon : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2Horizon(HUD2data *huddata, QWidget *parent = 0);
    void paint(QPainter *painter, QColor color);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void updateGeometry(const QSize *size);

private:
    void drawpitchlines(QPainter *painter, qreal degstep, qreal pixstep);
    void drawwings(QPainter *painter, QColor color);
    qreal rad2deg(float);

    HUD2PitchLinePos pitchlinepos;
    int pitchstep_pix;  // pixels between pitch lines (for internal use only)
    int pitchstep_deg;  // degrees between pitch lines
    int pitchcount; // how many pitch lines can be fitted on screen

    int gap; /* space between right and left parts */
    const HUD2data *huddata;
    QPen pen;
    QLine left;
    QLine right;
};

#endif // HUDHORIZON_H
