#ifndef HUD2INDICATORSPEED_H
#define HUD2INDICATORSPEED_H

#include <QWidget>
#include <QPen>

#define SIZE_H_MIN      2
#define SIZE_TEXT_MIN   7

#include "HUD2Data.h"

class HUD2IndicatorSpeed : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2IndicatorSpeed(HUD2Data &huddata, QWidget *parent = 0);
    void paint(QPainter *painter);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void setColor(QColor color);
    void updateGeometry(const QSize &size);

private:
    QPen   thickPen;
    QPen   thinPen;
    QPen   arrowPen;
    QPen   textPen;
    QFont  textFont;
    QLineF *thickLines;
    int    thickLinesCnt;
    QLineF *thinLines;
    int    thinLinesCnt;
    QRect  *textRects;
    QString *textStrings;
    int    overlap; // how many "spare" scratches have on the ends
    QRect  mainRect;
    QRect  clipRect;
    QRect  numRect;
    bool   opaqueBackground;
    QLine  arrowLines[2];

    qreal scale_interval_pix;
    int scale_interval_deg;

    HUD2Data &huddata;
};

#endif // HUD2INDICATORSPEED_H
