#ifndef HUD2INDICATORYAW_H
#define HUD2INDICATORYAW_H

#include <QWidget>
#include <QPen>

#include "HUD2Data.h"

#define SIZE_H_MIN      2
#define SIZE_TEXT_MIN   7

class HUD2IndicatorYaw : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2IndicatorYaw(HUD2Data &huddata, QWidget *parent = 0);
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
    int    overlap; // how many "spare" scratches have on the ends to realize "gapless" rotation
    QRect  mainRect;
    QRect  clipRect;
    QRect  numRect;
    bool   opaqueBackground;
    QLine  arrowLines[2];

    qreal scale_interval_pix;
    int scale_interval_deg;

    HUD2Data &huddata;
};

#endif // HUD2INDICATORYAW_H
