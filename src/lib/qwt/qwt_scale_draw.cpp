/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include <qpen.h>
#include <qpainter.h>
#include "qwt_math.h"
#include "qwt_painter.h"
#include "qwt_polygon.h"
#include "qwt_scale_div.h"
#include "qwt_scale_map.h"
#include "qwt_scale_draw.h"

#if QT_VERSION < 0x040000
#include <qwmatrix.h>
#define QwtMatrix QWMatrix
#else
#include <qmatrix.h>
#define QwtMatrix QMatrix
#endif

class QwtScaleDraw::PrivateData
{
public:
    PrivateData():
        len(0),
        alignment(QwtScaleDraw::BottomScale),
        labelAlignment(0),
        labelRotation(0.0)
    {
    }

    QPoint pos;
    int len;

    Alignment alignment;

#if QT_VERSION < 0x040000
    int labelAlignment;
#else
    Qt::Alignment labelAlignment;
#endif
    double labelRotation;
};

/*!
  \brief Constructor

  The range of the scale is initialized to [0, 100],
  The position is at (0, 0) with a length of 100.
  The orientation is QwtAbstractScaleDraw::Bottom.
*/
QwtScaleDraw::QwtScaleDraw()
{
    d_data = new QwtScaleDraw::PrivateData;
    setLength(100);
}

//! Copy constructor
QwtScaleDraw::QwtScaleDraw(const QwtScaleDraw &other):
    QwtAbstractScaleDraw(other)
{
    d_data = new QwtScaleDraw::PrivateData(*other.d_data);
}

//! Destructor
QwtScaleDraw::~QwtScaleDraw()
{
    delete d_data;
}

//! Assignment operator
QwtScaleDraw &QwtScaleDraw::operator=(const QwtScaleDraw &other)
{
    *(QwtAbstractScaleDraw*)this = (const QwtAbstractScaleDraw &)other;
    *d_data = *other.d_data;
    return *this;
}

/*! 
   Return alignment of the scale
   \sa setAlignment()
*/
QwtScaleDraw::Alignment QwtScaleDraw::alignment() const 
{
    return d_data->alignment; 
}

/*!
   Set the alignment of the scale

   The default alignment is QwtScaleDraw::BottomScale
   \sa alignment()
*/
void QwtScaleDraw::setAlignment(Alignment align)
{
    d_data->alignment = align;
}

/*!
  Return the orientation

  TopScale, BottomScale are horizontal (Qt::Horizontal) scales,
  LeftScale, RightScale are vertical (Qt::Vertical) scales.

  \sa alignment()
*/
Qt::Orientation QwtScaleDraw::orientation() const
{
    switch(d_data->alignment)
    {
        case TopScale:
        case BottomScale:
            return Qt::Horizontal;
        case LeftScale:
        case RightScale:
        default:
            return Qt::Vertical;
    }
}

/*!
  \brief Determine the minimum border distance

  This member function returns the minimum space
  needed to draw the mark labels at the scale's endpoints.

  \param font Font
  \param start Start border distance
  \param end End border distance
*/
void QwtScaleDraw::getBorderDistHint(const QFont &font,
    int &start, int &end ) const
{
    start = 0;
    end = 0;
    
    if ( !hasComponent(QwtAbstractScaleDraw::Labels) )
        return;

    const QwtValueList &ticks = scaleDiv().ticks(QwtScaleDiv::MajorTick);
    if ( ticks.count() == 0 ) 
        return;

    QRect lr = labelRect(font, ticks[0]);

    // find the distance between tick and border
    int off = qwtAbs(map().transform(ticks[0]) - qRound(map().p1()));

    if ( orientation() == Qt::Vertical )
        end = lr.bottom() + 1 - off;
    else
        start = -lr.left() - off;

    const int lastTick = ticks.count() - 1;
    lr = labelRect(font, ticks[lastTick]);

    // find the distance between tick and border
    off = qwtAbs(map().transform(ticks[lastTick]) - qRound(map().p2()));

    if ( orientation() == Qt::Vertical )
        start = -lr.top() - off;
    else
        end = lr.right() + 1 - off;

    // if the distance between tick and border is larger
    // than half of the label width/height, we set to 0

    if ( start < 0 )
        start = 0;
    if ( end < 0 )
        end = 0;
}

/*!
  Determine the minimum distance between two labels, that is necessary
  that the texts don't overlap.

  \param font Font
  \return The maximum width of a label

  \sa getBorderDistHint()
*/

int QwtScaleDraw::minLabelDist(const QFont &font) const
{
    if ( !hasComponent(QwtAbstractScaleDraw::Labels) )
        return 0;

    const QwtValueList &ticks = scaleDiv().ticks(QwtScaleDiv::MajorTick);
    if (ticks.count() == 0)
        return 0;

    const QFontMetrics fm(font);

    const bool vertical = (orientation() == Qt::Vertical);

    QRect bRect1;
    QRect bRect2 = labelRect(font, ticks[0]);
    if ( vertical )
    {
        bRect2.setRect(-bRect2.bottom(), 0, bRect2.height(), bRect2.width());
    }
    int maxDist = 0;

    for (uint i = 1; i < (uint)ticks.count(); i++ )
    {
        bRect1 = bRect2;
        bRect2 = labelRect(font, ticks[i]);
        if ( vertical )
        {
            bRect2.setRect(-bRect2.bottom(), 0,
                bRect2.height(), bRect2.width());
        }

        int dist = fm.leading(); // space between the labels
        if ( bRect1.right() > 0 )
            dist += bRect1.right();
        if ( bRect2.left() < 0 )
            dist += -bRect2.left();

        if ( dist > maxDist )
            maxDist = dist;
    }

    double angle = labelRotation() / 180.0 * M_PI;
    if ( vertical )
        angle += M_PI / 2;

    if ( sin(angle) == 0.0 )
        return maxDist;

    const int fmHeight = fm.ascent() - 2; 

    // The distance we need until there is
    // the height of the label font. This height is needed
    // for the neighbour labal.

    int labelDist = (int)(fmHeight / sin(angle) * cos(angle));
    if ( labelDist < 0 )
        labelDist = -labelDist;

    // The cast above floored labelDist. We want to ceil.
    labelDist++; 

    // For text orientations close to the scale orientation 

    if ( labelDist > maxDist )
        labelDist = maxDist;

    // For text orientations close to the opposite of the 
    // scale orientation

    if ( labelDist < fmHeight )
        labelDist = fmHeight;

    return labelDist;
}

/*!
   Calculate the width/height that is needed for a
   vertical/horizontal scale.

   The extent is calculated from the pen width of the backbone,
   the major tick length, the spacing and the maximum width/height
   of the labels.

   \param pen Pen that is used for painting backbone and ticks
   \param font Font used for painting the labels

   \sa minLength()
*/
int QwtScaleDraw::extent(const QPen &pen, const QFont &font) const
{
    int d = 0;

    if ( hasComponent(QwtAbstractScaleDraw::Labels) )
    {
        if ( orientation() == Qt::Vertical )
            d = maxLabelWidth(font);
        else
            d = maxLabelHeight(font);

        if ( d > 0 )
            d += spacing();
    }

    if ( hasComponent(QwtAbstractScaleDraw::Ticks) )
    {
        d += majTickLength();
    }

    if ( hasComponent(QwtAbstractScaleDraw::Backbone) )
    {
        const int pw = qwtMax( 1, pen.width() );  // penwidth can be zero
        d += pw;
    }

    d = qwtMax(d, minimumExtent());
    return d;
}

/*!
   Calculate the minimum length that is needed to draw the scale

   \param pen Pen that is used for painting backbone and ticks
   \param font Font used for painting the labels

   \sa extent()
*/
int QwtScaleDraw::minLength(const QPen &pen, const QFont &font) const
{
    int startDist, endDist;
    getBorderDistHint(font, startDist, endDist);

    const QwtScaleDiv &sd = scaleDiv();

    const uint minorCount =
        sd.ticks(QwtScaleDiv::MinorTick).count() +
        sd.ticks(QwtScaleDiv::MediumTick).count();
    const uint majorCount =
        sd.ticks(QwtScaleDiv::MajorTick).count();

    int lengthForLabels = 0;
    if ( hasComponent(QwtAbstractScaleDraw::Labels) )
    {
        if ( majorCount >= 2 )
            lengthForLabels = minLabelDist(font) * (majorCount - 1);
    }

    int lengthForTicks = 0;
    if ( hasComponent(QwtAbstractScaleDraw::Ticks) )
    {
        const int pw = qwtMax( 1, pen.width() );  // penwidth can be zero
        lengthForTicks = 2 * (majorCount + minorCount) * pw;
    }

    return startDist + endDist + qwtMax(lengthForLabels, lengthForTicks);
}

/*!
   Find the position, where to paint a label

   The position has a distance of majTickLength() + spacing() + 1
   from the backbone. The direction depends on the alignment()

   \param value Value
*/
QPoint QwtScaleDraw::labelPosition( double value) const
{
    const int tval = map().transform(value);
    int dist = spacing() + 1;
    if ( hasComponent(QwtAbstractScaleDraw::Ticks) )
        dist += majTickLength();

    int px = 0;
    int py = 0;

    switch(alignment())
    {
        case RightScale:
        {
            px = d_data->pos.x() + dist;
            py = tval;
            break;
        }
        case LeftScale:
        {
            px = d_data->pos.x() - dist;
            py = tval;
            break;
        }
        case BottomScale:
        {
            px = tval;
            py = d_data->pos.y() + dist;
            break;
        }
        case TopScale:
        {
            px = tval;
            py = d_data->pos.y() - dist;
            break;
        }
    }

    return QPoint(px, py);
}

/*!
   Draw a tick

   \param painter Painter
   \param value Value of the tick
   \param len Lenght of the tick

   \sa drawBackbone(), drawLabel()
*/
void QwtScaleDraw::drawTick(QPainter *painter, double value, int len) const
{
    if ( len <= 0 )
        return;

    int pw2 = qwtMin((int)painter->pen().width(), len) / 2;
    
    QwtScaleMap scaleMap = map();
    const QwtMetricsMap metricsMap = QwtPainter::metricsMap();
    QPoint pos = d_data->pos;

    if ( !metricsMap.isIdentity() )
    {
        /*
           The perfect position of the ticks is important.
           To avoid rounding errors we have to use 
           device coordinates.
         */
        QwtPainter::resetMetricsMap();

        pos = metricsMap.layoutToDevice(pos);
    
        if ( orientation() == Qt::Vertical )
        {
            scaleMap.setPaintInterval(
                metricsMap.layoutToDeviceY((int)scaleMap.p1()),
                metricsMap.layoutToDeviceY((int)scaleMap.p2())
            );
            len = metricsMap.layoutToDeviceX(len);
        }
        else
        {
            scaleMap.setPaintInterval(
                metricsMap.layoutToDeviceX((int)scaleMap.p1()),
                metricsMap.layoutToDeviceX((int)scaleMap.p2())
            );
            len = metricsMap.layoutToDeviceY(len);
        }
    }

    const int tval = scaleMap.transform(value);

    switch(alignment())
    {
        case LeftScale:
        {
#if QT_VERSION < 0x040000
            QwtPainter::drawLine(painter, pos.x() + pw2, tval,
                pos.x() - len - 2 * pw2, tval);
#else
            QwtPainter::drawLine(painter, pos.x() - pw2, tval,
                pos.x() - len, tval);
#endif
            break;
        }

        case RightScale:
        {
#if QT_VERSION < 0x040000
            QwtPainter::drawLine(painter, pos.x(), tval,
                pos.x() + len + pw2, tval);
#else
            QwtPainter::drawLine(painter, pos.x() + pw2, tval,
                pos.x() + len, tval);
#endif
            break;
        }
    
        case BottomScale:
        {
#if QT_VERSION < 0x040000
            QwtPainter::drawLine(painter, tval, pos.y(),
                tval, pos.y() + len + 2 * pw2);
#else
            QwtPainter::drawLine(painter, tval, pos.y() + pw2,
                tval, pos.y() + len);
#endif
            break;
        }

        case TopScale:
        {
#if QT_VERSION < 0x040000
            QwtPainter::drawLine(painter, tval, pos.y() + pw2,
                tval, pos.y() - len - 2 * pw2);
#else
            QwtPainter::drawLine(painter, tval, pos.y() - pw2,
                tval, pos.y() - len);
#endif
            break;
        }
    }
    QwtPainter::setMetricsMap(metricsMap); // restore metrics map
}

/*! 
   Draws the baseline of the scale
   \param painter Painter

   \sa drawTick(), drawLabel()
*/
void QwtScaleDraw::drawBackbone(QPainter *painter) const
{
    const int bw2 = painter->pen().width() / 2;

    const QPoint &pos = d_data->pos;
    const int len = d_data->len - 1;

    switch(alignment())
    {
        case LeftScale:
            QwtPainter::drawLine(painter, pos.x() - bw2,
                pos.y(), pos.x() - bw2, pos.y() + len );
            break;
        case RightScale:
            QwtPainter::drawLine(painter, pos.x() + bw2,
                pos.y(), pos.x() + bw2, pos.y() + len);
            break;
        case TopScale:
            QwtPainter::drawLine(painter, pos.x(), pos.y() - bw2,
                pos.x() + len, pos.y() - bw2);
            break;
        case BottomScale:
            QwtPainter::drawLine(painter, pos.x(), pos.y() + bw2,
                pos.x() + len, pos.y() + bw2);
            break;
    }
}

/*!
  \brief Move the position of the scale

  The meaning of the parameter pos depends on the alignment:
  <dl>
  <dt>QwtScaleDraw::LeftScale
  <dd>The origin is the topmost point of the
      backbone. The backbone is a vertical line. 
      Scale marks and labels are drawn 
      at the left of the backbone.
  <dt>QwtScaleDraw::RightScale
  <dd>The origin is the topmost point of the
      backbone. The backbone is a vertical line. 
      Scale marks and labels are drawn
      at the right of the backbone.
  <dt>QwtScaleDraw::TopScale
  <dd>The origin is the leftmost point of the
      backbone. The backbone is a horizontal line. 
      Scale marks and labels are drawn
      above the backbone.
  <dt>QwtScaleDraw::BottomScale
  <dd>The origin is the leftmost point of the
      backbone. The backbone is a horizontal line 
      Scale marks and labels are drawn
      below the backbone.
  </dl>

  \param pos Origin of the scale

  \sa pos(), setLength()
*/
void QwtScaleDraw::move(const QPoint &pos)
{
    d_data->pos = pos;
    updateMap();
}

/*! 
   \return Origin of the scale
   \sa move(), length()
*/
QPoint QwtScaleDraw::pos() const
{
    return d_data->pos;
}

/*!
  Set the length of the backbone.
  
  The length doesn't include the space needed for
  overlapping labels.

  \sa move(), minLabelDist()
*/
void QwtScaleDraw::setLength(int length)
{
    if ( length >= 0 && length < 10 )
        length = 10;
    if ( length < 0 && length > -10 )
        length = -10;
    
    d_data->len = length;
    updateMap();
}

/*! 
   \return the length of the backbone
   \sa setLength(), pos()
*/
int QwtScaleDraw::length() const
{
    return d_data->len;
}

/*! 
   Draws the label for a major scale tick

   \param painter Painter
   \param value Value

   \sa drawTick(), drawBackbone(), boundingLabelRect()
*/
void QwtScaleDraw::drawLabel(QPainter *painter, double value) const
{
    QwtText lbl = tickLabel(painter->font(), value);
    if ( lbl.isEmpty() )
        return; 

    const QPoint pos = labelPosition(value);

    QSize labelSize = lbl.textSize(painter->font());
    if ( labelSize.height() % 2 )
        labelSize.setHeight(labelSize.height() + 1);
    
    const QwtMatrix m = labelMatrix( pos, labelSize);

    painter->save();
#if QT_VERSION < 0x040000
    painter->setWorldMatrix(m, true);
#else
    painter->setMatrix(m, true);
#endif

    lbl.draw (painter, QRect(QPoint(0, 0), labelSize) );
    painter->restore();
}

/*!
  Find the bounding rect for the label. The coordinates of
  the rect are absolute coordinates ( calculated from pos() ).
  in direction of the tick.

  \param font Font used for painting
  \param value Value

  \sa labelRect()
*/
QRect QwtScaleDraw::boundingLabelRect(const QFont &font, double value) const
{
    QwtText lbl = tickLabel(font, value);
    if ( lbl.isEmpty() )
        return QRect(); 

    const QPoint pos = labelPosition(value);
    QSize labelSize = lbl.textSize(font);
    if ( labelSize.height() % 2 )
        labelSize.setHeight(labelSize.height() + 1);

    const QwtMatrix m = labelMatrix( pos, labelSize);
    return m.mapRect(QRect(QPoint(0, 0), labelSize));
}

/*!
   Calculate the matrix that is needed to paint a label
   depending on its alignment and rotation.

   \param pos Position where to paint the label
   \param size Size of the label

   \sa setLabelAlignment(), setLabelRotation()
*/
QwtMatrix QwtScaleDraw::labelMatrix( 
    const QPoint &pos, const QSize &size) const
{   
    QwtMatrix m;
    m.translate(pos.x(), pos.y());
    m.rotate(labelRotation());
    
    int flags = labelAlignment();
    if ( flags == 0 )
    {
        switch(alignment())
        {
            case RightScale:
            {
                if ( flags == 0 )
                    flags = Qt::AlignRight | Qt::AlignVCenter;
                break;
            }
            case LeftScale:
            {
                if ( flags == 0 )
                    flags = Qt::AlignLeft | Qt::AlignVCenter;
                break;
            }
            case BottomScale:
            {
                if ( flags == 0 )
                    flags = Qt::AlignHCenter | Qt::AlignBottom;
                break;
            }
            case TopScale:
            {
                if ( flags == 0 )
                    flags = Qt::AlignHCenter | Qt::AlignTop;
                break;
            }
        }
    }

    const int w = size.width();
    const int h = size.height();

    int x, y;
    
    if ( flags & Qt::AlignLeft )
        x = -w;
    else if ( flags & Qt::AlignRight )
        x = -(w % 2); 
    else // Qt::AlignHCenter
        x = -(w / 2);
        
    if ( flags & Qt::AlignTop )
        y =  -h ;
    else if ( flags & Qt::AlignBottom )
        y = -(h % 2); 
    else // Qt::AlignVCenter
        y = -(h/2);
        
    m.translate(x, y);
    
    return m;
}   

/*!
  Find the bounding rect for the label. The coordinates of
  the rect are relative to spacing + ticklength from the backbone
  in direction of the tick.

  \param font Font used for painting
  \param value Value
*/
QRect QwtScaleDraw::labelRect(const QFont &font, double value) const
{   
    QwtText lbl = tickLabel(font, value);
    if ( lbl.isEmpty() )
        return QRect(0, 0, 0, 0);

    const QPoint pos = labelPosition(value);

    QSize labelSize = lbl.textSize(font);
    if ( labelSize.height() % 2 )
    {
        labelSize.setHeight(labelSize.height() + 1);
    }

    const QwtMatrix m = labelMatrix(pos, labelSize);

#if 0
    QRect br = QwtMetricsMap::translate(m, QRect(QPoint(0, 0), labelSize));
#else
    QwtPolygon pol(4);
    pol.setPoint(0, 0, 0); 
    pol.setPoint(1, 0, labelSize.height() - 1 );
    pol.setPoint(2, labelSize.width() - 1, 0);
    pol.setPoint(3, labelSize.width() - 1, labelSize.height() - 1 );

    pol = QwtMetricsMap::translate(m, pol);
    QRect br = pol.boundingRect();
#endif

#if QT_VERSION < 0x040000
    br.moveBy(-pos.x(), -pos.y());
#else
    br.translate(-pos.x(), -pos.y());
#endif

    return br;
}

/*!
   Calculate the size that is needed to draw a label

   \param font Label font
   \param value Value
*/
QSize QwtScaleDraw::labelSize(const QFont &font, double value) const
{
    return labelRect(font, value).size();
}

/*!
  Rotate all labels.

  When changing the rotation, it might be necessary to
  adjust the label flags too. Finding a useful combination is
  often the result of try and error.

  \param rotation Angle in degrees. When changing the label rotation,
                  the label flags often needs to be adjusted too.

  \sa setLabelAlignment(), labelRotation(), labelAlignment().

*/
void QwtScaleDraw::setLabelRotation(double rotation)
{
    d_data->labelRotation = rotation;
}

/*!
  \return the label rotation
  \sa setLabelRotation(), labelAlignment()
*/
double QwtScaleDraw::labelRotation() const
{
    return d_data->labelRotation;
}

/*!
  \brief Change the label flags

  Labels are aligned to the point ticklength + spacing away from the backbone.

  The alignment is relative to the orientation of the label text.
  In case of an flags of 0 the label will be aligned  
  depending on the orientation of the scale: 
  
      QwtScaleDraw::TopScale: Qt::AlignHCenter | Qt::AlignTop\n
      QwtScaleDraw::BottomScale: Qt::AlignHCenter | Qt::AlignBottom\n
      QwtScaleDraw::LeftScale: Qt::AlignLeft | Qt::AlignVCenter\n
      QwtScaleDraw::RightScale: Qt::AlignRight | Qt::AlignVCenter\n
  
  Changing the alignment is often necessary for rotated labels.
  
  \param alignment Or'd Qt::AlignmentFlags <see qnamespace.h>

  \sa setLabelRotation(), labelRotation(), labelAlignment()
  \warning The various alignments might be confusing. 
           The alignment of the label is not the alignment
           of the scale and is not the alignment of the flags
           (QwtText::flags()) returned from QwtAbstractScaleDraw::label().
*/    
      
#if QT_VERSION < 0x040000
void QwtScaleDraw::setLabelAlignment(int alignment)
#else
void QwtScaleDraw::setLabelAlignment(Qt::Alignment alignment)
#endif
{
    d_data->labelAlignment = alignment;
}   

/*!
  \return the label flags
  \sa setLabelAlignment(), labelRotation()
*/
#if QT_VERSION < 0x040000
int QwtScaleDraw::labelAlignment() const
#else
Qt::Alignment QwtScaleDraw::labelAlignment() const
#endif
{
    return d_data->labelAlignment;
}

/*!
  \param font Font
  \return the maximum width of a label
*/
int QwtScaleDraw::maxLabelWidth(const QFont &font) const
{
    int maxWidth = 0;

    const QwtValueList &ticks = scaleDiv().ticks(QwtScaleDiv::MajorTick);
    for (uint i = 0; i < (uint)ticks.count(); i++)
    {
        const double v = ticks[i];
        if ( scaleDiv().contains(v) )
        {
            const int w = labelSize(font, ticks[i]).width();
            if ( w > maxWidth )
                maxWidth = w;
        }
    }

    return maxWidth;
}

/*!
  \param font Font
  \return the maximum height of a label
*/
int QwtScaleDraw::maxLabelHeight(const QFont &font) const
{
    int maxHeight = 0;
    
    const QwtValueList &ticks = scaleDiv().ticks(QwtScaleDiv::MajorTick);
    for (uint i = 0; i < (uint)ticks.count(); i++)
    {
        const double v = ticks[i];
        if ( scaleDiv().contains(v) )
        {
            const int h = labelSize(font, ticks[i]).height();
            if ( h > maxHeight )
                maxHeight = h; 
        }       
    }   
    
    return maxHeight;
}   

void QwtScaleDraw::updateMap()
{
    QwtScaleMap &sm = scaleMap();
    if ( orientation() == Qt::Vertical )
        sm.setPaintInterval(d_data->pos.y() + d_data->len, d_data->pos.y());
    else
        sm.setPaintInterval(d_data->pos.x(), d_data->pos.x() + d_data->len);
}
