#ifndef HUD2INDICATORHORIZON_H
#define HUD2INDICATORHORIZON_H

#include <QWidget>
#include <QPen>
#include <QLine>

#include "HUD2Data.h"
#include "HUD2IndicatorHorizonPitchline.h"
#include "HUD2IndicatorHorizonCrosshair.h"
#include "HUD2FormHorizon.h"

class HUD2IndicatorHorizon : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2IndicatorHorizon(const double *pitch, const double *roll, QWidget *parent);
    void paint(QPainter *painter);
    HUD2FormHorizon *getForm(void);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void updateGeometry(const QSize &size);
    void setColor(QColor color);
    void setSkyColor(QColor color);
    void setGndColor(QColor color);
    void setColoredBg(bool checked);
    void setBigScratchLenStep(double value);
    void setBigScratchValueStep(int value);
    void setStepsBig(int value);

private:
    void drawpitchlines(QPainter *painter, qreal bigScratchValueStep, qreal big_pixstep);
    void drawhorizon(QPainter *painter);

    HUD2IndicatorHorizonPitchline pitchline;
    HUD2IndicatorHorizonCrosshair crosshair;
    HUD2FormHorizon *form;

    int bigScratchValueStep;    // numerical value step
    qreal big_pixstep;          // pixels between two lines (internal use only)
    qreal bigScratchLenStep;    // step in percents of widget sizes
    int stepsBig;
    QSize size_cached;          // cached value to recalculate all sizes after changing of settings

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
