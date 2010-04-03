/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <math.h>
#include <qpainter.h>
#include "qwt_math.h"
#include "qwt_painter.h"
#include "qwt_compass_rose.h"

static QPoint cutPoint(QPoint p11, QPoint p12, QPoint p21, QPoint p22)
{
    double dx1 = p12.x() - p11.x();
    double dy1 = p12.y() - p11.y();
    double dx2 = p22.x() - p21.x();
    double dy2 = p22.y() - p21.y();

    if ( dx1 == 0.0 && dx2 == 0.0 )
        return QPoint();

    if ( dx1 == 0.0 )
    {
        const double m = dy2 / dx2;
        const double t = p21.y() - m * p21.x();
        return QPoint(p11.x(), qRound(m * p11.x() + t));
    }

    if ( dx2 == 0 )
    {
        const double m = dy1 / dx1;
        const double t = p11.y() - m * p11.x();
        return QPoint(p21.x(), qRound(m * p21.x() + t));
    }

    const double m1 = dy1 / dx1;
    const double t1 = p11.y() - m1 * p11.x();

    const double m2 = dy2 / dx2;
    const double t2 = p21.y() - m2 * p21.x();

    if ( m1 == m2 )
        return QPoint();

    const double x = ( t2 - t1 ) / ( m1 - m2 );
    const double y = t1 + m1 * x;

    return QPoint(qRound(x), qRound(y));
}

/*!
   Constructor

   \param numThorns Number of thorns
   \param numThornLevels Number of thorn levels
*/
QwtSimpleCompassRose::QwtSimpleCompassRose(int numThorns, int numThornLevels):
    d_width(0.2),
    d_numThorns(numThorns),
    d_numThornLevels(numThornLevels),
    d_shrinkFactor(0.9)
{
    const QColor dark(128,128,255);
    const QColor light(192,255,255);
    
    QPalette palette;
    for ( int i = 0; i < QPalette::NColorGroups; i++ )
    {
#if QT_VERSION < 0x040000
        palette.setColor((QPalette::ColorGroup)i,
            QColorGroup::Dark, dark);
        palette.setColor((QPalette::ColorGroup)i,
            QColorGroup::Light, light);
#else
        palette.setColor((QPalette::ColorGroup)i,
            QPalette::Dark, dark);
        palette.setColor((QPalette::ColorGroup)i,
            QPalette::Light, light);
#endif
    }

    setPalette(palette);
}

/*!
   Draw the rose

   \param painter Painter
   \param center Center point
   \param radius Radius of the rose
   \param north Position
   \param cg Color group
*/
void QwtSimpleCompassRose::draw(QPainter *painter, const QPoint &center, 
    int radius, double north, QPalette::ColorGroup cg) const
{
#if QT_VERSION < 0x040000
    QColorGroup colorGroup;
    switch(cg)
    {
        case QPalette::Disabled:
            colorGroup = palette().disabled();
        case QPalette::Inactive:
            colorGroup = palette().inactive();
        default:
            colorGroup = palette().active();
    }

    drawRose(painter, colorGroup, center, radius, north, d_width, 
        d_numThorns, d_numThornLevels, d_shrinkFactor);
#else
    QPalette pal = palette();
    pal.setCurrentColorGroup(cg);
    drawRose(painter, pal, center, radius, north, d_width, 
        d_numThorns, d_numThornLevels, d_shrinkFactor);
#endif
}

/*!
   Draw the rose

   \param painter Painter
   \param palette Palette
   \param center Center of the rose
   \param radius Radius of the rose
   \param north Position pointing to north
   \param width Width of the rose
   \param numThorns Number of thorns
   \param numThornLevels Number of thorn levels
   \param shrinkFactor Factor to shrink the thorns with each level
*/
void QwtSimpleCompassRose::drawRose(
    QPainter *painter, 
#if QT_VERSION < 0x040000
    const QColorGroup &cg,
#else
    const QPalette &palette,
#endif
    const QPoint &center, int radius, double north, double width,
    int numThorns, int numThornLevels, double shrinkFactor)
{
    if ( numThorns < 4 )
        numThorns = 4;

    if ( numThorns % 4 )
        numThorns += 4 - numThorns % 4;

    if ( numThornLevels <= 0 )
        numThornLevels = numThorns / 4;

    if ( shrinkFactor >= 1.0 )
        shrinkFactor = 1.0;

    if ( shrinkFactor <= 0.5 )
        shrinkFactor = 0.5;

    painter->save();

    painter->setPen(Qt::NoPen);

    for ( int j = 1; j <= numThornLevels; j++ )
    {
        double step =  pow(2.0, j) * M_PI / (double)numThorns;
        if ( step > M_PI_2 )
            break;

        double r = radius;
        for ( int k = 0; k < 3; k++ )
        {
            if ( j + k < numThornLevels )
                r *= shrinkFactor;
        }

        double leafWidth = r * width;
        if ( 2.0 * M_PI / step > 32 )
            leafWidth = 16;

        const double origin = north / 180.0 * M_PI;
        for ( double angle = origin; 
            angle < 2.0 * M_PI + origin; angle += step)
        {
            const QPoint p = qwtPolar2Pos(center, r, angle);
            QPoint p1 = qwtPolar2Pos(center, leafWidth, angle + M_PI_2);
            QPoint p2 = qwtPolar2Pos(center, leafWidth, angle - M_PI_2);

            QwtPolygon pa(3);
            pa.setPoint(0, center);
            pa.setPoint(1, p);

            QPoint p3 = qwtPolar2Pos(center, r, angle + step / 2.0);
            p1 = cutPoint(center, p3, p1, p);
            pa.setPoint(2, p1);
#if QT_VERSION < 0x040000
            painter->setBrush(cg.brush(QColorGroup::Dark));
#else
            painter->setBrush(palette.brush(QPalette::Dark));
#endif
            painter->drawPolygon(pa);

            QPoint p4 = qwtPolar2Pos(center, r, angle - step / 2.0);
            p2 = cutPoint(center, p4, p2, p);

            pa.setPoint(2, p2);
#if QT_VERSION < 0x040000
            painter->setBrush(cg.brush(QColorGroup::Light));
#else
            painter->setBrush(palette.brush(QPalette::Light));
#endif
            painter->drawPolygon(pa);
        }
    }
    painter->restore();
}

/*!
   Set the width of the rose heads. Lower value make thinner heads.
   The range is limited from 0.03 to 0.4.

   \param width Width
*/

void QwtSimpleCompassRose::setWidth(double width) 
{
   d_width = width;
   if (d_width < 0.03) 
        d_width = 0.03;

   if (d_width > 0.4) 
        d_width = 0.4;
}

/*!
  Set the number of thorns on one level
  The number is aligned to a multiple of 4, with a minimum of 4 

  \param numThorns Number of thorns
  \sa numThorns(), setNumThornLevels()
*/
void QwtSimpleCompassRose::setNumThorns(int numThorns) 
{
    if ( numThorns < 4 )
        numThorns = 4;

    if ( numThorns % 4 )
        numThorns += 4 - numThorns % 4;

    d_numThorns = numThorns;
}

/*!
   \return Number of thorns
   \sa setNumThorns(), setNumThornLevels()
*/
int QwtSimpleCompassRose::numThorns() const
{
   return d_numThorns;
}

/*!
  Set the of thorns levels

  \param numThornLevels Number of thorns levels
  \sa setNumThorns(), numThornLevels()
*/
void QwtSimpleCompassRose::setNumThornLevels(int numThornLevels) 
{
    d_numThornLevels = numThornLevels;
}

/*!
   \return Number of thorn levels
   \sa setNumThorns(), setNumThornLevels()
*/
int QwtSimpleCompassRose::numThornLevels() const
{
    return d_numThornLevels;
}
