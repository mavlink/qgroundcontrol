/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_COMPASS_ROSE_H
#define QWT_COMPASS_ROSE_H 1

#include <qpalette.h>
#include "qwt_global.h"

class QPainter;

/*!
  \brief Abstract base class for a compass rose
*/
class QWT_EXPORT QwtCompassRose
{
public:
    virtual ~QwtCompassRose() {};

    virtual void setPalette(const QPalette &p) { d_palette = p; }
    const QPalette &palette() const { return d_palette; }

    /*!
        Draw the rose

        \param painter Painter
        \param center Center point
        \param radius Radius of the rose
        \param north Position
        \param colorGroup Color group
     */
    virtual void draw(QPainter *painter, const QPoint &center, 
        int radius, double north, 
        QPalette::ColorGroup colorGroup = QPalette::Active) const = 0;

private:
    QPalette d_palette;
};

/*!
  \brief A simple rose for QwtCompass
*/
class QWT_EXPORT QwtSimpleCompassRose: public QwtCompassRose
{
public:
    QwtSimpleCompassRose(int numThorns = 8, int numThornLevels = -1);

    void setWidth(double w);
    double width() const { return d_width; }

    void setNumThorns(int count);
    int numThorns() const;

    void setNumThornLevels(int count);
    int numThornLevels() const;

    void setShrinkFactor(double factor) { d_shrinkFactor = factor; }
    double shrinkFactor() const { return d_shrinkFactor; }

    virtual void draw(QPainter *, const QPoint &center, int radius, 
        double north, QPalette::ColorGroup = QPalette::Active) const;

    static void drawRose(QPainter *, 
#if QT_VERSION < 0x040000
        const QColorGroup &,
#else
        const QPalette &,
#endif
        const QPoint &center, int radius, double origin, double width, 
        int numThorns, int numThornLevels, double shrinkFactor);

private:
    double d_width;     
    int d_numThorns;        
    int d_numThornLevels; 
    double d_shrinkFactor;
};

#endif // QWT_COMPASS_ROSE_H
