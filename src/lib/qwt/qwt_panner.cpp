/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include <qpainter.h>
#include <qpixmap.h>
#include <qevent.h>
#include <qframe.h>
#include <qcursor.h>
#if QT_VERSION < 0x040000
#include <qobjectlist.h>
#endif
#include "qwt_picker.h"
#include "qwt_array.h"
#include "qwt_panner.h"

static QwtArray<QwtPicker *> activePickers(QWidget *w)
{
    QwtArray<QwtPicker *> pickers;

#if QT_VERSION >= 0x040000
    QObjectList children = w->children();
    for ( int i = 0; i < children.size(); i++ )
    {
        QObject *obj = children[i];
        if ( obj->inherits("QwtPicker") )
        {
            QwtPicker *picker = (QwtPicker *)obj;
            if ( picker->isEnabled() )
                pickers += picker;
        }
    }
#else
    QObjectList *children = (QObjectList *)w->children();
    if ( children )
    {
        for ( QObjectListIterator it(*children); it.current(); ++it )
        {
            QObject *obj = (QObject *)it.current();
            if ( obj->inherits("QwtPicker") )
            {
                QwtPicker *picker = (QwtPicker *)obj;
                if ( picker->isEnabled() )
                {
                    pickers.resize(pickers.size() + 1);
                    pickers[int(pickers.size()) - 1] = picker;
                }
            }
        }
    }
#endif

    return pickers;
}

class QwtPanner::PrivateData
{
public:
    PrivateData():
        button(Qt::LeftButton),
        buttonState(Qt::NoButton),
        abortKey(Qt::Key_Escape),
        abortKeyState(Qt::NoButton),
#ifndef QT_NO_CURSOR
        cursor(NULL),
        restoreCursor(NULL),
        hasCursor(false),
#endif
        isEnabled(false)
    {
#if QT_VERSION >= 0x040000
        orientations = Qt::Vertical | Qt::Horizontal;
#else
        orientations[Qt::Vertical] = true;
        orientations[Qt::Horizontal] = true;
#endif
    }

    ~PrivateData()
    {
#ifndef QT_NO_CURSOR
        delete cursor;
        delete restoreCursor;
#endif
    }
        
    int button;
    int buttonState;
    int abortKey;
    int abortKeyState;

    QPoint initialPos;
    QPoint pos;

    QPixmap pixmap;
#ifndef QT_NO_CURSOR
    QCursor *cursor;
    QCursor *restoreCursor;
    bool hasCursor;
#endif
    bool isEnabled;
#if QT_VERSION >= 0x040000
    Qt::Orientations orientations;
#else
    bool orientations[2];
#endif
};

/*!
  Creates an panner that is enabled for the left mouse button.

  \param parent Parent widget to be panned
*/
QwtPanner::QwtPanner(QWidget *parent):
    QWidget(parent)
{
    d_data = new PrivateData();

#if QT_VERSION >= 0x040000
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::NoFocus);
#else
    setBackgroundMode(Qt::NoBackground);
    setFocusPolicy(QWidget::NoFocus);
#endif
    hide();

    setEnabled(true);
}

//! Destructor
QwtPanner::~QwtPanner()
{
    delete d_data;
}

/*!
   Change the mouse button
   The defaults are Qt::LeftButton and Qt::NoButton
*/
void QwtPanner::setMouseButton(int button, int buttonState)
{
    d_data->button = button;
    d_data->buttonState = buttonState;
}

//! Get the mouse button
void QwtPanner::getMouseButton(int &button, int &buttonState) const
{
    button = d_data->button;
    buttonState = d_data->buttonState;
}

/*!
   Change the abort key
   The defaults are Qt::Key_Escape and Qt::NoButton
*/
void QwtPanner::setAbortKey(int key, int state)
{
    d_data->abortKey = key;
    d_data->abortKeyState = state;
}

//! Get the abort key
void QwtPanner::getAbortKey(int &key, int &state) const
{
    key = d_data->abortKey;
    state = d_data->abortKeyState;
}

/*!
   Change the cursor, that is active while panning
   The default is the cursor of the parent widget.

   \param cursor New cursor

   \sa setCursor()
*/
#ifndef QT_NO_CURSOR
void QwtPanner::setCursor(const QCursor &cursor)
{
    d_data->cursor = new QCursor(cursor);
}
#endif

/*!
   \return Cursor that is active while panning
   \sa setCursor()
*/
#ifndef QT_NO_CURSOR
const QCursor QwtPanner::cursor() const
{
    if ( d_data->cursor )
        return *d_data->cursor;

    if ( parentWidget() )
        return parentWidget()->cursor();

    return QCursor();
}
#endif

/*! 
  \brief En/disable the panner
 
  When enabled is true an event filter is installed for
  the observed widget, otherwise the event filter is removed.

  \param on true or false
  \sa isEnabled(), eventFilter()
*/
void QwtPanner::setEnabled(bool on)
{
    if ( d_data->isEnabled != on )
    {
        d_data->isEnabled = on;

        QWidget *w = parentWidget();
        if ( w )
        {
            if ( d_data->isEnabled )
            {
                w->installEventFilter(this);
            }
            else
            {
                w->removeEventFilter(this);
                hide();
            }
        }
    }
}

#if QT_VERSION >= 0x040000
/*!
   Set the orientations, where panning is enabled
   The default value is in both directions: Qt::Horizontal | Qt::Vertical

   /param o Orientation
*/
void QwtPanner::setOrientations(Qt::Orientations o)
{
    d_data->orientations = o;
}

//! Return the orientation, where paning is enabled
Qt::Orientations QwtPanner::orientations() const
{
    return d_data->orientations;
}

#else
void QwtPanner::enableOrientation(Qt::Orientation o, bool enable)
{
    if ( o == Qt::Vertical || o == Qt::Horizontal )
        d_data->orientations[o] = enable;
}
#endif

/*! 
   Return true if a orientatio is enabled
   \sa orientations(), setOrientations()
*/
bool QwtPanner::isOrientationEnabled(Qt::Orientation o) const
{
#if QT_VERSION >= 0x040000
    return d_data->orientations & o;
#else
    if ( o == Qt::Vertical || o == Qt::Horizontal )
        return d_data->orientations[o];
    return false;
#endif
}

/*!
  \return true when enabled, false otherwise
  \sa setEnabled, eventFilter()
*/
bool QwtPanner::isEnabled() const
{
    return d_data->isEnabled;
}

/*!
   \brief Paint event

   Repaint the grabbed pixmap on its current position and
   fill the empty spaces by the background of the parent widget.

   \param pe Paint event
*/
void QwtPanner::paintEvent(QPaintEvent *pe)
{
    QPixmap pm(size());

    QPainter painter(&pm);

    const QColor bg =
#if QT_VERSION < 0x040000
        parentWidget()->palette().color(
            QPalette::Normal, QColorGroup::Background);
#else
        parentWidget()->palette().color(
            QPalette::Normal, QPalette::Background);
#endif

    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(bg));
    painter.drawRect(0, 0, pm.width(), pm.height());

    int dx = d_data->pos.x() - d_data->initialPos.x();
    int dy = d_data->pos.y() - d_data->initialPos.y();

    QRect r(0, 0, d_data->pixmap.width(), d_data->pixmap.height());
    r.moveCenter(QPoint(r.center().x() + dx, r.center().y() + dy));

    painter.drawPixmap(r, d_data->pixmap);
    painter.end();

    painter.begin(this);
    painter.setClipRegion(pe->region());
    painter.drawPixmap(0, 0, pm);
}

/*! 
  \brief Event filter

  When isEnabled() the mouse events of the observed widget are filtered.

  \sa widgetMousePressEvent(), widgetMouseReleaseEvent(),
      widgetMouseMoveEvent()
*/
bool QwtPanner::eventFilter(QObject *o, QEvent *e)
{
    if ( o == NULL || o != parentWidget() )
        return false;

    switch(e->type())
    {
        case QEvent::MouseButtonPress:
        {
            widgetMousePressEvent((QMouseEvent *)e);
            break;
        }
        case QEvent::MouseMove:
        {
            widgetMouseMoveEvent((QMouseEvent *)e);
            break;
        }
        case QEvent::MouseButtonRelease:
        {
            widgetMouseReleaseEvent((QMouseEvent *)e);
            break;
        }
        case QEvent::KeyPress:
        {
            widgetKeyPressEvent((QKeyEvent *)e);
            break;
        }
        case QEvent::KeyRelease:
        {
            widgetKeyReleaseEvent((QKeyEvent *)e);
            break;
        }
        case QEvent::Paint:
        {
            if ( isVisible() )
                return true;
            break;
        }
        default:;
    }

    return false;
}

/*!
  Handle a mouse press event for the observed widget.

  \param me Mouse event
  \sa eventFilter(), widgetMouseReleaseEvent(),
      widgetMouseMoveEvent(),
*/
void QwtPanner::widgetMousePressEvent(QMouseEvent *me)
{
    if ( me->button() != d_data->button )
        return;

    QWidget *w = parentWidget();
    if ( w == NULL )
        return;

#if QT_VERSION < 0x040000
    if ( (me->state() & Qt::KeyButtonMask) !=
        (d_data->buttonState & Qt::KeyButtonMask) )
#else
    if ( (me->modifiers() & Qt::KeyboardModifierMask) !=
        (int)(d_data->buttonState & Qt::KeyboardModifierMask) )
#endif
    {
        return;
    }

#ifndef QT_NO_CURSOR
    showCursor(true);
#endif

    d_data->initialPos = d_data->pos = me->pos();

    QRect cr = parentWidget()->rect();
    if ( parentWidget()->inherits("QFrame") )
    {
        const QFrame* frame = (QFrame*)parentWidget();
        cr = frame->contentsRect();
    }
    setGeometry(cr);

    // We don't want to grab the picker !
    QwtArray<QwtPicker *> pickers = activePickers(parentWidget());
    for ( int i = 0; i < (int)pickers.size(); i++ )
        pickers[i]->setEnabled(false);

    d_data->pixmap = QPixmap::grabWidget(parentWidget(),
        cr.x(), cr.y(), cr.width(), cr.height());

    for ( int i = 0; i < (int)pickers.size(); i++ )
        pickers[i]->setEnabled(true);

    show();
}

/*!
  Handle a mouse move event for the observed widget.

  \param me Mouse event
  \sa eventFilter(), widgetMousePressEvent(), widgetMouseReleaseEvent()
*/
void QwtPanner::widgetMouseMoveEvent(QMouseEvent *me)
{
    if ( !isVisible() )
        return;

    QPoint pos = me->pos();
    if ( !isOrientationEnabled(Qt::Horizontal) )
        pos.setX(d_data->initialPos.x());
    if ( !isOrientationEnabled(Qt::Vertical) )
        pos.setY(d_data->initialPos.y());

    if ( pos != d_data->pos && rect().contains(pos) )
    {
        d_data->pos = pos;
        update();

        emit moved(d_data->pos.x() - d_data->initialPos.x(), 
            d_data->pos.y() - d_data->initialPos.y());
    }
}

/*!
  Handle a mouse release event for the observed widget.

  \param me Mouse event
  \sa eventFilter(), widgetMousePressEvent(),
      widgetMouseMoveEvent(),
*/
void QwtPanner::widgetMouseReleaseEvent(QMouseEvent *me)
{
    if ( isVisible() )
    {
        hide();
#ifndef QT_NO_CURSOR
        showCursor(false);
#endif

        QPoint pos = me->pos();
        if ( !isOrientationEnabled(Qt::Horizontal) )
            pos.setX(d_data->initialPos.x());
        if ( !isOrientationEnabled(Qt::Vertical) )
            pos.setY(d_data->initialPos.y());

        d_data->pixmap = QPixmap();
        d_data->pos = pos;

        if ( d_data->pos != d_data->initialPos )
        {
            emit panned(d_data->pos.x() - d_data->initialPos.x(), 
                d_data->pos.y() - d_data->initialPos.y());
        }
    }
}

/*!
  Handle a key press event for the observed widget.

  \param ke Key event
  \sa eventFilter(), widgetKeyReleaseEvent()
*/
void QwtPanner::widgetKeyPressEvent(QKeyEvent *ke)
{
    if ( ke->key() == d_data->abortKey )
    {
        const bool matched =
#if QT_VERSION < 0x040000
            (ke->state() & Qt::KeyButtonMask) ==
                (d_data->abortKeyState & Qt::KeyButtonMask);
#else
            (ke->modifiers() & Qt::KeyboardModifierMask) ==
                (int)(d_data->abortKeyState & Qt::KeyboardModifierMask);
#endif
        if ( matched )
        {
            hide();
#ifndef QT_NO_CURSOR
            showCursor(false);
#endif
            d_data->pixmap = QPixmap();
        }
    }
}

/*!
  Handle a key release event for the observed widget.

  \param ke Key event
  \sa eventFilter(), widgetKeyReleaseEvent()
*/
void QwtPanner::widgetKeyReleaseEvent(QKeyEvent *)
{
}

#ifndef QT_NO_CURSOR
void QwtPanner::showCursor(bool on)
{
    if ( on == d_data->hasCursor )
        return;

    QWidget *w = parentWidget();
    if ( w == NULL || d_data->cursor == NULL )
        return;

    d_data->hasCursor = on;

    if ( on )
    {
#if QT_VERSION < 0x040000
        if ( w->testWState(WState_OwnCursor) )
#else
        if ( w->testAttribute(Qt::WA_SetCursor) )
#endif
        {
            delete d_data->restoreCursor;
            d_data->restoreCursor = new QCursor(w->cursor());
        }
        w->setCursor(*d_data->cursor);
    }
    else
    {
        if ( d_data->restoreCursor ) 
        {
            w->setCursor(*d_data->restoreCursor);
            delete d_data->restoreCursor;
            d_data->restoreCursor = NULL;
        }
        else
            w->unsetCursor();
    }
}
#endif
