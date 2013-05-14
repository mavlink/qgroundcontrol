#ifndef HUD2INDICATORHORIZON_H
#define HUD2INDICATORHORIZON_H

#include <QWidget>
#include <QPen>
#include <QLine>

#include "HUD2Data.h"
#include "HUD2IndicatorHorizonPitchline.h"
#include "HUD2IndicatorHorizonCrosshair.h"

class HUD2IndicatorHorizon : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2IndicatorHorizon(const double *pitch, const double *roll, QWidget *parent);
    void paint(QPainter *painter);
    bool getColoredBg(void){return coloredBackground;}

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void setSkyColor(QColor color);
    void setGndColor(QColor color);
    void updateGeometry(const QSize &size);
    void setColoredBg(bool checked){coloredBackground = checked;}

private:
    void drawpitchlines(QPainter *painter, qreal degstep, qreal pixstep);
    void drawhorizon(QPainter *painter);

    HUD2IndicatorHorizonPitchline pitchline;
    int degstep;    // vertical screen capacity in degrees
    qreal pixstep;  // pixels between two lines
    int pitchcount; // how many pitch lines can be fitted on screen (approximately)

    HUD2IndicatorHorizonCrosshair crosshair;

    qreal gap; /* space between right and left parts */
    QPen pen;
    QLine hirizonleft;
    QLine horizonright;

    bool coloredBackground;
    QColor skyColor;
    QColor gndColor;
    QPen   skyPen;
    QPen   gndPen;
    QBrush skyBrush;
    QBrush gndBrush;

    const double *pitch;
    const double *roll;
};

#endif // HUD2INDICATORHORIZON_H
