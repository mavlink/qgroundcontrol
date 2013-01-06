#ifndef HUD2PITCHLINE_H
#define HUD2PITCHLINE_H

#include <QWidget>
#include <QPen>
#include <QLine>

#include "HUD2Data.h"

class HUD2HorizonPitchLine : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2HorizonPitchLine(const int *gapscale, QWidget *parent);
    void paint(QPainter *painter, int deg);

signals:
    void geometryChanged(const QSize *size);

public slots:
    void updateGeometry(const QSize *size);
    void setColor(QColor color);

private:
    int size_wscale;
    int size_hscale;
    int size_hmin;
    int text_size_min;
    const int *gapscale; /* space between right and left parts */
    HUD2data *huddata;

    QRect update_geometry_lines_pos(int gap, int w, int h);
    QRect update_geometry_lines_neg(int gap, int w, int h);

    QPen pen;
    QLine lines_pos[4];
    QLine lines_neg[8];

    void update_text(const QRect rect);
    void draw_text(QPainter *painter, int deg);
    QPen textPen;
    QFont textFont;
    QRect textRectPos;
    QRect textRectNeg;

protected:

};

#endif // HUD2PITCHLINE_H
