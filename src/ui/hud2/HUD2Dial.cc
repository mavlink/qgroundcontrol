#include "HUD2Dial.h"
#include "HUD2Math.h"

HUD2Dial::HUD2Dial(qreal r, qreal x, qreal y,
                   int marks, int markStep, int hands,
                   QPen *handPens, qreal *handScales,
                   QWidget *parent)
    : QWidget(parent),
      r(r),
      x(x),
      y(y),
      marks(marks),
      markStep(markStep),
      hands(hands)
{
    // marks
    this->markRects = new QRect[marks];
    this->markStrings = new QString[marks];
    this->markPen.setColor(Qt::white);

    // hands
    this->handLines  = new QLine[hands]; // will be inited on size change
    this->handPens   = new QPen[hands];
    this->handScales = new qreal[hands];
    for (int i=0; i<hands; i++){
        this->handPens[i] = handPens[i];
        this->handScales[i] = handScales[i];
    }

    // other
    this->dialPen.setColor(Qt::green);
    this->dialPen.setWidth(0);
}

void HUD2Dial::updateGeometry(const QSize *size){
    // main sizes
    _r = percent2pix_h(size, r);
    _y = percent2pix_h(size, y);
    _x = percent2pix_w(size, x);

    // hands
    handLines[0] = QLine(0, 0, 0, -_r/2);
    handLines[1] = QLine(0, 0, 0, -(3*_r)/4 );
    handLines[2] = QLine(0, 0, 0, -_r);

    // marks
    const int markSizeMin = 8;
    int markSize = _r / 4;
    if (markSize < markSizeMin)
        markSize = markSizeMin;

    int gap = -markSize / 3; // additional gap between number and dial circle
    markFont.setPixelSize(markSize);
    for (int i=0; i<marks/markStep; i+=markStep){
        // convert numbers to string
        markStrings[i] = QString::number(i);

        // move rectangles for text in need positionsb
        markRects[i].setSize(QSize(markSize, markSize));
        qreal phi = i * (2 * M_PI / marks) - M_PI/2;
        qreal x = cos(phi) * (_r - markSize - gap);
        qreal y = sin(phi) * (_r - markSize - gap);
        markRects[i].moveCenter(QPoint(x, y));
    }

    //other
}

void HUD2Dial::paint(QPainter *painter, qreal value){
    painter->save();
    painter->setPen(dialPen);
    painter->drawEllipse(QPoint(_x, _y), _r, _r); // dial circle
    painter->translate(_x, _y);

    // marks
    painter->save();
    painter->setPen(markPen);
    painter->setFont(markFont);
    for (int i=0; i<marks/markStep; i+=markStep){
        painter->drawText(markRects[i], Qt::AlignCenter, markStrings[i]);
    }
    painter->restore();

    // hands
    for (int i=0; i < hands; i++){
        painter->save();
        painter->rotate(360*value / handScales[i]);
        painter->setPen(handPens[i]);
        painter->drawLine(handLines[i]);
        painter->restore();
    }

    //other
    painter->restore();
}

void HUD2Dial::setColor(QColor color){
    Q_UNUSED(color);
}
