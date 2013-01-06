#include <QtGui>
#include "HUD2HorizonPitchLine.h"

HUD2PitchLine::HUD2PitchLine(const int *gapscale, QWidget *parent) :
    QWidget(parent)
{
    this->gapscale = gapscale;
    this->huddata = huddata;

    this->size_wscale   = 18;
    this->size_hscale   = 50;
    this->size_hmin     = 3;
    this->text_size_min = 8;

    this->textPen.setColor(Qt::green);
}

/**
 * @brief HUD2PitchLine::update_geometry_lines_pos
 * @param gap
 * @param w
 * @param h
 * @return rectangular suitable for text placing
 */
QRect HUD2PitchLine::update_geometry_lines_pos(int gap, int w, int h){

    // Positive pitch indicator:
    //
    //      _______      _______
    //     |10                  |
    //
    int x1, x2;

    // right
    x1 = gap/2;
    x2 = gap/2 + w;
    lines_pos[0] = QLine(x1, 0, x2, 0); // horiz
    lines_pos[1] = QLine(x2, 0, x2, h); // vert

    // left
    x1 = -gap/2;
    x2 = -gap/2 - w;
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
QRect HUD2PitchLine::update_geometry_lines_neg(int gap, int w, int h){

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
    x1 = gap/2;
    x2 = gap/2 + w;
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
    x1 = -gap/2;
    x2 = -gap/2 - w;
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
void HUD2PitchLine::updateGeometry(const QSize *size){
    int text_size = 25;
    int w = size->width()  / size_wscale;
    int h = size->height() / size_hscale;

    if (h < size_hmin)
        h = size_hmin;

    int gap = size->width() / *gapscale;
    textRectPos = update_geometry_lines_pos(gap, w, h);
    textRectNeg = update_geometry_lines_neg(gap, w, h);
    textRectNeg.translate(0, -text_size);

    textFont.setPixelSize(text_size);
    textRectPos.setHeight(text_size);
    textRectNeg.setHeight(text_size);
}

/**
 * @brief HUD2PitchLine::draw_text
 * @param painter
 * @param deg
 */
void HUD2PitchLine::draw_text(QPainter *painter, int deg){
    painter->save();
    painter->setPen(textPen);
    painter->setFont(textFont);
    if (deg > 0){
        //painter->fillRect(textRectPos, Qt::red);
        painter->drawText(textRectPos, Qt::AlignCenter, QString::number(deg));
    }
    else{
        //painter->fillRect(textRectNeg, Qt::red);
        painter->drawText(textRectNeg, Qt::AlignCenter, QString::number(deg));
    }
    painter->restore();
}

/**
 * @brief HUD2PitchLine::paint
 * @param painter
 * @param deg
 */
void HUD2PitchLine::paint(QPainter *painter, int deg){    
    if (deg > 0)
        painter->drawLines(lines_pos, sizeof(lines_pos)/sizeof(QLine));
    else
        painter->drawLines(lines_neg, sizeof(lines_neg)/sizeof(QLine));

    draw_text(painter, deg);
}

/**
 * @brief HUD2PitchLine::setColor
 * @param color
 */
void HUD2PitchLine::setColor(QColor color){
    pen.setColor(color);
}

