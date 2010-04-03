/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <qapplication.h>
#include <qevent.h>
#include <qpainter.h>
#include <qframe.h>
#include <qcursor.h>
#include <qbitmap.h>
#include "qwt_math.h"
#include "qwt_painter.h"
#include "qwt_picker_machine.h"
#include "qwt_picker.h"
#if QT_VERSION < 0x040000
#include <qguardedptr.h>
#else
#include <qpointer.h>
#include <qpaintengine.h>
#endif

class QwtPicker::PickerWidget: public QWidget
{
public:
    enum Type
    {
        RubberBand,
        Text
    };

    PickerWidget(QwtPicker *, QWidget *, Type);
    virtual void updateMask();

    /*
       For a tracker text with a background we can use the background 
       rect as mask. Also for "regular" Qt widgets >= 4.3.0 we
       don't need to mask the text anymore.
     */
    bool d_hasTextMask;

protected:
    virtual void paintEvent(QPaintEvent *);

    QwtPicker *d_picker;
    Type d_type;
};

class QwtPicker::PrivateData
{
public:
    bool enabled;

    QwtPickerMachine *stateMachine;

    int selectionFlags;
    QwtPicker::ResizeMode resizeMode;

    QwtPicker::RubberBand rubberBand;
    QPen rubberBandPen;

    QwtPicker::DisplayMode trackerMode;
    QPen trackerPen;
    QFont trackerFont;

    QwtPolygon selection;
    bool isActive;
    QPoint trackerPosition;

    bool mouseTracking; // used to save previous value

    /*
      On X11 the widget below the picker widgets gets paint events
      with a region that is the bounding rect of the mask, if it is complex.
      In case of (f.e) a CrossRubberBand and a text this creates complete
      repaints of the widget. So we better use two different widgets.
     */
     
#if QT_VERSION < 0x040000
    QGuardedPtr<PickerWidget> rubberBandWidget;
    QGuardedPtr<PickerWidget> trackerWidget;
#else
    QPointer<PickerWidget> rubberBandWidget;
    QPointer<PickerWidget> trackerWidget;
#endif
};

QwtPicker::PickerWidget::PickerWidget(
        QwtPicker *picker, QWidget *parent, Type type):
    QWidget(parent),
    d_hasTextMask(false),
    d_picker(picker),
    d_type(type)
{
#if QT_VERSION >= 0x040000
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::NoFocus);
#else
    setBackgroundMode(Qt::NoBackground);
    setFocusPolicy(QWidget::NoFocus);
    setMouseTracking(true);
#endif
    hide();
}

void QwtPicker::PickerWidget::updateMask()
{
    QRegion mask;

    if ( d_type == RubberBand )
    {
        QBitmap bm(width(), height());
        bm.fill(Qt::color0);

        QPainter painter(&bm);
        QPen pen = d_picker->rubberBandPen();
        pen.setColor(Qt::color1);
        painter.setPen(pen);

        d_picker->drawRubberBand(&painter);

        mask = QRegion(bm);
    }
    if ( d_type == Text )
    {
        d_hasTextMask = true;
#if QT_VERSION >= 0x040300
        if ( !parentWidget()->testAttribute(Qt::WA_PaintOnScreen) )
        {
#if 0
            if ( parentWidget()->paintEngine()->type() != QPaintEngine::OpenGL )
#endif
            {
                // With Qt >= 4.3 drawing of the tracker can be implemented in an
                // easier way, using the textRect as mask. 

                d_hasTextMask = false;
            }
        }
#endif
        
        if ( d_hasTextMask )
        {
            const QwtText label = d_picker->trackerText(
                d_picker->trackerPosition());
            if ( label.testPaintAttribute(QwtText::PaintBackground)
                && label.backgroundBrush().style() != Qt::NoBrush )
            {
#if QT_VERSION >= 0x040300
                if ( label.backgroundBrush().color().alpha() > 0 )
#endif
                // We don't need a text mask, when we have a background
                d_hasTextMask = false;
            }
        }

        if ( d_hasTextMask )
        {
            QBitmap bm(width(), height());
            bm.fill(Qt::color0);

            QPainter painter(&bm);
            painter.setFont(font());

            QPen pen = d_picker->trackerPen();
            pen.setColor(Qt::color1);
            painter.setPen(pen);

            d_picker->drawTracker(&painter);

            mask = QRegion(bm);
        }
        else
        {
            mask = d_picker->trackerRect(font());
        }
    }

#if QT_VERSION < 0x040000
    QWidget *w = parentWidget();
    const bool doUpdate = w->isUpdatesEnabled();
    const Qt::BackgroundMode bgMode = w->backgroundMode();
    w->setUpdatesEnabled(false);
    if ( bgMode != Qt::NoBackground )
        w->setBackgroundMode(Qt::NoBackground);
#endif

    setMask(mask);

#if QT_VERSION < 0x040000
    if ( bgMode != Qt::NoBackground )
        w->setBackgroundMode(bgMode);

    w->setUpdatesEnabled(doUpdate);
#endif

    setShown(!mask.isEmpty());
}

void QwtPicker::PickerWidget::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.setClipRegion(e->region());

    if ( d_type == RubberBand )
    {
        painter.setPen(d_picker->rubberBandPen());
        d_picker->drawRubberBand(&painter);
    }

    if ( d_type == Text )
    {
        /*
           If we have a text mask we simply fill the region of
           the mask. This gives better results for antialiased fonts.
         */
        bool doDrawTracker = !d_hasTextMask;
#if QT_VERSION < 0x040000
        if ( !doDrawTracker && QPainter::redirect(this) )
        {
            // setMask + painter redirection doesn't work
            doDrawTracker = true;
        }
#endif
        if ( doDrawTracker )
        {
            painter.setPen(d_picker->trackerPen());
            d_picker->drawTracker(&painter);
        }
        else
            painter.fillRect(e->rect(), QBrush(d_picker->trackerPen().color()));
    }
}

/*!
  Constructor

  Creates an picker that is enabled, but where selection flag
  is set to NoSelection, rubberband and tracker are disabled.
  
  \param parent Parent widget, that will be observed
 */

QwtPicker::QwtPicker(QWidget *parent):
    QObject(parent)
{
    init(parent, NoSelection, NoRubberBand, AlwaysOff);
}

/*!
  Constructor

  \param selectionFlags Or'd value of SelectionType, RectSelectionType and 
                        SelectionMode
  \param rubberBand Rubberband style
  \param trackerMode Tracker mode
  \param parent Parent widget, that will be observed
 */
QwtPicker::QwtPicker(int selectionFlags, RubberBand rubberBand,
        DisplayMode trackerMode, QWidget *parent):
    QObject(parent)
{
    init(parent, selectionFlags, rubberBand, trackerMode);
}

//! Destructor
QwtPicker::~QwtPicker()
{
    setMouseTracking(false);
    delete d_data->stateMachine;
    delete d_data->rubberBandWidget;
    delete d_data->trackerWidget;
    delete d_data;
}

//! Init the picker, used by the constructors
void QwtPicker::init(QWidget *parent, int selectionFlags, 
    RubberBand rubberBand, DisplayMode trackerMode)
{
    d_data = new PrivateData;

    d_data->rubberBandWidget = NULL;
    d_data->trackerWidget = NULL;

    d_data->rubberBand = rubberBand;
    d_data->enabled = false;
    d_data->resizeMode = Stretch;
    d_data->trackerMode = AlwaysOff;
    d_data->isActive = false;
    d_data->trackerPosition = QPoint(-1, -1);
    d_data->mouseTracking = false;

    d_data->stateMachine = NULL;
    setSelectionFlags(selectionFlags);

    if ( parent )
    {
#if QT_VERSION >= 0x040000
        if ( parent->focusPolicy() == Qt::NoFocus )
            parent->setFocusPolicy(Qt::WheelFocus);
#else
        if ( parent->focusPolicy() == QWidget::NoFocus )
            parent->setFocusPolicy(QWidget::WheelFocus);
#endif

        d_data->trackerFont = parent->font();
        d_data->mouseTracking = parent->hasMouseTracking();
        setEnabled(true);
    }
    setTrackerMode(trackerMode);
}

/*!
   Set a state machine and delete the previous one
*/
void QwtPicker::setStateMachine(QwtPickerMachine *stateMachine)
{
    if ( d_data->stateMachine != stateMachine )
    {
        reset();

        delete d_data->stateMachine;
        d_data->stateMachine = stateMachine;

        if ( d_data->stateMachine )
            d_data->stateMachine->reset();
    }
}

/*!
   Create a state machine depending on the selection flags.

   - PointSelection | ClickSelection\n
     QwtPickerClickPointMachine()
   - PointSelection | DragSelection\n
     QwtPickerDragPointMachine()
   - RectSelection | ClickSelection\n
     QwtPickerClickRectMachine()
   - RectSelection | DragSelection\n
     QwtPickerDragRectMachine()
   - PolygonSelection\n
     QwtPickerPolygonMachine()

   \sa setSelectionFlags()
*/
QwtPickerMachine *QwtPicker::stateMachine(int flags) const
{
    if ( flags & PointSelection )
    {
        if ( flags & ClickSelection )
            return new QwtPickerClickPointMachine;
        else
            return new QwtPickerDragPointMachine;
    }
    if ( flags & RectSelection )
    {
        if ( flags & ClickSelection )
            return new QwtPickerClickRectMachine;
        else
            return new QwtPickerDragRectMachine;
    }
    if ( flags & PolygonSelection )
    {
        return new QwtPickerPolygonMachine();
    }
    return NULL;
}

//! Return the parent widget, where the selection happens
QWidget *QwtPicker::parentWidget()
{
    QObject *obj = parent();
    if ( obj && obj->isWidgetType() )
        return (QWidget *)obj;

    return NULL;
}

//! Return the parent widget, where the selection happens
const QWidget *QwtPicker::parentWidget() const
{
    QObject *obj = parent();
    if ( obj && obj->isWidgetType() )
        return (QWidget *)obj;

    return NULL;
}

/*!
  Set the selection flags

  \param flags Or'd value of SelectionType, RectSelectionType and 
               SelectionMode. The default value is NoSelection.

  \sa selectionFlags(), SelectionType, RectSelectionType, SelectionMode
*/

void QwtPicker::setSelectionFlags(int flags)
{
    d_data->selectionFlags = flags;
    setStateMachine(stateMachine(flags));
}

/*!
  \return Selection flags, an Or'd value of SelectionType, RectSelectionType and
          SelectionMode.
  \sa setSelectionFlags(), SelectionType, RectSelectionType, SelectionMode
*/
int QwtPicker::selectionFlags() const
{
    return d_data->selectionFlags;
}

/*!
  Set the rubberband style 

  \param rubberBand Rubberband style
         The default value is NoRubberBand.

  \sa rubberBand(), RubberBand, setRubberBandPen()
*/
void QwtPicker::setRubberBand(RubberBand rubberBand)
{
    d_data->rubberBand = rubberBand;
}

/*!
  \return Rubberband style
  \sa setRubberBand(), RubberBand, rubberBandPen()
*/
QwtPicker::RubberBand QwtPicker::rubberBand() const
{
    return d_data->rubberBand;
}

/*!
  \brief Set the display mode of the tracker.

  A tracker displays information about current position of
  the cursor as a string. The display mode controls
  if the tracker has to be displayed whenever the observed
  widget has focus and cursor (AlwaysOn), never (AlwaysOff), or
  only when the selection is active (ActiveOnly).
  
  \param mode Tracker display mode

  \warning In case of AlwaysOn, mouseTracking will be enabled
           for the observed widget.
  \sa trackerMode(), DisplayMode
*/

void QwtPicker::setTrackerMode(DisplayMode mode)
{   
    if ( d_data->trackerMode != mode )
    {
        d_data->trackerMode = mode;
        setMouseTracking(d_data->trackerMode == AlwaysOn);
    }
}   

/*!
  \return Tracker display mode
  \sa setTrackerMode(), DisplayMode
*/
QwtPicker::DisplayMode QwtPicker::trackerMode() const
{   
    return d_data->trackerMode;
}   

/*!
  \brief Set the resize mode.

  The resize mode controls what to do with the selected points of an active
  selection when the observed widget is resized.

  Stretch means the points are scaled according to the new
  size, KeepSize means the points remain unchanged.

  The default mode is Stretch.

  \param mode Resize mode
  \sa resizeMode(), ResizeMode
*/
void QwtPicker::setResizeMode(ResizeMode mode)
{
    d_data->resizeMode = mode;
}   

/*!
  \return Resize mode
  \sa setResizeMode(), ResizeMode
*/

QwtPicker::ResizeMode QwtPicker::resizeMode() const
{   
    return d_data->resizeMode;
}

/*!
  \brief En/disable the picker

  When enabled is true an event filter is installed for
  the observed widget, otherwise the event filter is removed.

  \param enabled true or false
  \sa isEnabled(), eventFilter()
*/
void QwtPicker::setEnabled(bool enabled)
{
    if ( d_data->enabled != enabled )
    {
        d_data->enabled = enabled;

        QWidget *w = parentWidget();
        if ( w )
        {
            if ( enabled )
                w->installEventFilter(this);
            else
                w->removeEventFilter(this);
        }

        updateDisplay();
    }
}

/*!
  \return true when enabled, false otherwise
  \sa setEnabled, eventFilter()
*/

bool QwtPicker::isEnabled() const
{
    return d_data->enabled;
}

/*!
  Set the font for the tracker

  \param font Tracker font
  \sa trackerFont(), setTrackerMode(), setTrackerPen()
*/
void QwtPicker::setTrackerFont(const QFont &font)
{
    if ( font != d_data->trackerFont )
    {
        d_data->trackerFont = font;
        updateDisplay();
    }
}

/*!
  \return Tracker font
  \sa setTrackerFont(), trackerMode(), trackerPen()
*/

QFont QwtPicker::trackerFont() const
{
    return d_data->trackerFont;
}

/*!
  Set the pen for the tracker

  \param pen Tracker pen
  \sa trackerPen(), setTrackerMode(), setTrackerFont()
*/
void QwtPicker::setTrackerPen(const QPen &pen)
{
    if ( pen != d_data->trackerPen )
    {
        d_data->trackerPen = pen;
        updateDisplay();
    }
}

/*!
  \return Tracker pen
  \sa setTrackerPen(), trackerMode(), trackerFont()
*/
QPen QwtPicker::trackerPen() const
{
    return d_data->trackerPen;
}

/*!
  Set the pen for the rubberband

  \param pen Rubberband pen
  \sa rubberBandPen(), setRubberBand()
*/
void QwtPicker::setRubberBandPen(const QPen &pen)
{
    if ( pen != d_data->rubberBandPen )
    {
        d_data->rubberBandPen = pen;
        updateDisplay();
    }
}

/*!
  \return Rubberband pen
  \sa setRubberBandPen(), rubberBand()
*/
QPen QwtPicker::rubberBandPen() const
{
    return d_data->rubberBandPen;
}

/*!
   \brief Return the label for a position

   In case of HLineRubberBand the label is the value of the
   y position, in case of VLineRubberBand the value of the x position.
   Otherwise the label contains x and y position separated by a ',' .

   The format for the string conversion is "%d".

   \param pos Position
   \return Converted position as string
*/

QwtText QwtPicker::trackerText(const QPoint &pos) const
{
    QString label;

    switch(rubberBand())
    {
        case HLineRubberBand:
            label.sprintf("%d", pos.y());
            break;
        case VLineRubberBand:
            label.sprintf("%d", pos.x());
            break;
        default:
            label.sprintf("%d, %d", pos.x(), pos.y());
    }
    return label;
}

/*!
   Draw a rubberband , depending on rubberBand() and selectionFlags()

   \param painter Painter, initialized with clip rect 

   \sa rubberBand(), RubberBand, selectionFlags()
*/

void QwtPicker::drawRubberBand(QPainter *painter) const
{
    if ( !isActive() || rubberBand() == NoRubberBand || 
        rubberBandPen().style() == Qt::NoPen )
    {
        return;
    }

    const QRect &pRect = pickRect();
    const QwtPolygon &pa = d_data->selection;

    if ( selectionFlags() & PointSelection )
    {
        if ( pa.count() < 1 )
            return;

        const QPoint pos = pa[0];

        switch(rubberBand())
        {
            case VLineRubberBand:
                QwtPainter::drawLine(painter, pos.x(),
                    pRect.top(), pos.x(), pRect.bottom());
                break;

            case HLineRubberBand:
                QwtPainter::drawLine(painter, pRect.left(), 
                    pos.y(), pRect.right(), pos.y());
                break;

            case CrossRubberBand:
                QwtPainter::drawLine(painter, pos.x(),
                    pRect.top(), pos.x(), pRect.bottom());
                QwtPainter::drawLine(painter, pRect.left(), 
                    pos.y(), pRect.right(), pos.y());
                break;
            default:
                break;
        }
    }

    else if ( selectionFlags() & RectSelection )
    {
        if ( pa.count() < 2 )
            return;

        QPoint p1 = pa[0];
        QPoint p2 = pa[int(pa.count() - 1)];

        if ( selectionFlags() & CenterToCorner )
        {
            p1.setX(p1.x() - (p2.x() - p1.x()));
            p1.setY(p1.y() - (p2.y() - p1.y()));
        }
        else if ( selectionFlags() & CenterToRadius )
        {
            const int radius = qwtMax(qwtAbs(p2.x() - p1.x()), 
                qwtAbs(p2.y() - p1.y()));
            p2.setX(p1.x() + radius);
            p2.setY(p1.y() + radius);
            p1.setX(p1.x() - radius);
            p1.setY(p1.y() - radius);
        }

#if QT_VERSION < 0x040000
        const QRect rect = QRect(p1, p2).normalize();
#else
        const QRect rect = QRect(p1, p2).normalized();
#endif
        switch(rubberBand())
        {
            case EllipseRubberBand:
                QwtPainter::drawEllipse(painter, rect);
                break;
            case RectRubberBand:
                QwtPainter::drawRect(painter, rect);
                break;
            default:
                break;
        }
    }
    else if ( selectionFlags() & PolygonSelection )
    {
        if ( rubberBand() == PolygonRubberBand )
            painter->drawPolyline(pa);
    }
}

/*!
   Draw the tracker

   \param painter Painter
   \sa trackerRect(), trackerText()
*/

void QwtPicker::drawTracker(QPainter *painter) const
{
    const QRect textRect = trackerRect(painter->font());
    if ( !textRect.isEmpty() )
    {
        QwtText label = trackerText(d_data->trackerPosition);
        if ( !label.isEmpty() )
        {
            painter->save();

#if defined(Q_WS_MAC)
            // Antialiased fonts are broken on the Mac.
#if QT_VERSION >= 0x040000 
            painter->setRenderHint(QPainter::TextAntialiasing, false);
#else
            QFont fnt = label.usedFont(painter->font());
            fnt.setStyleStrategy(QFont::NoAntialias);
            label.setFont(fnt);
#endif
#endif
            label.draw(painter, textRect);

            painter->restore();
        }
    }
}

QPoint QwtPicker::trackerPosition() const 
{
    return d_data->trackerPosition;
}

QRect QwtPicker::trackerRect(const QFont &font) const
{
    if ( trackerMode() == AlwaysOff || 
        (trackerMode() == ActiveOnly && !isActive() ) )
    {
        return QRect();
    }

    if ( d_data->trackerPosition.x() < 0 || d_data->trackerPosition.y() < 0 )
        return QRect();

    QwtText text = trackerText(d_data->trackerPosition);
    if ( text.isEmpty() )
        return QRect();

    QRect textRect(QPoint(0, 0), text.textSize(font));

    const QPoint &pos = d_data->trackerPosition;

    int alignment = 0;
    if ( isActive() && d_data->selection.count() > 1 
        && rubberBand() != NoRubberBand )
    {
        const QPoint last = 
            d_data->selection[int(d_data->selection.count()) - 2];

        alignment |= (pos.x() >= last.x()) ? Qt::AlignRight : Qt::AlignLeft;
        alignment |= (pos.y() > last.y()) ? Qt::AlignBottom : Qt::AlignTop;
    }
    else
        alignment = Qt::AlignTop | Qt::AlignRight;

    const int margin = 5;

    int x = pos.x();
    if ( alignment & Qt::AlignLeft )
        x -= textRect.width() + margin;
    else if ( alignment & Qt::AlignRight )
        x += margin;

    int y = pos.y();
    if ( alignment & Qt::AlignBottom )
        y += margin;
    else if ( alignment & Qt::AlignTop )
        y -= textRect.height() + margin;
    
    textRect.moveTopLeft(QPoint(x, y));

    int right = qwtMin(textRect.right(), pickRect().right() - margin);
    int bottom = qwtMin(textRect.bottom(), pickRect().bottom() - margin);
    textRect.moveBottomRight(QPoint(right, bottom));

    int left = qwtMax(textRect.left(), pickRect().left() + margin);
    int top = qwtMax(textRect.top(), pickRect().top() + margin);
    textRect.moveTopLeft(QPoint(left, top));

    return textRect;
}

/*!
  \brief Event filter

  When isEnabled() == true all events of the observed widget are filtered.
  Mouse and keyboard events are translated into widgetMouse- and widgetKey-
  and widgetWheel-events. Paint and Resize events are handled to keep 
  rubberband and tracker up to date.

  \sa event(), widgetMousePressEvent(), widgetMouseReleaseEvent(),
      widgetMouseDoubleClickEvent(), widgetMouseMoveEvent(),
      widgetWheelEvent(), widgetKeyPressEvent(), widgetKeyReleaseEvent()
*/
bool QwtPicker::eventFilter(QObject *o, QEvent *e)
{
    if ( o && o == parentWidget() )
    {
        switch(e->type())
        {
            case QEvent::Resize:
            {
                const QResizeEvent *re = (QResizeEvent *)e;
                if ( d_data->resizeMode == Stretch )
                    stretchSelection(re->oldSize(), re->size());

                if ( d_data->rubberBandWidget )
                    d_data->rubberBandWidget->resize(re->size());
             
                if ( d_data->trackerWidget )
                    d_data->trackerWidget->resize(re->size());
                break;
            }
            case QEvent::Leave:
                widgetLeaveEvent(e);
                break;
            case QEvent::MouseButtonPress:
                widgetMousePressEvent((QMouseEvent *)e);
                break;
            case QEvent::MouseButtonRelease:
                widgetMouseReleaseEvent((QMouseEvent *)e);
                break;
            case QEvent::MouseButtonDblClick:
                widgetMouseDoubleClickEvent((QMouseEvent *)e);
                break;
            case QEvent::MouseMove:
                widgetMouseMoveEvent((QMouseEvent *)e);
                break;
            case QEvent::KeyPress:
                widgetKeyPressEvent((QKeyEvent *)e);
                break;
            case QEvent::KeyRelease:
                widgetKeyReleaseEvent((QKeyEvent *)e);
                break;
            case QEvent::Wheel:
                widgetWheelEvent((QWheelEvent *)e);
                break;
            default:
                break;
        }
    }
    return false;
}

/*!
  Handle a mouse press event for the observed widget.

  Begin and/or end a selection depending on the selection flags.

  \sa QwtPicker, selectionFlags()
  \sa eventFilter(), widgetMouseReleaseEvent(),
      widgetMouseDoubleClickEvent(), widgetMouseMoveEvent(),
      widgetWheelEvent(), widgetKeyPressEvent(), widgetKeyReleaseEvent()
*/
void QwtPicker::widgetMousePressEvent(QMouseEvent *e)
{
    transition(e);
}

/*!
  Handle a mouse move event for the observed widget.

  Move the last point of the selection in case of isActive() == true

  \sa eventFilter(), widgetMousePressEvent(), widgetMouseReleaseEvent(),
      widgetMouseDoubleClickEvent(),
      widgetWheelEvent(), widgetKeyPressEvent(), widgetKeyReleaseEvent()
*/
void QwtPicker::widgetMouseMoveEvent(QMouseEvent *e)
{
    if ( pickRect().contains(e->pos()) )
        d_data->trackerPosition = e->pos();
    else
        d_data->trackerPosition = QPoint(-1, -1);

    if ( !isActive() )
        updateDisplay();

    transition(e);
}

/*!
  Handle a leave event for the observed widget.

  \sa eventFilter(), widgetMousePressEvent(), widgetMouseReleaseEvent(),
      widgetMouseDoubleClickEvent(),
      widgetWheelEvent(), widgetKeyPressEvent(), widgetKeyReleaseEvent()
*/
void QwtPicker::widgetLeaveEvent(QEvent *)   
{
    d_data->trackerPosition = QPoint(-1, -1);
    if ( !isActive() )
        updateDisplay();
}

/*!
  Handle a mouse relase event for the observed widget.

  End a selection depending on the selection flags.

  \sa QwtPicker, selectionFlags()
  \sa eventFilter(), widgetMousePressEvent(), 
      widgetMouseDoubleClickEvent(), widgetMouseMoveEvent(),
      widgetWheelEvent(), widgetKeyPressEvent(), widgetKeyReleaseEvent()
*/
void QwtPicker::widgetMouseReleaseEvent(QMouseEvent *e)
{
    transition(e);
}

/*!
  Handle mouse double click event for the observed widget.

  Empty implementation, does nothing.

  \sa eventFilter(), widgetMousePressEvent(), widgetMouseReleaseEvent(),
      widgetMouseMoveEvent(),
      widgetWheelEvent(), widgetKeyPressEvent(), widgetKeyReleaseEvent()
*/
void QwtPicker::widgetMouseDoubleClickEvent(QMouseEvent *me)
{
    transition(me);
}
    

/*!
  Handle a wheel event for the observed widget.

  Move the last point of the selection in case of isActive() == true

  \sa eventFilter(), widgetMousePressEvent(), widgetMouseReleaseEvent(),
      widgetMouseDoubleClickEvent(), widgetMouseMoveEvent(),
      widgetKeyPressEvent(), widgetKeyReleaseEvent()
*/
void QwtPicker::widgetWheelEvent(QWheelEvent *e)
{
    if ( pickRect().contains(e->pos()) )
        d_data->trackerPosition = e->pos();
    else
        d_data->trackerPosition = QPoint(-1, -1);

    updateDisplay();

    transition(e);
}

/*!
  Handle a key press event for the observed widget.

  Selections can be completely done by the keyboard. The arrow keys
  move the cursor, the abort key aborts a selection. All other keys
  are handled by the current state machine.

  \sa QwtPicker, selectionFlags()
  \sa eventFilter(), widgetMousePressEvent(), widgetMouseReleaseEvent(),
      widgetMouseDoubleClickEvent(), widgetMouseMoveEvent(),
      widgetWheelEvent(), widgetKeyReleaseEvent(), stateMachine(),
      QwtEventPattern::KeyPatternCode
*/
void QwtPicker::widgetKeyPressEvent(QKeyEvent *ke)
{
    int dx = 0;
    int dy = 0;

    int offset = 1;
    if ( ke->isAutoRepeat() )
        offset = 5;

    if ( keyMatch(KeyLeft, ke) )
        dx = -offset;
    else if ( keyMatch(KeyRight, ke) )
        dx = offset;
    else if ( keyMatch(KeyUp, ke) )
        dy = -offset;
    else if ( keyMatch(KeyDown, ke) )
        dy = offset;
    else if ( keyMatch(KeyAbort, ke) )
    {
        reset();
    }
    else
        transition(ke);

    if ( dx != 0 || dy != 0 )
    {
        const QRect rect = pickRect();
        const QPoint pos = parentWidget()->mapFromGlobal(QCursor::pos());

        int x = pos.x() + dx;
        x = qwtMax(rect.left(), x);
        x = qwtMin(rect.right(), x);

        int y = pos.y() + dy;
        y = qwtMax(rect.top(), y);
        y = qwtMin(rect.bottom(), y);

        QCursor::setPos(parentWidget()->mapToGlobal(QPoint(x, y)));
    }
}
 
/*!
  Handle a key release event for the observed widget.

  Passes the event to the state machine.

  \sa eventFilter(), widgetMousePressEvent(), widgetMouseReleaseEvent(),
      widgetMouseDoubleClickEvent(), widgetMouseMoveEvent(),
      widgetWheelEvent(), widgetKeyPressEvent(), stateMachine()
*/
void QwtPicker::widgetKeyReleaseEvent(QKeyEvent *ke)
{
    transition(ke);
}

/*!
  Passes an event to the state machine and executes the resulting 
  commands. Append and Move commands use the current position
  of the cursor (QCursor::pos()).

  \param e Event
*/
void QwtPicker::transition(const QEvent *e)
{
    if ( !d_data->stateMachine )
        return;

    QwtPickerMachine::CommandList commandList =
        d_data->stateMachine->transition(*this, e);

    QPoint pos;
    switch(e->type())
    {
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseMove:
        {
            const QMouseEvent *me = (QMouseEvent *)e;
            pos = me->pos();
            break;
        }
        default:
            pos = parentWidget()->mapFromGlobal(QCursor::pos());
    }

    for ( uint i = 0; i < (uint)commandList.count(); i++ )
    {
        switch(commandList[i])
        {
            case QwtPickerMachine::Begin:
            {
                begin();
                break;
            }
            case QwtPickerMachine::Append:
            {
                append(pos);
                break;
            }
            case QwtPickerMachine::Move:
            {
                move(pos);
                break;
            }
            case QwtPickerMachine::End:
            {
                end();
                break;
            }
        }
    }
}

/*!
  Open a selection setting the state to active

  \sa isActive, end(), append(), move()
*/
void QwtPicker::begin()
{
    if ( d_data->isActive )
        return;

    d_data->selection.resize(0);
    d_data->isActive = true;

    if ( trackerMode() != AlwaysOff )
    {
        if ( d_data->trackerPosition.x() < 0 || d_data->trackerPosition.y() < 0 ) 
        {
            QWidget *w = parentWidget();
            if ( w )
                d_data->trackerPosition = w->mapFromGlobal(QCursor::pos());
        }
    }

    updateDisplay();
    setMouseTracking(true);
}

/*!
  \brief Close a selection setting the state to inactive.

  The selection is validated and maybe fixed by QwtPicker::accept().

  \param ok If true, complete the selection and emit a selected signal
            otherwise discard the selection.
  \return true if the selection is accepted, false otherwise
  \sa isActive, begin(), append(), move(), selected(), accept()
*/
bool QwtPicker::end(bool ok)
{
    if ( d_data->isActive )
    {
        setMouseTracking(false);

        d_data->isActive = false;

        if ( trackerMode() == ActiveOnly )
            d_data->trackerPosition = QPoint(-1, -1);

        if ( ok )
            ok = accept(d_data->selection);

        if ( ok )
            emit selected(d_data->selection);
        else
            d_data->selection.resize(0);

        updateDisplay();
    }
    else
        ok = false;

    return ok;
}

/*!
   Reset the state machine and terminate (end(false)) the selection
*/
void QwtPicker::reset()
{
    if ( d_data->stateMachine )
        d_data->stateMachine->reset();

    if (isActive())
        end(false);
}

/*!
  Append a point to the selection and update rubberband and tracker.
  The appended() signal is emitted.

  \param pos Additional point

  \sa isActive, begin(), end(), move(), appended()
*/
void QwtPicker::append(const QPoint &pos)
{
    if ( d_data->isActive )
    {
        const int idx = d_data->selection.count();
        d_data->selection.resize(idx + 1);
        d_data->selection[idx] = pos;

        updateDisplay();

        emit appended(pos);
    }
}

/*!
  Move the last point of the selection
  The moved() signal is emitted.

  \param pos New position
  \sa isActive, begin(), end(), append()

*/
void QwtPicker::move(const QPoint &pos)
{
    if ( d_data->isActive )
    {
        const int idx = d_data->selection.count() - 1;
        if ( idx >= 0 )
        {
            if ( d_data->selection[idx] != pos )
            {
                d_data->selection[idx] = pos;

                updateDisplay();

                emit moved(pos);
            }
        }
    }
}

bool QwtPicker::accept(QwtPolygon &) const
{
    return true;
}

/*!
  A picker is active between begin() and end().
  \return true if the selection is active.
*/
bool QwtPicker::isActive() const 
{
    return d_data->isActive;
}

//!  Return Selected points
const QwtPolygon &QwtPicker::selection() const
{
    return d_data->selection;
}

/*!
  Scale the selection by the ratios of oldSize and newSize
  The changed() signal is emitted.

  \param oldSize Previous size
  \param newSize Current size

  \sa ResizeMode, setResizeMode(), resizeMode()
*/
void QwtPicker::stretchSelection(const QSize &oldSize, const QSize &newSize)
{
    if ( oldSize.isEmpty() )
    {
        // avoid division by zero. But scaling for small sizes also 
        // doesn't make much sense, because of rounding losses. TODO ...
        return;
    }

    const double xRatio =
        double(newSize.width()) / double(oldSize.width());
    const double yRatio =
        double(newSize.height()) / double(oldSize.height());

    for ( int i = 0; i < int(d_data->selection.count()); i++ )
    {
        QPoint &p = d_data->selection[i];
        p.setX(qRound(p.x() * xRatio));
        p.setY(qRound(p.y() * yRatio));

        emit changed(d_data->selection);
    }
}

/*!
  Set mouse tracking for the observed widget.

  In case of enable is true, the previous value
  is saved, that is restored when enable is false.

  \warning Even when enable is false, mouse tracking might be restored
           to true. When mouseTracking for the observed widget
           has been changed directly by QWidget::setMouseTracking
           while mouse tracking has been set to true, this value can't
           be restored.
*/

void QwtPicker::setMouseTracking(bool enable)
{
    QWidget *widget = parentWidget();
    if ( !widget )
        return;

    if ( enable )
    {
        d_data->mouseTracking = widget->hasMouseTracking();
        widget->setMouseTracking(true);
    }
    else
    {
        widget->setMouseTracking(d_data->mouseTracking);
    }
}

/*!
  Find the area of the observed widget, where selection might happen.

  \return QFrame::contentsRect() if it is a QFrame, QWidget::rect() otherwise.
*/
QRect QwtPicker::pickRect() const
{
    QRect rect;

    const QWidget *widget = parentWidget();
    if ( !widget )
        return rect;

    if ( widget->inherits("QFrame") )
        rect = ((QFrame *)widget)->contentsRect();
    else
        rect = widget->rect();

    return rect;
}

void QwtPicker::updateDisplay()
{
    QWidget *w = parentWidget();

    bool showRubberband = false;
    bool showTracker = false;
    if ( w && w->isVisible() && d_data->enabled )
    {
        if ( rubberBand() != NoRubberBand && isActive() &&
            rubberBandPen().style() != Qt::NoPen )
        {
            showRubberband = true;
        }

        if ( trackerMode() == AlwaysOn ||
            (trackerMode() == ActiveOnly && isActive() ) )
        {
            if ( trackerPen() != Qt::NoPen )
                showTracker = true;
        }
    }

#if QT_VERSION < 0x040000
    QGuardedPtr<PickerWidget> &rw = d_data->rubberBandWidget;
#else
    QPointer<PickerWidget> &rw = d_data->rubberBandWidget;
#endif
    if ( showRubberband )
    {
        if ( rw.isNull() )
        {
            rw = new PickerWidget( this, w, PickerWidget::RubberBand);
            rw->resize(w->size());
        }
        rw->updateMask();
        rw->update(); // Needed, when the mask doesn't change
    }
    else
        delete rw;

#if QT_VERSION < 0x040000
    QGuardedPtr<PickerWidget> &tw = d_data->trackerWidget;
#else
    QPointer<PickerWidget> &tw = d_data->trackerWidget;
#endif
    if ( showTracker )
    {
        if ( tw.isNull() )
        {
            tw = new PickerWidget( this, w, PickerWidget::Text);
            tw->resize(w->size());
        }
        tw->updateMask();
        tw->update(); // Needed, when the mask doesn't change
    }
    else
        delete tw;
}

const QWidget *QwtPicker::rubberBandWidget() const
{
    return d_data->rubberBandWidget;
}

const QWidget *QwtPicker::trackerWidget() const
{
    return d_data->trackerWidget;
}

