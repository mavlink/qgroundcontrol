/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_DIAL_NEEDLE_H
#define QWT_DIAL_NEEDLE_H 1

#include <qpalette.h>
#include "qwt_global.h"

class QPainter;
class QPoint;

/*!
  \brief Base class for needles that can be used in a QwtDial.

  QwtDialNeedle is a pointer that indicates a value by pointing 
  to a specific direction. 
    
  Qwt is missing a set of good looking needles. 
  Contributions are very welcome.

  \sa QwtDial, QwtCompass
*/

class QWT_EXPORT QwtDialNeedle
{
public:
    QwtDialNeedle();
    virtual ~QwtDialNeedle();

    /*!
        Draw the needle

        \param painter Painter
        \param center Center of the dial, start position for the needle
        \param length Length of the needle
        \param direction Direction of the needle, in degrees counter clockwise
        \param cg Color group, used for painting
    */
    virtual void draw(QPainter *painter, const QPoint &center, 
        int length, double direction, 
        QPalette::ColorGroup cg = QPalette::Active) const = 0;

    virtual void setPalette(const QPalette &);
    const QPalette &palette() const; 

protected:
    static void drawKnob(QPainter *, const QPoint &pos, 
        int width, const QBrush &, bool sunken);

private:
    QPalette d_palette;
};

/*!
  \brief A needle for dial widgets

  The following colors are used:
  - QColorGroup::Mid\n
    Pointer
  - QColorGroup::base\n
    Knob

  \sa QwtDial, QwtCompass
*/

class QWT_EXPORT QwtDialSimpleNeedle: public QwtDialNeedle
{
public:
    //! Style of the needle
    enum Style
    {
        Arrow,
        Ray
    };

    QwtDialSimpleNeedle(Style, bool hasKnob = true, 
        const QColor &mid = Qt::gray, const QColor &base = Qt::darkGray);

    virtual void draw(QPainter *, const QPoint &, int length, 
        double direction, QPalette::ColorGroup = QPalette::Active) const;

    static void drawArrowNeedle(QPainter *, 
        const QPalette&, QPalette::ColorGroup,
        const QPoint &, int length, int width, double direction, 
        bool hasKnob);

    static void drawRayNeedle(QPainter *, 
        const QPalette&, QPalette::ColorGroup,
        const QPoint &, int length, int width, double direction, 
        bool hasKnob);

    void setWidth(int width);
    int width() const;

private:
    Style d_style;
    bool d_hasKnob;
    int d_width;
};

/*!
  \brief A magnet needle for compass widgets

  A magnet needle points to two opposite directions indicating
  north and south.

  The following colors are used:
  - QColorGroup::Light\n
    Used for pointing south
  - QColorGroup::Dark\n
    Used for pointing north
  - QColorGroup::Base\n
    Knob (ThinStyle only)

  \sa QwtDial, QwtCompass
*/

class QWT_EXPORT QwtCompassMagnetNeedle: public QwtDialNeedle
{
public:
    //! Style of the needle
    enum Style
    {
        TriangleStyle,
        ThinStyle
    };
    QwtCompassMagnetNeedle(Style = TriangleStyle,
        const QColor &light = Qt::white, const QColor &dark = Qt::red);

    virtual void draw(QPainter *, const QPoint &, int length, 
        double direction, QPalette::ColorGroup = QPalette::Active) const;

    static void drawTriangleNeedle(QPainter *, 
        const QPalette &, QPalette::ColorGroup,
        const QPoint &, int length, double direction); 

    static void drawThinNeedle(QPainter *,
        const QPalette &, QPalette::ColorGroup,
        const QPoint &, int length, double direction);

protected:
    static void drawPointer(QPainter *painter, const QBrush &brush,
        int colorOffset, const QPoint &center, 
        int length, int width, double direction);

private:
    Style d_style;
};

/*!
  \brief An indicator for the wind direction

  QwtCompassWindArrow shows the direction where the wind comes from.

  - QColorGroup::Light\n
    Used for Style1, or the light half of Style2
  - QColorGroup::Dark\n
    Used for the dark half of Style2

  \sa QwtDial, QwtCompass
*/

class QWT_EXPORT QwtCompassWindArrow: public QwtDialNeedle
{
public:
    //! Style of the arrow
    enum Style
    {
        Style1,
        Style2
    };

    QwtCompassWindArrow(Style, const QColor &light = Qt::white,
        const QColor &dark = Qt::gray);

    virtual void draw(QPainter *, const QPoint &, int length,
        double direction, QPalette::ColorGroup = QPalette::Active) const;

    static void drawStyle1Needle(QPainter *, 
        const QPalette &, QPalette::ColorGroup,
        const QPoint &, int length, double direction);

    static void drawStyle2Needle(QPainter *, 
        const QPalette &, QPalette::ColorGroup,
        const QPoint &, int length, double direction);

private:
    Style d_style;
};

#endif // QWT_DIAL_NEEDLE_H
