/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_ABSTRACT_SLIDER_H
#define QWT_ABSTRACT_SLIDER_H

#include <qwidget.h>
#include "qwt_global.h"
#include "qwt_double_range.h"

/*!
  \brief An abstract base class for slider widgets

  QwtAbstractSlider is a base class for
  slider widgets. It handles mouse events
  and updates the slider's value accordingly. Derived classes
  only have to implement the getValue() and 
  getScrollMode() members, and should react to a
  valueChange(), which normally requires repainting. 
*/

class QWT_EXPORT QwtAbstractSlider : public QWidget, public QwtDoubleRange
{
    Q_OBJECT 
    Q_PROPERTY( bool readOnly READ isReadOnly WRITE setReadOnly )
    Q_PROPERTY( bool valid READ isValid WRITE setValid )
    Q_PROPERTY( double mass READ mass WRITE setMass )
#ifndef Q_MOC_RUN // Qt3 moc
#define QWT_PROPERTY Q_PROPERTY
    Q_PROPERTY( Orientation orientation 
        READ orientation WRITE setOrientation )
#else // Qt4 moc
// MOC_SKIP_BEGIN
    Q_PROPERTY( Qt::Orientation orientation 
        READ orientation WRITE setOrientation )
// MOC_SKIP_END
#endif

public:
    /*! 
      Scroll mode
      \sa getScrollMode()
     */
    enum ScrollMode 
    { 
        ScrNone, 
        ScrMouse, 
        ScrTimer, 
        ScrDirect, 
        ScrPage 
    };
    
    explicit QwtAbstractSlider(Qt::Orientation, QWidget *parent = NULL);
    virtual ~QwtAbstractSlider();

    void setUpdateTime(int t);
    void stopMoving();
    void setTracking(bool enable);
    
    virtual void setMass(double val);
    virtual double mass() const;

#if QT_VERSION >= 0x040000
    virtual void setOrientation(Qt::Orientation o);
    Qt::Orientation orientation() const;
#else
    virtual void setOrientation(Orientation o);
    Orientation orientation() const;
#endif

    bool isReadOnly() const;

    /* 
        Wrappers for QwtDblRange::isValid/QwtDblRange::setValid made
        to be available as Q_PROPERTY in the designer.
    */

    /*! 
      \sa QwtDblRange::isValid
    */
    bool isValid() const { return QwtDoubleRange::isValid(); }

    /*! 
      \sa QwtDblRange::isValid
    */
    void setValid(bool valid) { QwtDoubleRange::setValid(valid); }

public slots:
    virtual void setValue(double val);
    virtual void fitValue(double val);
    virtual void incValue(int steps);

    virtual void setReadOnly(bool); 

signals:

    /*!
      \brief Notify a change of value.

      In the default setting 
      (tracking enabled), this signal will be emitted every 
      time the value changes ( see setTracking() ). 
      \param value new value
    */
    void valueChanged(double value);

    /*!
      This signal is emitted when the user presses the 
      movable part of the slider (start ScrMouse Mode).
    */
    void sliderPressed();

    /*!
      This signal is emitted when the user releases the 
      movable part of the slider.
    */

    void sliderReleased();
    /*!
      This signal is emitted when the user moves the
      slider with the mouse.
      \param value new value
    */
    void sliderMoved(double value);
    
protected:
    virtual void setPosition(const QPoint &);
    virtual void valueChange();

    virtual void timerEvent(QTimerEvent *e);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void wheelEvent(QWheelEvent *e);

  /*!
    \brief Determine the value corresponding to a specified poind

    This is an abstract virtual function which is called when
    the user presses or releases a mouse button or moves the
    mouse. It has to be implemented by the derived class.
    \param p point 
  */
    virtual double getValue(const QPoint & p) = 0;
  /*!
    \brief Determine what to do when the user presses a mouse button.

    This function is abstract and has to be implemented by derived classes.
    It is called on a mousePress event. The derived class can determine
    what should happen next in dependence of the position where the mouse
    was pressed by returning scrolling mode and direction. QwtAbstractSlider
    knows the following modes:<dl>
    <dt>QwtAbstractSlider::ScrNone
    <dd>Scrolling switched off. Don't change the value.
    <dt>QwtAbstractSlider::ScrMouse
    <dd>Change the value while the user keeps the
        button pressed and moves the mouse.
    <dt>QwtAbstractSlider::ScrTimer
    <dd>Automatic scrolling. Increment the value
        in the specified direction as long as
    the user keeps the button pressed.
    <dt>QwtAbstractSlider::ScrPage
    <dd>Automatic scrolling. Same as ScrTimer, but
        increment by page size.</dl>

    \param p point where the mouse was pressed
    \retval scrollMode The scrolling mode
    \retval direction  direction: 1, 0, or -1.
  */
    virtual void getScrollMode( const QPoint &p,
                  int &scrollMode, int &direction) = 0;

    void setMouseOffset(double);
    double mouseOffset() const;

    int scrollMode() const;

private:
    void buttonReleased();

    class PrivateData;
    PrivateData *d_data;
};

#endif
