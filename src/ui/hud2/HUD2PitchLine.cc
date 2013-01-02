#include <QtGui>
#include "HUD2PitchLine.h"

HUD2PitchLine::HUD2PitchLine(int *gap, QWidget *parent) :
    QWidget(parent)
{
    this->gap = gap;
    this->huddata = huddata;

    this->size_wscale = 18;
    this->size_hscale = 50;
    this->size_hmin   = 3;
}

/**
 * @brief HUD2PitchLine::update_lines_pos
 * @param rect
 */
void HUD2PitchLine::update_lines_pos(const QRect rect){

    // Positive pitch indicator:
    //
    //      _______      _______
    //     |10                  |
    //

    int x1 = *gap/2;
    int x2 = *gap/2 + rect.width();

    lines_pos[0] = QLine(x1,    0,     x2,  0);             // long right
    lines_pos[1] = QLine(-x1,   0,    -x2,  0);             // long left
    lines_pos[2] = QLine(x2,    0,     x2,  rect.height()); // short right
    lines_pos[3] = QLine(-x2,   0,    -x2,  rect.height()); // short left
}

/**
 * @brief HUD2PitchLine::update_lines_neg
 * @param rect
 * @param lines_array
 */
void HUD2PitchLine::update_lines_neg(const QRect rect, QLine *lines_array){

    int step = rect.width() / 5;
    QPoint p0 = rect.bottomLeft();
    QPoint p1 = p0;
    p1.setX(p1.x() + step);

    lines_array[0] = QLine(p0, p1);

    lines_array[1] = lines_array[0];
    lines_array[1].translate(step * 2, 0);

    lines_array[2] = lines_array[1];
    lines_array[2].translate(step * 2, 0);

    // vertical line
    if (rect.left() < 0)
        lines_array[3] = QLine(rect.topRight(), rect.bottomRight());
    else
        lines_array[3] = QLine(rect.topLeft(), rect.bottomLeft());
}

/**
 * @brief HUD2PitchLine::updateGeometry
 * @param size
 */
void HUD2PitchLine::updateGeometry(const QSize *size){

    int size_w = size->width()  / size_wscale;
    int size_h = size->height() / size_hscale;

    if (size_h < size_hmin)
        size_h = size_hmin;

    QRect rect = QRect(0, 0, size_w, size_h);
    update_lines_pos(rect);

    // negative right part
    rect.moveTo(*gap/2, rect.height());
    update_lines_neg(rect, lines_neg);

    // negative left part
    rect.moveTo(-(*gap/2) - rect.width(), rect.height());
    update_lines_neg(rect, lines_neg + 4);

    update_text(rect);
}

/**
 * @brief HUD2PitchLine::update_text
 * @param rect
 */
void HUD2PitchLine::update_text(QRect rect){
    int needed_h = 25;

    textPen.setColor(Qt::green);
    textFont.setPixelSize(25);

    textRect = rect;
    textRect.moveBottom(needed_h - textRect.height());
    textRect.setHeight(needed_h);
    //textRect.translate(0, 5);
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
    painter->drawText(textRect, Qt::AlignCenter, QString::number(deg));
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

void HUD2PitchLine::setColor(QColor color){
    pen.setColor(color);
}

