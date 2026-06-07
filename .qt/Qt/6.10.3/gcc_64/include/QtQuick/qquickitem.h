// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKITEM_H
#define QQUICKITEM_H

#include <QtQuick/qtquickglobal.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlcomponent.h>

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/qproperty.h>
#include <QtGui/qevent.h>
#include <QtGui/qfont.h>
#include <QtGui/qaccessible.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickTransformPrivate;
class Q_QUICK_EXPORT QQuickTransform : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)
public:
    explicit QQuickTransform(QObject *parent = nullptr);
    ~QQuickTransform() override;

    void appendToItem(QQuickItem *);
    void prependToItem(QQuickItem *);

    virtual void applyTo(QMatrix4x4 *matrix) const = 0;

protected Q_SLOTS:
    void update();

protected:
    QQuickTransform(QQuickTransformPrivate &dd, QObject *parent);

private:
    Q_DECLARE_PRIVATE(QQuickTransform)
};

class QCursor;
class QQuickItemLayer;
class QQuickState;
class QQuickAnchorLine;
class QQuickTransition;
class QQuickKeyEvent;
class QQuickAnchors;
class QQuickItemPrivate;
class QQuickWindow;
class QTouchEvent;
class QSGNode;
class QSGTransformNode;
class QSGTextureProvider;
class QQuickItemGrabResult;
class QQuickPalette;

class Q_QUICK_EXPORT QQuickItem : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QQuickItem *parent READ parentItem WRITE setParentItem NOTIFY parentChanged DESIGNABLE false FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQmlListProperty<QObject> data READ data DESIGNABLE false)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQmlListProperty<QObject> resources READ resources DESIGNABLE false)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQmlListProperty<QQuickItem> children READ children NOTIFY childrenChanged DESIGNABLE false)

    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged BINDABLE bindableX FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged BINDABLE bindableY FINAL)
    Q_PROPERTY(qreal z READ z WRITE setZ NOTIFY zChanged FINAL)
    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged RESET resetWidth BINDABLE bindableWidth FINAL)
    Q_PROPERTY(qreal height READ height WRITE setHeight NOTIFY heightChanged RESET resetHeight BINDABLE bindableHeight FINAL)

    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged FINAL)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickPalette *palette READ palette WRITE setPalette RESET resetPalette NOTIFY paletteChanged REVISION(6, 0))
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQmlListProperty<QQuickItem> visibleChildren READ visibleChildren NOTIFY visibleChildrenChanged DESIGNABLE false)

    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQmlListProperty<QQuickState> states READ states DESIGNABLE false)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQmlListProperty<QQuickTransition> transitions READ transitions DESIGNABLE false)
    Q_PROPERTY(QString state READ state WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(QRectF childrenRect READ childrenRect NOTIFY childrenRectChanged DESIGNABLE false FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickAnchors * anchors READ anchors DESIGNABLE false CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickAnchorLine left READ left CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickAnchorLine right READ right CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickAnchorLine horizontalCenter READ horizontalCenter CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickAnchorLine top READ top CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickAnchorLine bottom READ bottom CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickAnchorLine verticalCenter READ verticalCenter CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickAnchorLine baseline READ baseline CONSTANT FINAL)
    Q_PROPERTY(qreal baselineOffset READ baselineOffset WRITE setBaselineOffset NOTIFY baselineOffsetChanged)

    Q_PROPERTY(bool clip READ clip WRITE setClip NOTIFY clipChanged)

    Q_PROPERTY(bool focus READ hasFocus WRITE setFocus NOTIFY focusChanged FINAL)
    Q_PROPERTY(bool activeFocus READ hasActiveFocus NOTIFY activeFocusChanged FINAL)
    Q_PROPERTY(bool activeFocusOnTab READ activeFocusOnTab WRITE setActiveFocusOnTab NOTIFY activeFocusOnTabChanged FINAL REVISION(2, 1))

    Q_PROPERTY(Qt::FocusPolicy focusPolicy READ focusPolicy WRITE setFocusPolicy NOTIFY focusPolicyChanged REVISION(6, 7))

    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(qreal scale READ scale WRITE setScale NOTIFY scaleChanged)
    Q_PROPERTY(TransformOrigin transformOrigin READ transformOrigin WRITE setTransformOrigin NOTIFY transformOriginChanged)
    Q_PROPERTY(QPointF transformOriginPoint READ transformOriginPoint)  // deprecated - see QTBUG-26423
    Q_PROPERTY(QQmlListProperty<QQuickTransform> transform READ transform DESIGNABLE false FINAL)

    Q_PROPERTY(bool smooth READ smooth WRITE setSmooth NOTIFY smoothChanged)
    Q_PROPERTY(bool antialiasing READ antialiasing WRITE setAntialiasing NOTIFY antialiasingChanged RESET resetAntialiasing)
    Q_PROPERTY(qreal implicitWidth READ implicitWidth WRITE setImplicitWidth NOTIFY implicitWidthChanged)
    Q_PROPERTY(qreal implicitHeight READ implicitHeight WRITE setImplicitHeight NOTIFY implicitHeightChanged)
    Q_PROPERTY(QObject *containmentMask READ containmentMask WRITE setContainmentMask NOTIFY containmentMaskChanged REVISION(2, 11))

#if QT_CONFIG(quick_shadereffect)
    Q_PRIVATE_PROPERTY(QQuickItem::d_func(), QQuickItemLayer *layer READ layer DESIGNABLE false CONSTANT FINAL)
#endif

    Q_CLASSINFO("DefaultProperty", "data")
    Q_CLASSINFO("ParentProperty", "parent")
    Q_CLASSINFO("qt_QmlJSWrapperFactoryMethod", "_q_createJSWrapper(QQmlV4ExecutionEnginePtr)")
    QML_NAMED_ELEMENT(Item)
    QML_ADDED_IN_VERSION(2, 0)

public:
    enum Flag {
        ItemClipsChildrenToShape  = 0x01,
#if QT_CONFIG(im)
        ItemAcceptsInputMethod    = 0x02,
#endif
        ItemIsFocusScope          = 0x04,
        ItemHasContents           = 0x08,
        ItemAcceptsDrops          = 0x10,
        ItemIsViewport            = 0x20,
        ItemObservesViewport      = 0x40,
        // Remember to increment the size of QQuickItemPrivate::flags
    };
    Q_DECLARE_FLAGS(Flags, Flag)
    Q_FLAG(Flags)

    enum ItemChange {
        ItemChildAddedChange,      // value.item
        ItemChildRemovedChange,    // value.item
        ItemSceneChange,           // value.window
        ItemVisibleHasChanged,     // value.boolValue
        ItemParentHasChanged,      // value.item
        ItemOpacityHasChanged,     // value.realValue
        ItemActiveFocusHasChanged, // value.boolValue
        ItemRotationHasChanged,    // value.realValue
        ItemAntialiasingHasChanged, // value.boolValue
        ItemDevicePixelRatioHasChanged, // value.realValue
        ItemEnabledHasChanged,     // value.boolValue
        ItemScaleHasChanged,       // value.realValue
        ItemTransformHasChanged,   // value.item
    };
    Q_ENUM(ItemChange)

    union ItemChangeData {
        ItemChangeData(QQuickItem *v) : item(v) {}
        ItemChangeData(QQuickWindow *v) : window(v) {}
        ItemChangeData(qreal v) : realValue(v) {}
        ItemChangeData(bool v) : boolValue(v) {}

        QQuickItem *item;
        QQuickWindow *window;
        qreal realValue;
        bool boolValue;
    };

    enum TransformOrigin {
        TopLeft, Top, TopRight,
        Left, Center, Right,
        BottomLeft, Bottom, BottomRight
    };
    Q_ENUM(TransformOrigin)

    explicit QQuickItem(QQuickItem *parent = nullptr);
    ~QQuickItem() override;

    QQuickWindow *window() const;
    QQuickItem *parentItem() const;
    void setParentItem(QQuickItem *parent);
    void stackBefore(const QQuickItem *);
    void stackAfter(const QQuickItem *);

    QRectF childrenRect();
    QList<QQuickItem *> childItems() const;

    bool clip() const;
    void setClip(bool);

    QString state() const;
    void setState(const QString &);

    qreal baselineOffset() const;
    void setBaselineOffset(qreal);

    QQmlListProperty<QQuickTransform> transform();

    qreal x() const;
    qreal y() const;
    QPointF position() const;
    void setX(qreal);
    void setY(qreal);
    void setPosition(const QPointF &);
    QBindable<qreal> bindableX();
    QBindable<qreal> bindableY();

    qreal width() const;
    void setWidth(qreal);
    void resetWidth();
    void setImplicitWidth(qreal);
    qreal implicitWidth() const;
    QBindable<qreal> bindableWidth();

    qreal height() const;
    void setHeight(qreal);
    void resetHeight();
    void setImplicitHeight(qreal);
    qreal implicitHeight() const;
    QBindable<qreal> bindableHeight();

    QSizeF size() const;
    void setSize(const QSizeF &size);

    TransformOrigin transformOrigin() const;
    void setTransformOrigin(TransformOrigin);
    QPointF transformOriginPoint() const;
    void setTransformOriginPoint(const QPointF &);

    qreal z() const;
    void setZ(qreal);

    qreal rotation() const;
    void setRotation(qreal);
    qreal scale() const;
    void setScale(qreal);

    qreal opacity() const;
    void setOpacity(qreal);

    bool isVisible() const;
    void setVisible(bool);

    bool isEnabled() const;
    void setEnabled(bool);

    bool smooth() const;
    void setSmooth(bool);

    bool activeFocusOnTab() const;
    void setActiveFocusOnTab(bool);

    bool antialiasing() const;
    void setAntialiasing(bool);
    void resetAntialiasing();

    Flags flags() const;
    void setFlag(Flag flag, bool enabled = true);
    void setFlags(Flags flags);

    virtual QRectF boundingRect() const;
    virtual QRectF clipRect() const;
    QQuickItem *viewportItem() const;

    bool hasActiveFocus() const;
    bool hasFocus() const;
    void setFocus(bool);
    void setFocus(bool focus, Qt::FocusReason reason);
    bool isFocusScope() const;
    QQuickItem *scopedFocusItem() const;

    Qt::FocusPolicy focusPolicy() const;
    void setFocusPolicy(Qt::FocusPolicy policy);

    bool isAncestorOf(const QQuickItem *child) const;

    Qt::MouseButtons acceptedMouseButtons() const;
    void setAcceptedMouseButtons(Qt::MouseButtons buttons);
    bool acceptHoverEvents() const;
    void setAcceptHoverEvents(bool enabled);
    bool acceptTouchEvents() const;
    void setAcceptTouchEvents(bool accept);

#if QT_CONFIG(cursor)
    QCursor cursor() const;
    void setCursor(const QCursor &cursor);
    void unsetCursor();
#endif

    bool isUnderMouse() const;
    void grabMouse();
    void ungrabMouse();
    bool keepMouseGrab() const;
    void setKeepMouseGrab(bool);
    bool filtersChildMouseEvents() const;
    void setFiltersChildMouseEvents(bool filter);

    void grabTouchPoints(const QList<int> &ids);
    void ungrabTouchPoints();
    bool keepTouchGrab() const;
    void setKeepTouchGrab(bool);

    // implemented in qquickitemgrabresult.cpp
    Q_REVISION(2, 4) Q_INVOKABLE bool grabToImage(const QJSValue &callback, const QSize &targetSize = QSize());
    QSharedPointer<QQuickItemGrabResult> grabToImage(const QSize &targetSize = QSize());

    Q_INVOKABLE virtual bool contains(const QPointF &point) const;
    QObject *containmentMask() const;
    void setContainmentMask(QObject *mask);

    QTransform itemTransform(QQuickItem *, bool *) const;
    QPointF mapToScene(const QPointF &point) const;
    QRectF mapRectToItem(const QQuickItem *item, const QRectF &rect) const;
    QRectF mapRectToScene(const QRectF &rect) const;
    QPointF mapFromScene(const QPointF &point) const;
    QRectF mapRectFromItem(const QQuickItem *item, const QRectF &rect) const;
    QRectF mapRectFromScene(const QRectF &rect) const;

    void polish();

#if QT_DEPRECATED_SINCE(6, 5)
    QT_DEPRECATED_VERSION_X_6_5("Use typed overload or mapRectFromItem")
    void mapFromItem(QQmlV4FunctionPtr) const;
#endif
    Q_INVOKABLE QPointF mapFromItem(const QQuickItem *item, const QPointF &point) const;
    // overloads mainly exist for QML
    Q_INVOKABLE QPointF mapFromItem(const QQuickItem *item, qreal x, qreal y);
    Q_INVOKABLE QRectF mapFromItem(const QQuickItem *item, const QRectF &rect) const;
    Q_INVOKABLE QRectF mapFromItem(const QQuickItem *item, qreal x, qreal y, qreal width, qreal height) const;

#if QT_DEPRECATED_SINCE(6, 5)
    QT_DEPRECATED_VERSION_X_6_5("Use typed overload or mapRectToItem")
    void mapToItem(QQmlV4FunctionPtr) const;
#endif
    Q_INVOKABLE QPointF mapToItem(const QQuickItem *item, const QPointF &point) const;
    // overloads mainly exist for QML
    Q_INVOKABLE QPointF mapToItem(const QQuickItem *item, qreal x, qreal y);
    Q_INVOKABLE QRectF mapToItem(const QQuickItem *item, const QRectF &rect) const;
    Q_INVOKABLE QRectF mapToItem(const QQuickItem *item, qreal x, qreal y, qreal width, qreal height) const;

#if QT_DEPRECATED_SINCE(6, 5)
    QT_DEPRECATED_VERSION_X_6_5("Use the typed overload")
    Q_REVISION(2, 7) void mapFromGlobal(QQmlV4FunctionPtr) const;
#endif
    Q_REVISION(2, 7) Q_INVOKABLE QPointF mapFromGlobal(qreal x, qreal y) const;
    // overload mainly exists for QML
    Q_REVISION(2, 7) Q_INVOKABLE QPointF mapFromGlobal(const QPointF &point) const;

#if QT_DEPRECATED_SINCE(6, 5)
    QT_DEPRECATED_VERSION_X_6_5("Use the typed overload")
    Q_REVISION(2, 7) void mapToGlobal(QQmlV4FunctionPtr) const;
#endif
    Q_REVISION(2, 7) Q_INVOKABLE  QPointF mapToGlobal(qreal x, qreal y) const;
    // overload only exist for QML
    Q_REVISION(2, 7) Q_INVOKABLE  QPointF mapToGlobal(const QPointF &point) const;

    Q_INVOKABLE void forceActiveFocus();
    Q_INVOKABLE void forceActiveFocus(Qt::FocusReason reason);
    Q_REVISION(2, 1) Q_INVOKABLE QQuickItem *nextItemInFocusChain(bool forward = true);
    Q_INVOKABLE QQuickItem *childAt(qreal x, qreal y) const;
    Q_REVISION(6, 3) Q_INVOKABLE void ensurePolished();

    Q_REVISION(6, 3) Q_INVOKABLE void dumpItemTree() const;

#if QT_CONFIG(im)
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;
#endif

    struct UpdatePaintNodeData {
       QSGTransformNode *transformNode;
    private:
       friend class QQuickWindowPrivate;
       UpdatePaintNodeData();
    };

    virtual bool isTextureProvider() const;
    virtual QSGTextureProvider *textureProvider() const;

public Q_SLOTS:
    void update();

Q_SIGNALS:
    void childrenRectChanged(const QRectF &);
    void baselineOffsetChanged(qreal);
    void stateChanged(const QString &);
    void focusChanged(bool);
    void activeFocusChanged(bool);
    Q_REVISION(6, 7) void focusPolicyChanged(Qt::FocusPolicy);
    Q_REVISION(2, 1) void activeFocusOnTabChanged(bool);
    void parentChanged(QQuickItem *);
    void transformOriginChanged(TransformOrigin);
    void smoothChanged(bool);
    void antialiasingChanged(bool);
    void clipChanged(bool);
    Q_REVISION(2, 1) void windowChanged(QQuickWindow* window);

    void childrenChanged();
    void opacityChanged();
    void enabledChanged();
    void visibleChanged();
    void visibleChildrenChanged();
    void rotationChanged();
    void scaleChanged();

    void xChanged();
    void yChanged();
    void widthChanged();
    void heightChanged();
    void zChanged();
    void implicitWidthChanged();
    void implicitHeightChanged();
    Q_REVISION(2, 11) void containmentMaskChanged();

    Q_REVISION(6, 0) void paletteChanged();
    Q_REVISION(6, 0) void paletteCreated();

protected:
    bool event(QEvent *) override;

    bool isComponentComplete() const;
    virtual void itemChange(ItemChange, const ItemChangeData &);
    virtual void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry);

#if QT_CONFIG(im)
    void updateInputMethod(Qt::InputMethodQueries queries = Qt::ImQueryInput);
#endif

    bool widthValid() const; // ### better name?
    bool heightValid() const; // ### better name?
    void setImplicitSize(qreal, qreal);

    void classBegin() override;
    void componentComplete() override;

    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
#if QT_CONFIG(im)
    virtual void inputMethodEvent(QInputMethodEvent *);
#endif
    virtual void focusInEvent(QFocusEvent *);
    virtual void focusOutEvent(QFocusEvent *);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    virtual void mouseUngrabEvent(); // XXX todo - params?
    virtual void touchUngrabEvent();
#if QT_CONFIG(wheelevent)
    virtual void wheelEvent(QWheelEvent *event);
#endif
    virtual void touchEvent(QTouchEvent *event);
    virtual void hoverEnterEvent(QHoverEvent *event);
    virtual void hoverMoveEvent(QHoverEvent *event);
    virtual void hoverLeaveEvent(QHoverEvent *event);
#if QT_CONFIG(quick_draganddrop)
    virtual void dragEnterEvent(QDragEnterEvent *);
    virtual void dragMoveEvent(QDragMoveEvent *);
    virtual void dragLeaveEvent(QDragLeaveEvent *);
    virtual void dropEvent(QDropEvent *);
#endif
    virtual bool childMouseEventFilter(QQuickItem *, QEvent *);
#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0)
    virtual bool contextMenuEvent(QContextMenuEvent *event);
#endif

    virtual QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);
    virtual void releaseResources();
    virtual void updatePolish();

    QQuickItem(QQuickItemPrivate &dd, QQuickItem *parent = nullptr);

private:
    Q_PRIVATE_SLOT(d_func(), void _q_resourceObjectDeleted(QObject *))
    Q_PRIVATE_SLOT(d_func(), quint64 _q_createJSWrapper(QQmlV4ExecutionEnginePtr))

    friend class QQuickWindowPrivate;
    friend class QQuickDeliveryAgentPrivate;
    friend class QSGRenderer;
    friend class QAccessibleQuickItem;
    friend class QQuickAccessibleAttached;
    friend class QQuickAnchorChanges;
#ifndef QT_NO_DEBUG_STREAM
    friend Q_QUICK_EXPORT QDebug operator<<(QDebug debug, QQuickItem *item);
#endif

    Q_DISABLE_COPY(QQuickItem)
    Q_DECLARE_PRIVATE(QQuickItem)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickItem::Flags)

#ifndef Q_QDOC
template <> inline QQuickItem *qobject_cast<QQuickItem *>(QObject *o)
{
    if (!o || !o->isQuickItemType())
        return nullptr;
    return static_cast<QQuickItem *>(o);
}
template <> inline const QQuickItem *qobject_cast<const QQuickItem *>(const QObject *o)
{
    if (!o || !o->isQuickItemType())
        return nullptr;
    return static_cast<const QQuickItem *>(o);
}
#endif // !Q_QDOC

#ifndef QT_NO_DEBUG_STREAM
QDebug Q_QUICK_EXPORT operator<<(QDebug debug,
#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0)
                                 const
#endif
                                 QQuickItem *item);
#endif // QT_NO_DEBUG_STREAM

QT_END_NAMESPACE

#endif // QQUICKITEM_H
