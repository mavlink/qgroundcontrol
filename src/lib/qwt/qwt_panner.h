/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_PANNER_H
#define QWT_PANNER_H 1

#include <qnamespace.h>
#include <qwidget.h>
#include "qwt_global.h"

class QCursor;

/*!
  \brief QwtPanner provides panning of a widget

  QwtPanner grabs the contents of a widget, that can be dragged
  in all directions. The offset between the start and the end position
  is emitted by the panned signal.
   
  QwtPanner grabs the content of the widget into a pixmap and moves
  the pixmap around, without initiating any repaint events for the widget.
  Areas, that are not part of content are not painted  while panning
  in in process. This makes panning fast enough for widgets, where 
  repaints are too slow for mouse movements. 

  For widgets, where repaints are very fast it might be better to
  implement panning manually by mapping mouse events into paint events.
*/
class QWT_EXPORT QwtPanner: public QWidget
{
    Q_OBJECT

public:
    QwtPanner(QWidget* parent);
    virtual ~QwtPanner();

    void setEnabled(bool);
    bool isEnabled() const;

    void setMouseButton(int button, int buttonState = Qt::NoButton);
    void getMouseButton(int &button, int &buttonState) const;
    void setAbortKey(int key, int state = Qt::NoButton);
    void getAbortKey(int &key, int &state) const;

    void setCursor(const QCursor &);
    const QCursor cursor() const;

#if QT_VERSION >= 0x040000
    void setOrientations(Qt::Orientations);
    Qt::Orientations orientations() const;
#else
    void enableOrientation(Qt::Orientation, bool enable);
#endif

    bool isOrientationEnabled(Qt::Orientation) const;

    virtual bool eventFilter(QObject *, QEvent *);

signals:
    /*!
      Signal emitted, when panning is done

      \param dx Offset in horizontal direction
      \param dy Offset in vertical direction
    */
    void panned(int dx, int dy);

    /*!
      Signal emitted, while the widget moved, but panning
      is not finished.

      \param dx Offset in horizontal direction
      \param dy Offset in vertical direction
    */
    void moved(int dx, int dy);

protected:
    virtual void widgetMousePressEvent(QMouseEvent *);
    virtual void widgetMouseReleaseEvent(QMouseEvent *);
    virtual void widgetMouseMoveEvent(QMouseEvent *);
    virtual void widgetKeyPressEvent(QKeyEvent *);
    virtual void widgetKeyReleaseEvent(QKeyEvent *);

    virtual void paintEvent(QPaintEvent *);

private:
#ifndef QT_NO_CURSOR
    void showCursor(bool);
#endif

    class PrivateData;
    PrivateData *d_data;
};

#endif
