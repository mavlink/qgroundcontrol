// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKITEM_P_H
#define QQUICKITEM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuick/private/qquickanchors_p.h>
#include <QtQuick/private/qquickanchors_p_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuick/private/qquickevents_p_p.h>
#include <QtQuick/private/qquickclipnode_p.h>
#include <QtQuick/private/qquickstate_p.h>
#include <QtQuick/private/qquickpaletteproviderprivatebase_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtCore/private/qproperty_p.h>

#if QT_CONFIG(quick_shadereffect)
#include <QtQuick/private/qquickshadereffectsource_p.h>
#endif

#include <QtQuick/qquickitem.h>
#include <QtQuick/qsgnode.h>

#include <QtQml/private/qqmlnullablevalue_p.h>
#include <QtQml/private/qqmlnotifier_p.h>
#include <QtQml/private/qqmlglobal_p.h>
#include <QtQml/private/qlazilyallocated_p.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlcontext.h>

#include <QtCore/qlist.h>
#include <QtCore/qdebug.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qpointer.h>

#include <QtGui/private/qlayoutpolicy_p.h>
#if QT_CONFIG(accessibility)
#include <QtGui/qaccessible_base.h>
#endif

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcHandlerParent)
Q_DECLARE_LOGGING_CATEGORY(lcVP)

class QNetworkReply;
class QQuickItemKeyFilter;
class QQuickLayoutMirroringAttached;
class QQuickEnterKeyAttached;
class QQuickScreenAttached;
class QQuickPointerHandler;

class QQuickContents : public QSafeQuickItemChangeListener<QQuickContents>
{
    Q_DISABLE_COPY(QQuickContents)
public:
    QQuickContents(QQuickItem *item);
    ~QQuickContents() override;

    QRectF rectF() const { return m_contents; }

    inline void calcGeometry(QQuickItem *changed = nullptr);
    void complete();

    bool inDestructor = false;

protected:
    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &) override;
    void itemDestroyed(QQuickItem *item) override;
    void itemChildAdded(QQuickItem *, QQuickItem *) override;
    void itemChildRemoved(QQuickItem *, QQuickItem *) override;
    //void itemVisibilityChanged(QQuickItem *item)

private:
    bool calcHeight(QQuickItem *changed = nullptr);
    bool calcWidth(QQuickItem *changed = nullptr);
    void updateRect();

    QQuickItem *m_item;
    QRectF m_contents;
};

void QQuickContents::calcGeometry(QQuickItem *changed)
{
    bool wChanged = calcWidth(changed);
    bool hChanged = calcHeight(changed);
    if (wChanged || hChanged)
        updateRect();
}

class QQuickTransformPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickTransform)
public:
    static QQuickTransformPrivate* get(QQuickTransform *transform) { return transform->d_func(); }

    QQuickTransformPrivate();

    QList<QQuickItem *> items;
};

#if QT_CONFIG(quick_shadereffect)

class Q_QUICK_EXPORT QQuickItemLayer : public QObject,
                                       public QSafeQuickItemChangeListener<QQuickItemLayer>
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(QSize textureSize READ size WRITE setSize NOTIFY sizeChanged FINAL)
    Q_PROPERTY(QRectF sourceRect READ sourceRect WRITE setSourceRect NOTIFY sourceRectChanged FINAL)
    Q_PROPERTY(bool mipmap READ mipmap WRITE setMipmap NOTIFY mipmapChanged FINAL)
    Q_PROPERTY(bool smooth READ smooth WRITE setSmooth NOTIFY smoothChanged FINAL)
    Q_PROPERTY(bool live READ live WRITE setLive NOTIFY liveChanged REVISION(6, 5) FINAL)
    Q_PROPERTY(QQuickShaderEffectSource::WrapMode wrapMode READ wrapMode WRITE setWrapMode NOTIFY wrapModeChanged FINAL)
    Q_PROPERTY(QQuickShaderEffectSource::Format format READ format WRITE setFormat NOTIFY formatChanged FINAL)
    Q_PROPERTY(QByteArray samplerName READ name WRITE setName NOTIFY nameChanged FINAL)
    Q_PROPERTY(QQmlComponent *effect READ effect WRITE setEffect NOTIFY effectChanged FINAL)
    Q_PROPERTY(QQuickShaderEffectSource::TextureMirroring textureMirroring READ textureMirroring WRITE setTextureMirroring NOTIFY textureMirroringChanged FINAL)
    Q_PROPERTY(int samples READ samples WRITE setSamples NOTIFY samplesChanged FINAL)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickItemLayer(QQuickItem *item);
    ~QQuickItemLayer() override;

    void classBegin();
    void componentComplete();

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    bool mipmap() const { return m_mipmap; }
    void setMipmap(bool mipmap);

    bool smooth() const { return m_smooth; }
    void setSmooth(bool s);

    bool live() const { return m_live; }
    void setLive(bool live);

    QSize size() const { return m_size; }
    void setSize(const QSize &size);

    QQuickShaderEffectSource::Format format() const { return m_format; }
    void setFormat(QQuickShaderEffectSource::Format f);

    QRectF sourceRect() const { return m_sourceRect; }
    void setSourceRect(const QRectF &sourceRect);

    QQuickShaderEffectSource::WrapMode wrapMode() const { return m_wrapMode; }
    void setWrapMode(QQuickShaderEffectSource::WrapMode mode);

    QByteArray name() const { return m_name; }
    void setName(const QByteArray &name);

    QQmlComponent *effect() const { return m_effectComponent; }
    void setEffect(QQmlComponent *effect);

    QQuickShaderEffectSource::TextureMirroring textureMirroring() const { return m_textureMirroring; }
    void setTextureMirroring(QQuickShaderEffectSource::TextureMirroring mirroring);

    int samples() const { return m_samples; }
    void setSamples(int count);

    QQuickShaderEffectSource *effectSource() const { return m_effectSource; }

    void itemGeometryChanged(QQuickItem *, QQuickGeometryChange, const QRectF &) override;
    void itemOpacityChanged(QQuickItem *) override;
    void itemParentChanged(QQuickItem *, QQuickItem *) override;
    void itemSiblingOrderChanged(QQuickItem *) override;
    void itemVisibilityChanged(QQuickItem *) override;

    void updateMatrix();
    void updateGeometry();
    void updateOpacity();
    void updateZ();

Q_SIGNALS:
    void enabledChanged(bool enabled);
    void sizeChanged(const QSize &size);
    void mipmapChanged(bool mipmap);
    void wrapModeChanged(QQuickShaderEffectSource::WrapMode mode);
    void nameChanged(const QByteArray &name);
    void effectChanged(QQmlComponent *component);
    void smoothChanged(bool smooth);
    void liveChanged(bool live);
    void formatChanged(QQuickShaderEffectSource::Format format);
    void sourceRectChanged(const QRectF &sourceRect);
    void textureMirroringChanged(QQuickShaderEffectSource::TextureMirroring mirroring);
    void samplesChanged(int count);

private:
    friend class QQuickTransformAnimatorJob;
    friend class QQuickOpacityAnimatorJob;

    void activate();
    void deactivate();
    void activateEffect();
    void deactivateEffect();

    QQuickItem *m_item;
    bool m_enabled;
    bool m_mipmap;
    bool m_smooth;
    bool m_live;
    bool m_componentComplete;
    QQuickShaderEffectSource::WrapMode m_wrapMode;
    QQuickShaderEffectSource::Format m_format;
    QSize m_size;
    QRectF m_sourceRect;
    QByteArray m_name;
    QQmlComponent *m_effectComponent;
    QQuickItem *m_effect;
    QQuickShaderEffectSource *m_effectSource;
    QQuickShaderEffectSource::TextureMirroring m_textureMirroring;
    int m_samples;
};

#endif

class Q_QUICK_EXPORT QQuickItemPrivate
    : public QObjectPrivate
    , public QQuickPaletteProviderPrivateBase<QQuickItem, QQuickItemPrivate>
{
    Q_DECLARE_PUBLIC(QQuickItem)

public:
    static QQuickItemPrivate* get(QQuickItem *item) { return item->d_func(); }
    static const QQuickItemPrivate* get(const QQuickItem *item) { return item->d_func(); }

    QQuickItemPrivate();
    ~QQuickItemPrivate() override;
    void init(QQuickItem *parent);

    QQmlListProperty<QObject> data();
    QQmlListProperty<QObject> resources();
    QQmlListProperty<QQuickItem> children();
    QQmlListProperty<QQuickItem> visibleChildren();

    QQmlListProperty<QQuickState> states();
    QQmlListProperty<QQuickTransition> transitions();

    QString state() const;
    void setState(const QString &);

    QQuickAnchorLine left() const;
    QQuickAnchorLine right() const;
    QQuickAnchorLine horizontalCenter() const;
    QQuickAnchorLine top() const;
    QQuickAnchorLine bottom() const;
    QQuickAnchorLine verticalCenter() const;
    QQuickAnchorLine baseline() const;

#if QT_CONFIG(quick_shadereffect)
    QQuickItemLayer *layer() const;
#endif

    void localizedTouchEvent(const QTouchEvent *event, bool isFiltering, QMutableTouchEvent *localized);
    bool hasPointerHandlers() const;
    bool hasEnabledHoverHandlers() const;
    virtual void addPointerHandler(QQuickPointerHandler *h);
    virtual void removePointerHandler(QQuickPointerHandler *h);

    QObject *setContextMenu(QObject *menu);

    // data property
    static void data_append(QQmlListProperty<QObject> *, QObject *);
    static qsizetype data_count(QQmlListProperty<QObject> *);
    static QObject *data_at(QQmlListProperty<QObject> *, qsizetype);
    static void data_clear(QQmlListProperty<QObject> *);
    static void data_removeLast(QQmlListProperty<QObject> *);

    // resources property
    static QObject *resources_at(QQmlListProperty<QObject> *, qsizetype);
    static void resources_append(QQmlListProperty<QObject> *, QObject *);
    static qsizetype resources_count(QQmlListProperty<QObject> *);
    static void resources_clear(QQmlListProperty<QObject> *);
    static void resources_removeLast(QQmlListProperty<QObject> *);

    // children property
    static void children_append(QQmlListProperty<QQuickItem> *, QQuickItem *);
    static qsizetype children_count(QQmlListProperty<QQuickItem> *);
    static QQuickItem *children_at(QQmlListProperty<QQuickItem> *, qsizetype);
    static void children_clear(QQmlListProperty<QQuickItem> *);
    static void children_removeLast(QQmlListProperty<QQuickItem> *);

    // visibleChildren property
    static qsizetype visibleChildren_count(QQmlListProperty<QQuickItem> *prop);
    static QQuickItem *visibleChildren_at(QQmlListProperty<QQuickItem> *prop, qsizetype index);

    // transform property
    static qsizetype transform_count(QQmlListProperty<QQuickTransform> *list);
    static void transform_append(QQmlListProperty<QQuickTransform> *list, QQuickTransform *);
    static QQuickTransform *transform_at(QQmlListProperty<QQuickTransform> *list, qsizetype);
    static void transform_clear(QQmlListProperty<QQuickTransform> *list);

    void _q_resourceObjectDeleted(QObject *);
    quint64 _q_createJSWrapper(QQmlV4ExecutionEnginePtr engine);

    enum ChangeType {
        Geometry = 0x01,
        SiblingOrder = 0x02,
        Visibility = 0x04,
        Opacity = 0x08,
        Destroyed = 0x10,
        Parent = 0x20,
        Children = 0x40,
        Rotation = 0x80,
        ImplicitWidth = 0x100,
        ImplicitHeight = 0x200,
        Enabled = 0x400,
        Focus = 0x800,
        Scale = 0x1000,
        Matrix = 0x2000,
        AllChanges = 0xFFFFFFFF
    };

    Q_DECLARE_FLAGS(ChangeTypes, ChangeType)
    friend inline QDebug &operator<<(QDebug &dbg, QQuickItemPrivate::ChangeTypes types) {
#define CHANGETYPE_OUTPUT(Type) if (types & QQuickItemPrivate::Type) { dbg << first << #Type ; first = '|'; }
        QDebugStateSaver state(dbg);
        dbg.noquote().nospace();
        if (types == QQuickItemPrivate::AllChanges) {
            dbg << " AllChanges";
        } else {
            char first = ' ';
            CHANGETYPE_OUTPUT(Geometry);
            CHANGETYPE_OUTPUT(SiblingOrder);
            CHANGETYPE_OUTPUT(Visibility);
            CHANGETYPE_OUTPUT(Opacity);
            CHANGETYPE_OUTPUT(Destroyed);
            CHANGETYPE_OUTPUT(Parent);
            CHANGETYPE_OUTPUT(Children);
            CHANGETYPE_OUTPUT(Rotation);
            CHANGETYPE_OUTPUT(ImplicitWidth);
            CHANGETYPE_OUTPUT(ImplicitHeight);
            CHANGETYPE_OUTPUT(Enabled);
            CHANGETYPE_OUTPUT(Focus);
            CHANGETYPE_OUTPUT(Scale);
            CHANGETYPE_OUTPUT(Matrix);
#undef CHANGETYPE_OUTPUT
        }
        return dbg;
    }

    struct ChangeListener {
        using ChangeTypes = QQuickItemPrivate::ChangeTypes;

        ChangeListener(QQuickItemChangeListener *l = nullptr, ChangeTypes t = { })
            : listener(l)
            , types(t)
            , gTypes(QQuickGeometryChange::All)
        {}

        ChangeListener(QQuickItemChangeListener *l, QQuickGeometryChange gt)
            : listener(l)
            , types(Geometry)
            , gTypes(gt)
        {}

        bool operator==(const ChangeListener &other) const
        { return listener == other.listener && types == other.types; }

        QQuickItemChangeListener *listener;
        ChangeTypes types;
        QQuickGeometryChange gTypes;  //NOTE: not used for ==

#ifndef QT_NO_DEBUG_STREAM
    private:
        friend QDebug operator<<(QDebug debug, const QQuickItemPrivate::ChangeListener &listener);
#endif // QT_NO_DEBUG_STREAM
    };

    // call QQuickItemChangeListener
    template <typename Fn, typename ...Args>
    void notifyChangeListeners(QQuickItemPrivate::ChangeTypes changeTypes, Fn &&function, Args &&...args)
    {
        Q_Q(QQuickItem);
        if (changeListeners.isEmpty())
            return;

        const auto listeners = changeListeners; // NOTE: intentional copy (QTBUG-54732)
        for (const QQuickItemPrivate::ChangeListener &change : listeners) {
            Q_ASSERT(change.listener);
            if (change.types & changeTypes) {
#ifdef QT_BUILD_INTERNAL
                if (changeTypes == AllChanges && change.listener->anchorPrivate() == nullptr) {
                    // Nothing to worry about as long as the lambdas in ~QQuickItem
                    // don't mess around with the change.listener directly!
                } else if (change.listener->baseDeleted(q)) {
                    auto output = qCritical();
                    output.noquote();
                    output << "Listener already tagged as destroyed when called!"
                           << "\n\tListener:" << change.listener->debugName()
                           << "\n\tChanges:" << change.types
                           << "\n\tCaller:  " << q
                           << "\n\tChanges:" << changeTypes;
                }
#endif
                if constexpr (std::is_member_function_pointer_v<Fn>)
                    (change.listener->*function)(args...);
                else
                    function(change, args...);
            }
            if (changeTypes & QQuickItemPrivate::Destroyed)
                change.listener->removeSourceItem(q);
        }
    }

    struct ExtraData {
        Q_QUICK_EXPORT ExtraData();

        qreal z;
        qreal scale;
        qreal rotation;
        qreal opacity;

        QQuickContents *contents;
        QQuickScreenAttached *screenAttached;
        QQuickLayoutMirroringAttached* layoutDirectionAttached;
        QQuickEnterKeyAttached *enterKeyAttached;
        QQuickItemKeyFilter *keyHandler;
        QVector<QQuickPointerHandler *> pointerHandlers;
        QObject *contextMenu;
#if QT_CONFIG(quick_shadereffect)
        mutable QQuickItemLayer *layer;
#endif
#if QT_CONFIG(cursor)
        QCursor cursor;
#endif
        QPointF userTransformOriginPoint;

        // these do not include child items
        int effectRefCount;
        int hideRefCount;
        // updated recursively for child items as well
        int recursiveEffectRefCount;
        // Mask contains() method index
        int maskContainsIndex;

        // Contains mask
        QPointer<QObject> mask;

        QSGOpacityNode *opacityNode;
        QQuickDefaultClipNode *clipNode;
        QSGRootNode *rootNode;
        // subsceneDeliveryAgent is set only if this item is the root of a subscene, not on all items within.
        QQuickDeliveryAgent *subsceneDeliveryAgent = nullptr;

        QObjectList resourcesList;

        // Although acceptedMouseButtons is inside ExtraData, we actually store
        // the LeftButton flag in the extra.tag() bit.  This is because it is
        // extremely common to set acceptedMouseButtons to LeftButton, but very
        // rare to use any of the other buttons.
        Qt::MouseButtons acceptedMouseButtons;
        Qt::MouseButtons acceptedMouseButtonsWithoutHandlers;

        uint origin:5; // QQuickItem::TransformOrigin
        uint transparentForPositioner : 1;

        // 26 bits padding
    };

    enum ExtraDataTag {
        NoTag = 0x1,
        LeftMouseButtonAccepted = 0x2
    };
    Q_DECLARE_FLAGS(ExtraDataTags, ExtraDataTag)

    QLazilyAllocated<ExtraData, ExtraDataTags> extra;
    // If the mask is an Item, inform it that it's being used as a mask (true) or is no longer being used (false)
    virtual void registerAsContainmentMask(QQuickItem * /* maskedItem */, bool /* set */) { }

    QQuickAnchors *anchors() const;
    mutable QQuickAnchors *_anchors;

    inline Qt::MouseButtons acceptedMouseButtons() const;

    QVector<QQuickItemPrivate::ChangeListener> changeListeners;

    void addItemChangeListener(QQuickItemChangeListener *listener, ChangeTypes types);
    void updateOrAddItemChangeListener(QQuickItemChangeListener *listener, ChangeTypes types);
    void removeItemChangeListener(QQuickItemChangeListener *, ChangeTypes types);
    void updateOrAddGeometryChangeListener(QQuickItemChangeListener *listener, QQuickGeometryChange types);
    void updateOrRemoveGeometryChangeListener(QQuickItemChangeListener *listener, QQuickGeometryChange types);

    QQuickStateGroup *_states();
    QQuickStateGroup *_stateGroup;

    inline QQuickItem::TransformOrigin origin() const;

    // Bit 0
    quint32 flags:7;
    quint32 widthValidFlag:1;
    quint32 heightValidFlag:1;
    quint32 componentComplete:1;
    quint32 keepMouse:1;
    quint32 keepTouch:1;
    quint32 hoverEnabled:1;
    quint32 smooth:1;
    quint32 antialiasing:1;
    quint32 focus:1;
    // Bit 16
    quint32 activeFocus:1;
    quint32 notifiedFocus:1;
    quint32 notifiedActiveFocus:1;
    quint32 filtersChildMouseEvents:1;
    quint32 explicitVisible:1;
    quint32 effectiveVisible:1;
    quint32 explicitEnable:1;
    quint32 effectiveEnable:1;
    quint32 polishScheduled:1;
    quint32 inheritedLayoutMirror:1;
    quint32 effectiveLayoutMirror:1;
    quint32 isMirrorImplicit:1;
    quint32 inheritMirrorFromParent:1;
    quint32 inheritMirrorFromItem:1;
    quint32 isAccessible:1;
    quint32 culled:1;
    // Bit 32
    quint32 hasCursor:1;
    quint32 subtreeCursorEnabled:1;
    quint32 subtreeHoverEnabled:1;
    quint32 activeFocusOnTab:1;
    quint32 implicitAntialiasing:1;
    quint32 antialiasingValid:1;
    // isTabFence: When true, the item acts as a fence within the tab focus chain.
    // This means that the item and its children will be skipped from the tab focus
    // chain when navigating from its parent or any of its siblings. Similarly,
    // when any of the item's descendants gets focus, the item constrains the tab
    // focus chain and prevents tabbing outside.
    quint32 isTabFence:1;
    quint32 replayingPressEvent:1;
    // Bit 40
    quint32 touchEnabled:1;
    quint32 hasCursorHandler:1;
    // set true when this item does not expect events via a subscene delivery agent; false otherwise
    quint32 maybeHasSubsceneDeliveryAgent:1;
    // set true if this item or any child wants QQuickItemPrivate::transformChanged() to visit all children
    // (e.g. when parent has ItemIsViewport and child has ItemObservesViewport)
    quint32 subtreeTransformChangedEnabled:1;
    quint32 inDestructor:1; // has entered ~QQuickItem
    quint32 focusReason:4;
    quint32 focusPolicy:4;
    // Bit 53

    enum DirtyType {
        TransformOrigin         = 0x00000001,
        Transform               = 0x00000002,
        BasicTransform          = 0x00000004,
        Position                = 0x00000008,
        Size                    = 0x00000010,

        ZValue                  = 0x00000020,
        Content                 = 0x00000040,
        Smooth                  = 0x00000080,
        OpacityValue            = 0x00000100,
        ChildrenChanged         = 0x00000200,
        ChildrenStackingChanged = 0x00000400,
        ParentChanged           = 0x00000800,

        Clip                    = 0x00001000,
        Window                  = 0x00002000,

        EffectReference         = 0x00008000,
        Visible                 = 0x00010000,
        HideReference           = 0x00020000,
        Antialiasing             = 0x00040000,
        // When you add an attribute here, don't forget to update
        // dirtyToString()

        TransformUpdateMask     = TransformOrigin | Transform | BasicTransform | Position |
                                  Window,
        ComplexTransformUpdateMask     = Transform | Window,
        ContentUpdateMask       = Size | Content | Smooth | Window | Antialiasing,
        ChildrenUpdateMask      = ChildrenChanged | ChildrenStackingChanged | EffectReference | Window
    };

    quint32 dirtyAttributes;
    QString dirtyToString() const;
    void dirty(DirtyType);
    void addToDirtyList();
    void removeFromDirtyList();
    QQuickItem *nextDirtyItem;
    QQuickItem**prevDirtyItem;

    void setCulled(bool);

    QQuickWindow *window;
    int windowRefCount;
    inline QSGContext *sceneGraphContext() const;
    inline QSGRenderContext *sceneGraphRenderContext() const;

    QQuickItem *parentItem;

    QList<QQuickItem *> childItems;
    mutable QList<QQuickItem *> *sortedChildItems;
    QList<QQuickItem *> paintOrderChildItems() const;
    void addChild(QQuickItem *);
    void removeChild(QQuickItem *);
    void siblingOrderChanged();

    inline void markSortedChildrenDirty(QQuickItem *child);

    void refWindow(QQuickWindow *);
    void derefWindow();

    qreal effectiveDevicePixelRatio() const;

    QPointer<QQuickItem> subFocusItem;
    void updateSubFocusItem(QQuickItem *scope, bool focus);

    bool setFocusIfNeeded(QEvent::Type);
    Qt::FocusReason lastFocusChangeReason() const;
    virtual bool setLastFocusChangeReason(Qt::FocusReason reason);

    QTransform windowToItemTransform() const;
    QTransform itemToWindowTransform() const;
    void itemToParentTransform(QTransform *) const;

    static bool focusNextPrev(QQuickItem *item, bool forward);
    static QQuickItem *nextTabChildItem(const QQuickItem *item, int start);
    static QQuickItem *prevTabChildItem(const QQuickItem *item, int start);
    static QQuickItem *nextPrevItemInTabFocusChain(QQuickItem *item, bool forward, bool wrap = true);

    static bool canAcceptTabFocus(QQuickItem *item);

    void setX(qreal x) {q_func()->setX(x);}
    void xChanged() { Q_EMIT q_func()->xChanged(); }
    Q_OBJECT_COMPAT_PROPERTY(QQuickItemPrivate, qreal, x, &QQuickItemPrivate::setX, &QQuickItemPrivate::xChanged);
    void setY(qreal y) {q_func()->setY(y);}
    void yChanged() { Q_EMIT q_func()->yChanged(); }
    Q_OBJECT_COMPAT_PROPERTY(QQuickItemPrivate, qreal, y, &QQuickItemPrivate::setY, &QQuickItemPrivate::yChanged);
    void setWidth(qreal width) {q_func()->setWidth(width);}
    void widthChanged() { Q_EMIT q_func()->widthChanged(); }
    Q_OBJECT_COMPAT_PROPERTY(QQuickItemPrivate, qreal, width, &QQuickItemPrivate::setWidth, &QQuickItemPrivate::widthChanged);
    void setHeight(qreal height) {q_func()->setHeight(height);}
    void heightChanged() { Q_EMIT q_func()->heightChanged(); }
    Q_OBJECT_COMPAT_PROPERTY(QQuickItemPrivate, qreal, height, &QQuickItemPrivate::setHeight, &QQuickItemPrivate::heightChanged);
    qreal implicitWidth;
    qreal implicitHeight;

    bool widthValid() const { return widthValidFlag || (width.hasBinding() && !QQmlPropertyBinding::isUndefined(width.binding()) ); }
    bool heightValid() const { return heightValidFlag || (height.hasBinding() && !QQmlPropertyBinding::isUndefined(height.binding()) ); }

    qreal baselineOffset;

    QList<QQuickTransform *> transforms;

    inline qreal z() const { return extra.isAllocated()?extra->z:0; }
    inline qreal scale() const { return extra.isAllocated()?extra->scale:1; }
    inline qreal rotation() const { return extra.isAllocated()?extra->rotation:0; }
    inline qreal opacity() const { return extra.isAllocated()?extra->opacity:1; }

    void setAccessible();

    virtual qreal getImplicitWidth() const;
    virtual qreal getImplicitHeight() const;
    virtual void implicitWidthChanged();
    virtual void implicitHeightChanged();

#if QT_CONFIG(accessibility)
    QAccessible::Role effectiveAccessibleRole() const;
private:
    virtual QAccessible::Role accessibleRole() const;
public:
#endif

    void setImplicitAntialiasing(bool antialiasing);

    void resolveLayoutMirror();
    void setImplicitLayoutMirror(bool mirror, bool inherit);
    void setLayoutMirror(bool mirror);
    bool isMirrored() const {
        return effectiveLayoutMirror;
    }

    void emitChildrenRectChanged(const QRectF &rect) {
        Q_Q(QQuickItem);
        Q_EMIT q->childrenRectChanged(rect);
    }

    QPointF computeTransformOrigin() const;
    virtual bool transformChanged(QQuickItem *transformedItem);

    QPointF adjustedPosForTransform(const QPointF &centroid,
                                    const QPointF &startPos, const QVector2D &activeTranslatation,
                                    qreal startScale, qreal activeScale,
                                    qreal startRotation, qreal activeRotation);

    QQuickDeliveryAgent *deliveryAgent();
    QQuickDeliveryAgentPrivate *deliveryAgentPrivate();
    QQuickDeliveryAgent *ensureSubsceneDeliveryAgent();

    void deliverKeyEvent(QKeyEvent *);
    bool filterKeyEvent(QKeyEvent *, bool post);
#if QT_CONFIG(im)
    void deliverInputMethodEvent(QInputMethodEvent *);
#endif
    void deliverShortcutOverrideEvent(QKeyEvent *);

    void deliverPointerEvent(QEvent *);

    bool anyPointerHandlerWants(const QPointerEvent *event, const QEventPoint &point) const;
    virtual bool handlePointerEvent(QPointerEvent *, bool avoidGrabbers = false);
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    virtual bool handleContextMenuEvent(QContextMenuEvent *event);
#endif

    virtual void setVisible(bool visible);

    bool isTransparentForPositioner() const;
    void setTransparentForPositioner(bool trans);

    bool calcEffectiveVisible() const;
    bool setEffectiveVisibleRecur(bool);
    bool calcEffectiveEnable() const;
    void setEffectiveEnableRecur(QQuickItem *scope, bool);


    inline QSGTransformNode *itemNode();
    inline QSGNode *childContainerNode();

    /*
      QSGNode order is:
         - itemNode
         - (opacityNode)
         - (clipNode)
         - (rootNode) (shader effect source's root node)
     */

    QSGOpacityNode *opacityNode() const { return extra.isAllocated()?extra->opacityNode:nullptr; }
    QQuickDefaultClipNode *clipNode() const { return extra.isAllocated()?extra->clipNode:nullptr; }
    QSGRootNode *rootNode() const { return extra.isAllocated()?extra->rootNode:nullptr; }

    QSGTransformNode *itemNodeInstance;
    QSGNode *paintNode;

    virtual QSGTransformNode *createTransformNode();

    // A reference from an effect item means that this item is used by the effect, so
    // it should insert a root node.
    void refFromEffectItem(bool hide);
    void recursiveRefFromEffectItem(int refs);
    void derefFromEffectItem(bool unhide);

    void itemChange(QQuickItem::ItemChange, const QQuickItem::ItemChangeData &);

    void enableSubtreeChangeNotificationsForParentHierachy();

    virtual void mirrorChange() {}

    void setHasCursorInChild(bool hasCursor);
    void setHasHoverInChild(bool hasHover);
#if QT_CONFIG(cursor)
    QCursor effectiveCursor(const QQuickPointerHandler *handler) const;
    QQuickPointerHandler *effectiveCursorHandler() const;
#endif

    virtual void updatePolish() { }
    virtual void dumpItemTree(int indent) const;

    QLayoutPolicy sizePolicy() const;
    void setSizePolicy(const QLayoutPolicy::Policy &horizontalPolicy, const QLayoutPolicy::Policy &verticalPolicy);
    QLayoutPolicy szPolicy;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickItemPrivate::ExtraDataTags)

/*
    Key filters can be installed on a QQuickItem, but not removed.  Currently they
    are only used by attached objects (which are only destroyed on Item
    destruction), so this isn't a problem.  If in future this becomes any form
    of public API, they will have to support removal too.
*/
class QQuickItemKeyFilter
{
public:
    QQuickItemKeyFilter(QQuickItem * = nullptr);
    virtual ~QQuickItemKeyFilter();

    virtual void keyPressed(QKeyEvent *event, bool post);
    virtual void keyReleased(QKeyEvent *event, bool post);
#if QT_CONFIG(im)
    virtual void inputMethodEvent(QInputMethodEvent *event, bool post);
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;
#endif
    virtual void shortcutOverrideEvent(QKeyEvent *event);
    virtual void componentComplete();

    bool m_processPost;

private:
    QQuickItemKeyFilter *m_next;
};

class QQuickKeyNavigationAttachedPrivate : public QObjectPrivate
{
public:
    QQuickKeyNavigationAttachedPrivate()
        : leftSet(false), rightSet(false), upSet(false), downSet(false),
          tabSet(false), backtabSet(false) {}

    QPointer<QQuickItem> left;
    QPointer<QQuickItem> right;
    QPointer<QQuickItem> up;
    QPointer<QQuickItem> down;
    QPointer<QQuickItem> tab;
    QPointer<QQuickItem> backtab;
    bool leftSet : 1;
    bool rightSet : 1;
    bool upSet : 1;
    bool downSet : 1;
    bool tabSet : 1;
    bool backtabSet : 1;
};

class Q_QUICK_EXPORT QQuickKeyNavigationAttached : public QObject, public QQuickItemKeyFilter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickKeyNavigationAttached)

    Q_PROPERTY(QQuickItem *left READ left WRITE setLeft NOTIFY leftChanged FINAL)
    Q_PROPERTY(QQuickItem *right READ right WRITE setRight NOTIFY rightChanged FINAL)
    Q_PROPERTY(QQuickItem *up READ up WRITE setUp NOTIFY upChanged FINAL)
    Q_PROPERTY(QQuickItem *down READ down WRITE setDown NOTIFY downChanged FINAL)
    Q_PROPERTY(QQuickItem *tab READ tab WRITE setTab NOTIFY tabChanged FINAL)
    Q_PROPERTY(QQuickItem *backtab READ backtab WRITE setBacktab NOTIFY backtabChanged FINAL)
    Q_PROPERTY(Priority priority READ priority WRITE setPriority NOTIFY priorityChanged FINAL)

    QML_NAMED_ELEMENT(KeyNavigation)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("KeyNavigation is only available via attached properties.")
    QML_ATTACHED(QQuickKeyNavigationAttached)

public:
    QQuickKeyNavigationAttached(QObject * = nullptr);

    QQuickItem *left() const;
    void setLeft(QQuickItem *);
    QQuickItem *right() const;
    void setRight(QQuickItem *);
    QQuickItem *up() const;
    void setUp(QQuickItem *);
    QQuickItem *down() const;
    void setDown(QQuickItem *);
    QQuickItem *tab() const;
    void setTab(QQuickItem *);
    QQuickItem *backtab() const;
    void setBacktab(QQuickItem *);

    enum Priority { BeforeItem, AfterItem };
    Q_ENUM(Priority)
    Priority priority() const;
    void setPriority(Priority);

    static QQuickKeyNavigationAttached *qmlAttachedProperties(QObject *);

Q_SIGNALS:
    void leftChanged();
    void rightChanged();
    void upChanged();
    void downChanged();
    void tabChanged();
    void backtabChanged();
    void priorityChanged();

private:
    void keyPressed(QKeyEvent *event, bool post) override;
    void keyReleased(QKeyEvent *event, bool post) override;
    void setFocusNavigation(QQuickItem *currentItem, const char *dir,
                            Qt::FocusReason reason = Qt::OtherFocusReason);
};

class QQuickLayoutMirroringAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled RESET resetEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool childrenInherit READ childrenInherit WRITE setChildrenInherit NOTIFY childrenInheritChanged FINAL)

    QML_NAMED_ELEMENT(LayoutMirroring)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("LayoutMirroring is only available via attached properties.")
    QML_ATTACHED(QQuickLayoutMirroringAttached)

public:
    explicit QQuickLayoutMirroringAttached(QObject *parent = nullptr);

    bool enabled() const;
    void setEnabled(bool);
    void resetEnabled();

    bool childrenInherit() const;
    void setChildrenInherit(bool);

    static QQuickLayoutMirroringAttached *qmlAttachedProperties(QObject *);
Q_SIGNALS:
    void enabledChanged();
    void childrenInheritChanged();
private:
    friend class QQuickItemPrivate;
    QQuickItemPrivate *itemPrivate;
};

class QQuickEnterKeyAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Qt::EnterKeyType type READ type WRITE setType NOTIFY typeChanged FINAL)

    QML_NAMED_ELEMENT(EnterKey)
    QML_UNCREATABLE("EnterKey is only available via attached properties")
    QML_ADDED_IN_VERSION(2, 6)
    QML_ATTACHED(QQuickEnterKeyAttached)

public:
    explicit QQuickEnterKeyAttached(QObject *parent = nullptr);

    Qt::EnterKeyType type() const;
    void setType(Qt::EnterKeyType type);

    static QQuickEnterKeyAttached *qmlAttachedProperties(QObject *);
Q_SIGNALS:
    void typeChanged();
private:
    friend class QQuickItemPrivate;
    QQuickItemPrivate *itemPrivate;

    Qt::EnterKeyType keyType;
};

class QQuickKeysAttachedPrivate : public QObjectPrivate
{
public:
    QQuickKeysAttachedPrivate()
        : inPress(false), inRelease(false), inIM(false), enabled(true)
    {}

    //loop detection
    bool inPress:1;
    bool inRelease:1;
    bool inIM:1;

    bool enabled : 1;

    QQuickItem *imeItem = nullptr;
    QList<QQuickItem *> targets;
    QQuickItem *item = nullptr;
    QQuickKeyEvent theKeyEvent;
};

class Q_QUICK_EXPORT QQuickKeysAttached : public QObject, public QQuickItemKeyFilter
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickKeysAttached)

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QQuickItem> forwardTo READ forwardTo FINAL)
    Q_PROPERTY(Priority priority READ priority WRITE setPriority NOTIFY priorityChanged FINAL)

    QML_NAMED_ELEMENT(Keys)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("Keys is only available via attached properties")
    QML_ATTACHED(QQuickKeysAttached)

public:
    QQuickKeysAttached(QObject *parent=nullptr);
    ~QQuickKeysAttached() override;

    bool enabled() const { Q_D(const QQuickKeysAttached); return d->enabled; }
    void setEnabled(bool enabled) {
        Q_D(QQuickKeysAttached);
        if (enabled != d->enabled) {
            d->enabled = enabled;
            Q_EMIT enabledChanged();
        }
    }

    enum Priority { BeforeItem, AfterItem};
    Q_ENUM(Priority)
    Priority priority() const;
    void setPriority(Priority);

    QQmlListProperty<QQuickItem> forwardTo() {
        Q_D(QQuickKeysAttached);
        return QQmlListProperty<QQuickItem>(this, &(d->targets));
    }

    void componentComplete() override;

    static QQuickKeysAttached *qmlAttachedProperties(QObject *);

Q_SIGNALS:
    void enabledChanged();
    void priorityChanged();
    void pressed(QQuickKeyEvent *event);
    void released(QQuickKeyEvent *event);
    void shortcutOverride(QQuickKeyEvent *event);
    void digit0Pressed(QQuickKeyEvent *event);
    void digit1Pressed(QQuickKeyEvent *event);
    void digit2Pressed(QQuickKeyEvent *event);
    void digit3Pressed(QQuickKeyEvent *event);
    void digit4Pressed(QQuickKeyEvent *event);
    void digit5Pressed(QQuickKeyEvent *event);
    void digit6Pressed(QQuickKeyEvent *event);
    void digit7Pressed(QQuickKeyEvent *event);
    void digit8Pressed(QQuickKeyEvent *event);
    void digit9Pressed(QQuickKeyEvent *event);

    void leftPressed(QQuickKeyEvent *event);
    void rightPressed(QQuickKeyEvent *event);
    void upPressed(QQuickKeyEvent *event);
    void downPressed(QQuickKeyEvent *event);
    void tabPressed(QQuickKeyEvent *event);
    void backtabPressed(QQuickKeyEvent *event);

    void asteriskPressed(QQuickKeyEvent *event);
    void numberSignPressed(QQuickKeyEvent *event);
    void escapePressed(QQuickKeyEvent *event);
    void returnPressed(QQuickKeyEvent *event);
    void enterPressed(QQuickKeyEvent *event);
    void deletePressed(QQuickKeyEvent *event);
    void spacePressed(QQuickKeyEvent *event);
    void backPressed(QQuickKeyEvent *event);
    void cancelPressed(QQuickKeyEvent *event);
    void selectPressed(QQuickKeyEvent *event);
    void yesPressed(QQuickKeyEvent *event);
    void noPressed(QQuickKeyEvent *event);
    void context1Pressed(QQuickKeyEvent *event);
    void context2Pressed(QQuickKeyEvent *event);
    void context3Pressed(QQuickKeyEvent *event);
    void context4Pressed(QQuickKeyEvent *event);
    void callPressed(QQuickKeyEvent *event);
    void hangupPressed(QQuickKeyEvent *event);
    void flipPressed(QQuickKeyEvent *event);
    void menuPressed(QQuickKeyEvent *event);
    void volumeUpPressed(QQuickKeyEvent *event);
    void volumeDownPressed(QQuickKeyEvent *event);

private:
    void keyPressed(QKeyEvent *event, bool post) override;
    void keyReleased(QKeyEvent *event, bool post) override;
#if QT_CONFIG(im)
    void inputMethodEvent(QInputMethodEvent *, bool post) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
#endif
    void shortcutOverrideEvent(QKeyEvent *event) override;
    static QByteArray keyToSignal(int key);

    bool isConnected(const char *signalName) const;
};

Qt::MouseButtons QQuickItemPrivate::acceptedMouseButtons() const
{
    return ((extra.tag().testFlag(LeftMouseButtonAccepted) ? Qt::LeftButton : Qt::MouseButton(0)) |
            (extra.isAllocated() ? extra->acceptedMouseButtons : Qt::MouseButtons{}));
}

QSGContext *QQuickItemPrivate::sceneGraphContext() const
{
    Q_ASSERT(window);
    return static_cast<QQuickWindowPrivate *>(QObjectPrivate::get(window))->context->sceneGraphContext();
}

QSGRenderContext *QQuickItemPrivate::sceneGraphRenderContext() const
{
    Q_ASSERT(window);
    return static_cast<QQuickWindowPrivate *>(QObjectPrivate::get(window))->context;
}

void QQuickItemPrivate::markSortedChildrenDirty(QQuickItem *child)
{
    // If sortedChildItems == &childItems then all in childItems have z == 0
    // and we don't need to invalidate if the changed item also has z == 0.
    if (child->z() != 0. || sortedChildItems != &childItems) {
        if (sortedChildItems != &childItems)
            delete sortedChildItems;
        sortedChildItems = nullptr;
    }
}

QQuickItem::TransformOrigin QQuickItemPrivate::origin() const
{
    return extra.isAllocated() ? QQuickItem::TransformOrigin(extra->origin)
                               : QQuickItem::Center;
}

QSGTransformNode *QQuickItemPrivate::itemNode()
{
    if (!itemNodeInstance) {
        itemNodeInstance = createTransformNode();
        itemNodeInstance->setFlag(QSGNode::OwnedByParent, false);
#ifdef QSG_RUNTIME_DESCRIPTION
        Q_Q(QQuickItem);
        qsgnode_set_description(itemNodeInstance, QString::fromLatin1("QQuickItem(%1:%2)").arg(QString::fromLatin1(q->metaObject()->className())).arg(q->objectName()));
#endif
    }
    return itemNodeInstance;
}

QSGNode *QQuickItemPrivate::childContainerNode()
{
    if (rootNode())
        return rootNode();
    else if (clipNode())
        return clipNode();
    else if (opacityNode())
        return opacityNode();
    else
        return itemNode();
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickItemPrivate::ChangeTypes)
Q_DECLARE_TYPEINFO(QQuickItemPrivate::ChangeListener, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif // QQUICKITEM_P_H
