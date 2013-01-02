#ifndef HUD2PITCHLINE_H
#define HUD2PITCHLINE_H

#include <QWidget>
#include <QPen>
#include <QLine>

#include "HUD2Data.h"

class HUD2PitchLine : public QWidget
{
    Q_OBJECT
public:
    explicit HUD2PitchLine(int *gap, QWidget *parent);
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
    int *gap; /* space between right and left parts */
    HUD2data *huddata;

    void update_lines_pos(const QRect rect);
    void update_lines_neg(const QRect rect, QLine *lines_array);
    QPen pen;
    QLine lines_pos[4];
    QLine lines_neg[8];

    void update_text(const QRect rect);
    void draw_text(QPainter *painter, int deg);
    QPen textPen;
    QFont textFont;
    QRect textRect;

protected:

};

#endif // HUD2PITCHLINE_H
