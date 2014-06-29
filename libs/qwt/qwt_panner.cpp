/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_panner.h"
#include "qwt_picker.h"
#include "qwt_painter.h"
#include <qpainter.h>
#include <qpixmap.h>
#include <qevent.h>
#include <qcursor.h>
#include <qbitmap.h>

static QVector<QwtPicker *> qwtActivePickers( QWidget *w )
{
    QVector<QwtPicker *> pickers;

    QObjectList children = w->children();
    for ( int i = 0; i < children.size(); i++ )
    {
        QwtPicker *picker = qobject_cast<QwtPicker *>( children[i] );
        if ( picker && picker->isEnabled() )
            pickers += picker;
    }

    return pickers;
}

class QwtPanner::PrivateData
{
public:
    PrivateData():
        button( Qt::LeftButton ),
        buttonModifiers( Qt::NoModifier ),
        abortKey( Qt::Key_Escape ),
        abortKeyModifiers( Qt::NoModifier ),
#ifndef QT_NO_CURSOR
        cursor( NULL ),
        restoreCursor( NULL ),
        hasCursor( false ),
#endif
        isEnabled( false )
    {
        orientations = Qt::Vertical | Qt::Horizontal;
    }

    ~PrivateData()
    {
#ifndef QT_NO_CURSOR
        delete cursor;
        delete restoreCursor;
#endif
    }

    Qt::MouseButton button;
    Qt::KeyboardModifiers  buttonModifiers;

    int abortKey;
    Qt::KeyboardModifiers abortKeyModifiers;

    QPoint initialPos;
    QPoint pos;

    QPixmap pixmap;
    QBitmap contentsMask;

#ifndef QT_NO_CURSOR
    QCursor *cursor;
    QCursor *restoreCursor;
    bool hasCursor;
#endif
    bool isEnabled;
    Qt::Orientations orientations;
};

/*!
  Creates an panner that is enabled for the left mouse button.

  \param parent Parent widget to be panned
*/
QwtPanner::QwtPanner( QWidget *parent ):
    QWidget( parent )
{
    d_data = new PrivateData();

    setAttribute( Qt::WA_TransparentForMouseEvents );
    setAttribute( Qt::WA_NoSystemBackground );
    setFocusPolicy( Qt::NoFocus );
    hide();

    setEnabled( true );
}

//! Destructor
QwtPanner::~QwtPanner()
{
    delete d_data;
}

/*!
   Change the mouse button and modifiers used for panning
   The defaults are Qt::LeftButton and Qt::NoModifier
*/
void QwtPanner::setMouseButton( Qt::MouseButton button,
    Qt::KeyboardModifiers modifiers )
{
    d_data->button = button;
    d_data->buttonModifiers = modifiers;
}

//! Get mouse button and modifiers used for panning
void QwtPanner::getMouseButton( Qt::MouseButton &button,
    Qt::KeyboardModifiers &modifiers ) const
{
    button = d_data->button;
    modifiers = d_data->buttonModifiers;
}

/*!
   Change the abort key
   The defaults are Qt::Key_Escape and Qt::NoModifiers

   \param key Key ( See Qt::Keycode )
   \param modifiers Keyboard modifiers
*/
void QwtPanner::setAbortKey( int key, 
    Qt::KeyboardModifiers modifiers )
{
    d_data->abortKey = key;
    d_data->abortKeyModifiers = modifiers;
}

//! Get the abort key and modifiers
void QwtPanner::getAbortKey( int &key, 
    Qt::KeyboardModifiers &modifiers ) const
{
    key = d_data->abortKey;
    modifiers = d_data->abortKeyModifiers;
}

/*!
   Change the cursor, that is active while panning
   The default is the cursor of the parent widget.

   \param cursor New cursor

   \sa setCursor()
*/
#ifndef QT_NO_CURSOR
void QwtPanner::setCursor( const QCursor &cursor )
{
    d_data->cursor = new QCursor( cursor );
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
void QwtPanner::setEnabled( bool on )
{
    if ( d_data->isEnabled != on )
    {
        d_data->isEnabled = on;

        QWidget *w = parentWidget();
        if ( w )
        {
            if ( d_data->isEnabled )
            {
                w->installEventFilter( this );
            }
            else
            {
                w->removeEventFilter( this );
                hide();
            }
        }
    }
}

/*!
   Set the orientations, where panning is enabled
   The default value is in both directions: Qt::Horizontal | Qt::Vertical

   /param o Orientation
*/
void QwtPanner::setOrientations( Qt::Orientations o )
{
    d_data->orientations = o;
}

//! Return the orientation, where paning is enabled
Qt::Orientations QwtPanner::orientations() const
{
    return d_data->orientations;
}

/*!
   \return True if an orientation is enabled
   \sa orientations(), setOrientations()
*/
bool QwtPanner::isOrientationEnabled( Qt::Orientation o ) const
{
    return d_data->orientations & o;
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
void QwtPanner::paintEvent( QPaintEvent *pe )
{
    int dx = d_data->pos.x() - d_data->initialPos.x();
    int dy = d_data->pos.y() - d_data->initialPos.y();

    QRect r( 0, 0, d_data->pixmap.width(), d_data->pixmap.height() );
    r.moveCenter( QPoint( r.center().x() + dx, r.center().y() + dy ) );

    QPixmap pm( size() );
    QwtPainter::fillPixmap( parentWidget(), pm );

    QPainter painter( &pm );

    if ( !d_data->contentsMask.isNull() )
    {
        QPixmap masked = d_data->pixmap;
        masked.setMask( d_data->contentsMask );
        painter.drawPixmap( r, masked );
    }
    else
    {
        painter.drawPixmap( r, d_data->pixmap );
    }

    painter.end();

    if ( !d_data->contentsMask.isNull() )
        pm.setMask( d_data->contentsMask );

    painter.begin( this );
    painter.setClipRegion( pe->region() );
    painter.drawPixmap( 0, 0, pm );
}

/*!
  \brief Calculate a mask for the contents of the panned widget

  Sometimes only parts of the contents of a widget should be
  panned. F.e. for a widget with a styled background with rounded borders
  only the area inside of the border should be panned.

  \return An empty bitmap, indicating no mask
*/
QBitmap QwtPanner::contentsMask() const
{
    return QBitmap();
}

/*!
  Grab the widget into a pixmap.
  \return Grabbed pixmap
*/
QPixmap QwtPanner::grab() const
{
#if QT_VERSION >= 0x050000
    return parentWidget()->grab( parentWidget()->rect() );
#else
    return QPixmap::grabWidget( parentWidget() );
#endif
}

/*!
  \brief Event filter

  When isEnabled() is true mouse events of the
  observed widget are filtered.

  \param object Object to be filtered
  \param event Event

  \return Always false, beside for paint events for the
          parent widget.

  \sa widgetMousePressEvent(), widgetMouseReleaseEvent(),
      widgetMouseMoveEvent()
*/
bool QwtPanner::eventFilter( QObject *object, QEvent *event )
{
    if ( object == NULL || object != parentWidget() )
        return false;

    switch ( event->type() )
    {
        case QEvent::MouseButtonPress:
        {
            widgetMousePressEvent( static_cast<QMouseEvent *>( event ) );
            break;
        }
        case QEvent::MouseMove:
        {
            widgetMouseMoveEvent( static_cast<QMouseEvent *>( event ) );
            break;
        }
        case QEvent::MouseButtonRelease:
        {
            widgetMouseReleaseEvent( static_cast<QMouseEvent *>( event ) );
            break;
        }
        case QEvent::KeyPress:
        {
            widgetKeyPressEvent( static_cast<QKeyEvent *>( event ) );
            break;
        }
        case QEvent::KeyRelease:
        {
            widgetKeyReleaseEvent( static_cast<QKeyEvent *>( event ) );
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

  \param mouseEvent Mouse event
  \sa eventFilter(), widgetMouseReleaseEvent(),
      widgetMouseMoveEvent(),
*/
void QwtPanner::widgetMousePressEvent( QMouseEvent *mouseEvent )
{
    if ( ( mouseEvent->button() != d_data->button )
        || ( mouseEvent->modifiers() != d_data->buttonModifiers ) )
    {
        return;
    }

    QWidget *w = parentWidget();
    if ( w == NULL )
        return;

#ifndef QT_NO_CURSOR
    showCursor( true );
#endif

    d_data->initialPos = d_data->pos = mouseEvent->pos();

    setGeometry( parentWidget()->rect() );

    // We don't want to grab the picker !
    QVector<QwtPicker *> pickers = qwtActivePickers( parentWidget() );
    for ( int i = 0; i < pickers.size(); i++ )
        pickers[i]->setEnabled( false );

    d_data->pixmap = grab();
    d_data->contentsMask = contentsMask();

    for ( int i = 0; i < pickers.size(); i++ )
        pickers[i]->setEnabled( true );

    show();
}

/*!
  Handle a mouse move event for the observed widget.

  \param mouseEvent Mouse event
  \sa eventFilter(), widgetMousePressEvent(), widgetMouseReleaseEvent()
*/
void QwtPanner::widgetMouseMoveEvent( QMouseEvent *mouseEvent )
{
    if ( !isVisible() )
        return;

    QPoint pos = mouseEvent->pos();
    if ( !isOrientationEnabled( Qt::Horizontal ) )
        pos.setX( d_data->initialPos.x() );
    if ( !isOrientationEnabled( Qt::Vertical ) )
        pos.setY( d_data->initialPos.y() );

    if ( pos != d_data->pos && rect().contains( pos ) )
    {
        d_data->pos = pos;
        update();

        Q_EMIT moved( d_data->pos.x() - d_data->initialPos.x(),
            d_data->pos.y() - d_data->initialPos.y() );
    }
}

/*!
  Handle a mouse release event for the observed widget.

  \param mouseEvent Mouse event
  \sa eventFilter(), widgetMousePressEvent(),
      widgetMouseMoveEvent(),
*/
void QwtPanner::widgetMouseReleaseEvent( QMouseEvent *mouseEvent )
{
    if ( isVisible() )
    {
        hide();
#ifndef QT_NO_CURSOR
        showCursor( false );
#endif

        QPoint pos = mouseEvent->pos();
        if ( !isOrientationEnabled( Qt::Horizontal ) )
            pos.setX( d_data->initialPos.x() );
        if ( !isOrientationEnabled( Qt::Vertical ) )
            pos.setY( d_data->initialPos.y() );

        d_data->pixmap = QPixmap();
        d_data->contentsMask = QBitmap();
        d_data->pos = pos;

        if ( d_data->pos != d_data->initialPos )
        {
            Q_EMIT panned( d_data->pos.x() - d_data->initialPos.x(),
                d_data->pos.y() - d_data->initialPos.y() );
        }
    }
}

/*!
  Handle a key press event for the observed widget.

  \param keyEvent Key event
  \sa eventFilter(), widgetKeyReleaseEvent()
*/
void QwtPanner::widgetKeyPressEvent( QKeyEvent *keyEvent )
{
    if ( ( keyEvent->key() == d_data->abortKey )
        && ( keyEvent->modifiers() == d_data->abortKeyModifiers ) )
    {
        hide();

#ifndef QT_NO_CURSOR
        showCursor( false );
#endif
        d_data->pixmap = QPixmap();
    }
}

/*!
  Handle a key release event for the observed widget.

  \param keyEvent Key event
  \sa eventFilter(), widgetKeyReleaseEvent()
*/
void QwtPanner::widgetKeyReleaseEvent( QKeyEvent *keyEvent )
{
    Q_UNUSED( keyEvent );
}

#ifndef QT_NO_CURSOR
void QwtPanner::showCursor( bool on )
{
    if ( on == d_data->hasCursor )
        return;

    QWidget *w = parentWidget();
    if ( w == NULL || d_data->cursor == NULL )
        return;

    d_data->hasCursor = on;

    if ( on )
    {
        if ( w->testAttribute( Qt::WA_SetCursor ) )
        {
            delete d_data->restoreCursor;
            d_data->restoreCursor = new QCursor( w->cursor() );
        }
        w->setCursor( *d_data->cursor );
    }
    else
    {
        if ( d_data->restoreCursor )
        {
            w->setCursor( *d_data->restoreCursor );
            delete d_data->restoreCursor;
            d_data->restoreCursor = NULL;
        }
        else
            w->unsetCursor();
    }
}
#endif
