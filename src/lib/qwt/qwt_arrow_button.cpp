/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qpainter.h>
#include <qstyle.h>
#include <qevent.h>
#include "qwt_math.h"
#include "qwt_polygon.h"
#include "qwt_arrow_button.h"

static const int MaxNum = 3;
static const int Margin = 2;
static const int Spacing = 1;

class QwtArrowButton::PrivateData
{
public:
    int num;
    Qt::ArrowType arrowType;
};


#if QT_VERSION >= 0x040000
#include <qstyleoption.h>
static QStyleOptionButton styleOpt(const QwtArrowButton* btn)
{
    QStyleOptionButton option;
    option.init(btn);
    option.features = QStyleOptionButton::None;
    if (btn->isFlat())
        option.features |= QStyleOptionButton::Flat;
    if (btn->menu())
        option.features |= QStyleOptionButton::HasMenu;
    if (btn->autoDefault() || btn->isDefault())
        option.features |= QStyleOptionButton::AutoDefaultButton;
    if (btn->isDefault())
        option.features |= QStyleOptionButton::DefaultButton;
    if (btn->isDown())
        option.state |= QStyle::State_Sunken;
    if (!btn->isFlat() && !btn->isDown())
        option.state |= QStyle::State_Raised;

    return option;
}
#endif

/*!
  \param num Number of arrows
  \param arrowType see Qt::ArowType in the Qt docs.
  \param parent Parent widget
*/
QwtArrowButton::QwtArrowButton(int num, 
        Qt::ArrowType arrowType, QWidget *parent): 
    QPushButton(parent)
{
    d_data = new PrivateData;
    d_data->num = qwtLim(num, 1, MaxNum);
    d_data->arrowType = arrowType;

    setAutoRepeat(true);
    setAutoDefault(false);

    switch(d_data->arrowType)
    {
        case Qt::LeftArrow:
        case Qt::RightArrow:
            setSizePolicy(QSizePolicy::Expanding, 
                QSizePolicy::Fixed);
            break;
        default:
            setSizePolicy(QSizePolicy::Fixed, 
                QSizePolicy::Expanding);
    }
}

//! Destructor
QwtArrowButton::~QwtArrowButton()
{
    delete d_data;
    d_data = NULL;
}

/*!
  \brief The direction of the arrows
*/
Qt::ArrowType QwtArrowButton::arrowType() const 
{ 
    return d_data->arrowType; 
}

/*!
  \brief The number of arrows
*/
int QwtArrowButton::num() const 
{ 
    return d_data->num; 
}

/*!
  \return the bounding rect for the label
*/
QRect QwtArrowButton::labelRect() const
{
    const int m = Margin;

    QRect r = rect();
    r.setRect(r.x() + m, r.y() + m, 
        r.width() - 2 * m, r.height() - 2 * m);

    if ( isDown() )
    {
        int ph, pv;
#if QT_VERSION < 0x040000
        ph = style().pixelMetric(
            QStyle::PM_ButtonShiftHorizontal, this);
        pv = style().pixelMetric(
            QStyle::PM_ButtonShiftVertical, this);
        r.moveBy(ph, pv);
#else
        QStyleOptionButton option = styleOpt(this);
        ph = style()->pixelMetric(
            QStyle::PM_ButtonShiftHorizontal, &option, this);
        pv = style()->pixelMetric(
            QStyle::PM_ButtonShiftVertical, &option, this);
        r.translate(ph, pv);
#endif
    }

    return r;
}

#if QT_VERSION >= 0x040000
/*!
   Paint event handler
   \param event Paint event
*/
void QwtArrowButton::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);
    QPainter painter(this);
    drawButtonLabel(&painter);
}
#endif

/*!
  \brief Draw the button label

  \param painter Painter
  \sa The Qt Manual on QPushButton
*/
void QwtArrowButton::drawButtonLabel(QPainter *painter)
{
    const bool isVertical = d_data->arrowType == Qt::UpArrow ||
        d_data->arrowType == Qt::DownArrow;

    const QRect r = labelRect();
    QSize boundingSize = labelRect().size();
    if ( isVertical )
        boundingSize.transpose();
        
    const int w = 
        (boundingSize.width() - (MaxNum - 1) * Spacing) / MaxNum;

    QSize arrow = arrowSize(Qt::RightArrow, 
        QSize(w, boundingSize.height()));

    if ( isVertical )
        arrow.transpose();

    QRect contentsSize; // aligned rect where to paint all arrows
    if ( d_data->arrowType == Qt::LeftArrow || d_data->arrowType == Qt::RightArrow )
    {
        contentsSize.setWidth(d_data->num * arrow.width() 
            + (d_data->num - 1) * Spacing);
        contentsSize.setHeight(arrow.height());
    }
    else
    {
        contentsSize.setWidth(arrow.width());
        contentsSize.setHeight(d_data->num * arrow.height() 
            + (d_data->num - 1) * Spacing);
    }

    QRect arrowRect(contentsSize);
    arrowRect.moveCenter(r.center());
    arrowRect.setSize(arrow);

    painter->save();
    for (int i = 0; i < d_data->num; i++)
    {
        drawArrow(painter, arrowRect, d_data->arrowType);

        int dx = 0;
        int dy = 0;

        if ( isVertical )
            dy = arrow.height() + Spacing;
        else
            dx = arrow.width() + Spacing;

#if QT_VERSION >= 0x040000
        arrowRect.translate(dx, dy);
#else
        arrowRect.moveBy(dx, dy);
#endif
    }
    painter->restore();

    if ( hasFocus() )
    {
#if QT_VERSION >= 0x040000
        QStyleOptionFocusRect option;
        option.init(this);
        option.backgroundColor = palette().color(QPalette::Background);

        style()->drawPrimitive(QStyle::PE_FrameFocusRect, 
            &option, painter, this);
#else
        const QRect focusRect =  
            style().subRect(QStyle::SR_PushButtonFocusRect, this);
        style().drawPrimitive(QStyle::PE_FocusRect, painter,
            focusRect, colorGroup());
#endif
    }
}

/*!
    Draw an arrow int a bounding rect

    \param painter Painter
    \param r Rectangle where to paint the arrow
    \param arrowType Arrow type
*/
void QwtArrowButton::drawArrow(QPainter *painter, 
    const QRect &r, Qt::ArrowType arrowType) const 
{
    QwtPolygon pa(3);

    switch(arrowType)
    {
        case Qt::UpArrow:
            pa.setPoint(0, r.bottomLeft());
            pa.setPoint(1, r.bottomRight());
            pa.setPoint(2, r.center().x(), r.top());
            break;
        case Qt::DownArrow:
            pa.setPoint(0, r.topLeft());
            pa.setPoint(1, r.topRight());
            pa.setPoint(2, r.center().x(), r.bottom());
            break;
        case Qt::RightArrow:
            pa.setPoint(0, r.topLeft());
            pa.setPoint(1, r.bottomLeft());
            pa.setPoint(2, r.right(), r.center().y());
            break;
        case Qt::LeftArrow:
            pa.setPoint(0, r.topRight());
            pa.setPoint(1, r.bottomRight());
            pa.setPoint(2, r.left(), r.center().y());
            break;
        default:
            break;
    }

    painter->save();
#if QT_VERSION < 0x040000
    painter->setPen(colorGroup().buttonText());
    painter->setBrush(colorGroup().brush(QColorGroup::ButtonText));
#else
    painter->setPen(palette().color(QPalette::ButtonText));
    painter->setBrush(palette().brush(QPalette::ButtonText));
#endif
    painter->drawPolygon(pa);
    painter->restore();
}

/*!
  \return a size hint
*/
QSize QwtArrowButton::sizeHint() const
{
    return minimumSizeHint();
}

/*!
  \brief Return a minimum size hint
*/
QSize QwtArrowButton::minimumSizeHint() const
{
    const QSize asz = arrowSize(Qt::RightArrow, QSize()); 

    QSize sz(
        2 * Margin + (MaxNum - 1) * Spacing + MaxNum * asz.width(),
        2 * Margin + asz.height()
    );

    if ( d_data->arrowType == Qt::UpArrow || d_data->arrowType == Qt::DownArrow )
        sz.transpose();

#if QT_VERSION >= 0x040000
    QStyleOption styleOption;
    styleOption.init(this);

    const QSize hsz = style()->sizeFromContents(QStyle::CT_PushButton, 
        &styleOption, sz, this);
#if QT_VERSION < 0x040300
    if ( hsz.width() != 80 ) // avoid a bug in the Cleanlooks style
#endif
        sz = hsz;

#else
    sz = style().sizeFromContents(QStyle::CT_PushButton, this, sz);
#endif

    return sz;
}

/*!
   Calculate the size for a arrow that fits into a rect of a given size

   \param arrowType Arrow type
   \param boundingSize Bounding size
   \return Size of the arrow
*/
QSize QwtArrowButton::arrowSize(Qt::ArrowType arrowType,
    const QSize &boundingSize) const
{
    QSize bs = boundingSize;
    if ( arrowType == Qt::UpArrow || arrowType == Qt::DownArrow )
        bs.transpose();
        
    const int MinLen = 2;
    const QSize sz = bs.expandedTo(
        QSize(MinLen, 2 * MinLen - 1)); // minimum

    int w = sz.width();
    int h = 2 * w - 1;

    if ( h > sz.height() )
    {
        h = sz.height();
        w = (h + 1) / 2;
    }

    QSize arrSize(w, h);
    if ( arrowType == Qt::UpArrow || arrowType == Qt::DownArrow )
        arrSize.transpose();

    return arrSize;
}

/*!
  \brief autoRepeat for the space keys
*/
void QwtArrowButton::keyPressEvent(QKeyEvent *e)
{
    if ( e->isAutoRepeat() && e->key() == Qt::Key_Space )
        emit clicked();

    QPushButton::keyPressEvent(e);
}
