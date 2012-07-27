/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_MAGNIFIER_H
#define QWT_MAGNIFIER_H 1

#include "qwt_global.h"
#include <qobject.h>

class QWidget;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;

/*!
  \brief QwtMagnifier provides zooming, by magnifying in steps.

  Using QwtMagnifier a plot can be zoomed in/out in steps using
  keys, the mouse wheel or moving a mouse button in vertical direction.
*/
class QWT_EXPORT QwtMagnifier: public QObject
{
    Q_OBJECT

public:
    explicit QwtMagnifier(QWidget *);
    virtual ~QwtMagnifier();

    QWidget *parentWidget();
    const QWidget *parentWidget() const;

    void setEnabled(bool);
    bool isEnabled() const;

    // mouse
    void setMouseFactor(double);
    double mouseFactor() const;

    void setMouseButton(int button, int buttonState = Qt::NoButton);
    void getMouseButton(int &button, int &buttonState) const;

    // mouse wheel
    void setWheelFactor(double);
    double wheelFactor() const;

    void setWheelButtonState(int buttonState);
    int wheelButtonState() const;

    // keyboard
    void setKeyFactor(double);
    double keyFactor() const;

    void setZoomInKey(int key, int modifiers);
    void getZoomInKey(int &key, int &modifiers) const;

    void setZoomOutKey(int key, int modifiers);
    void getZoomOutKey(int &key, int &modifiers) const;

    virtual bool eventFilter(QObject *, QEvent *);

protected:
    virtual void rescale(double factor) = 0;

    virtual void widgetMousePressEvent(QMouseEvent *);
    virtual void widgetMouseReleaseEvent(QMouseEvent *);
    virtual void widgetMouseMoveEvent(QMouseEvent *);
    virtual void widgetWheelEvent(QWheelEvent *);
    virtual void widgetKeyPressEvent(QKeyEvent *);
    virtual void widgetKeyReleaseEvent(QKeyEvent *);

private:
    class PrivateData;
    PrivateData *d_data;
};

#endif
