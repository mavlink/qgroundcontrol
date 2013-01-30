#include <QtGui>

#include "HUD2HorizonPitch.h"
#include "HUD2Math.h"

HUD2HorizonPitch::HUD2HorizonPitch(const qreal *gap, QWidget *parent) :
    QWidget(parent),
    gap(gap)
{
    this->size_w = 6;
    this->size_h = 2;

    this->textPen.setColor(Qt::green);

    pen.setColor(Qt::green);
    pen.setWidth(2);
}

/**
 * @brief HUD2PitchLine::update_geometry_lines_pos
 * @param gap
 * @param w
 * @param h
 * @return rectangular suitable for text placing
 */
QRect HUD2HorizonPitch::update_geometry_lines_pos(int _gap, int w, int h){

    // Positive pitch indicator:
    //
    //      _______      _______
    //     |10                  |
    //
    int x1, x2;

    // right
    x1 = _gap/2;
    x2 = _gap/2 + w;
    lines_pos[0] = QLine(x1, 0, x2, 0); // horiz
    lines_pos[1] = QLine(x2, 0, x2, h); // vert

    // left
    x1 = -_gap/2;
    x2 = -_gap/2 - w;
    lines_pos[2] = QLine(x1, 0, x2,  0); // horiz
    lines_pos[3] = QLine(x2, 0, x2,  h); // vert

    return QRect(x2, 0, w, h);
}

/**
 * @brief HUD2PitchLine::update_lines_neg
 * @param rect
 * @param lines_array
 * @return rectangular suitable for text placing
 */
QRect HUD2HorizonPitch::update_geometry_lines_neg(int _gap, int w, int h){

    // Negative pitch indicator:
    //
    //      -10
    //     _ _ _ _|     |_ _ _ _
    //

    int dashcount = 3;
    int step = w / (dashcount * 2 - 1); // do not forget about spaces between dashes
    int x1, x2;
    int line = 0;
    QPoint p0, p1;
    QLine l;

    // right
    x1 = _gap/2;
    x2 = _gap/2 + w;
    lines_neg[line] = QLine(x1, 0, x1, -h); // vert
    line++;
    p0 = QPoint(x1, 0);
    p1 = QPoint(x1 + step, 0);
    l  = QLine(p0, p1);
    for(int i=0; i<dashcount; i++){
        lines_neg[line] = l;
        line++;
        l.translate(step * 2, 0);
    }

    // left
    x1 = -_gap/2;
    x2 = -_gap/2 - w;
    lines_neg[line] = QLine(x1, 0, x1, -h); // vert
    line++;
    p0 = QPoint(x1, 0);
    p1 = QPoint(x1 - step, 0);
    l  = QLine(p0, p1);
    for(int i=0; i<dashcount; i++){
        lines_neg[line] = l;
        line++;
        l.translate(-step * 2, 0);
    }

    return QRect(x2, 0, w, h);
}

/**
 * @brief HUD2PitchLine::updateGeometry
 * @param size
 */
void HUD2HorizonPitch::updateGeometry(const QSize &size){

    int tmp;
    tmp = percent2pix_d(size, 0.3);
    hud2_clamp(tmp, 1, 10);
    pen.setWidth(tmp);

    int text_size = percent2pix_h(size, 4);
    int w = percent2pix_w(size, size_w);
    int h = percent2pix_h(size, size_h);

    hud2_clamp(h, SIZE_H_MIN, 50);
    hud2_clamp(text_size, SIZE_TEXT_MIN, 50);

    int _gap = percent2pix_w(size, *gap);
    textRectPos = update_geometry_lines_pos(_gap, w, h);
    textRectNeg = update_geometry_lines_neg(_gap, w, h);
    textRectNeg.translate(0, -text_size);

    textFont.setPixelSize(text_size);
    textRectPos.setHeight(text_size);
    textRectPos.translate(0, 2);
    textRectNeg.setHeight(text_size);
}

void HUD2HorizonPitch::paint(QPainter *painter, int deg){
    painter->save();

    painter->setPen(pen);
    if (deg > 0)
        painter->drawLines(lines_pos, sizeof(lines_pos)/sizeof(lines_pos[0]));
    else
        painter->drawLines(lines_neg, sizeof(lines_neg)/sizeof(lines_neg[0]));

    painter->setPen(textPen);
    painter->setFont(textFont);
    if (deg > 0)
        painter->drawText(textRectPos, Qt::AlignCenter, QString::number(deg));
    else
        painter->drawText(textRectNeg, Qt::AlignCenter, QString::number(deg));

    painter->restore();
}

void HUD2HorizonPitch::setColor(QColor color){
    pen.setColor(color);
}

