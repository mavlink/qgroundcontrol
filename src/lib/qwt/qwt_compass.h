/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_COMPASS_H
#define QWT_COMPASS_H 1

#include <qstring.h>
#include <qmap.h>
#include "qwt_dial.h"

#if defined(QWT_TEMPLATEDLL)

#if defined(QT_NO_STL) || QT_VERSION < 0x040000 || QT_VERSION > 0x040001
/*
  Unfortunately Qt 4.0.0/Qt 4.0.1 contains uncompilable 
  code in the STL adaptors of qmap.h. The declaration below 
  instantiates this code resulting in compiler errors. 
  If you really need the map to be exported, remove the condition above
  and fix the qmap.h
*/
// MOC_SKIP_BEGIN
template class QWT_EXPORT QMap<double, QString>;
// MOC_SKIP_END
#endif

#endif


class QwtCompassRose;

/*!
  \brief A Compass Widget

  QwtCompass is a widget to display and enter directions. It consists
  of a scale, an optional needle and rose. 

  \image html dials1.png 

  \note The examples/dials example shows how to use QwtCompass.
*/

class QWT_EXPORT QwtCompass: public QwtDial 
{
    Q_OBJECT

public:
    explicit QwtCompass( QWidget* parent = NULL);
#if QT_VERSION < 0x040000
    explicit QwtCompass(QWidget* parent, const char *name);
#endif
    virtual ~QwtCompass();

    void setRose(QwtCompassRose *rose);
    const QwtCompassRose *rose() const;
    QwtCompassRose *rose();

    const QMap<double, QString> &labelMap() const;
    QMap<double, QString> &labelMap();
    void setLabelMap(const QMap<double, QString> &map);

protected:
    virtual QwtText scaleLabel(double value) const;

    virtual void drawRose(QPainter *, const QPoint &center,
        int radius, double north, QPalette::ColorGroup) const;

    virtual void drawScaleContents(QPainter *, 
        const QPoint &center, int radius) const; 

    virtual void keyPressEvent(QKeyEvent *);

private:
    void initCompass();

    class PrivateData;
    PrivateData *d_data;
};

#endif
