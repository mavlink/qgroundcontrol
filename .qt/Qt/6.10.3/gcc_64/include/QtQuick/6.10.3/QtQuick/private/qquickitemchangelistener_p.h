// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKITEMCHANGELISTENER_P_H
#define QQUICKITEMCHANGELISTENER_P_H

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

#include <QtCore/qxptype_traits.h>
#include <QtQml/private/qqmldata_p.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class QRectF;
class QQuickItem;
class QQuickAnchorsPrivate;

class QQuickGeometryChange
{
public:
    enum Kind: int {
        Nothing = 0x00,
        X       = 0x01,
        Y       = 0x02,
        Width   = 0x04,
        Height  = 0x08,

        Size = Width | Height,
        All = X | Y | Size
    };

    QQuickGeometryChange(int change = Nothing)
        : kind(change)
    {}

    bool noChange() const { return kind == Nothing; }
    bool anyChange() const { return !noChange(); }

    bool xChange() const { return kind & X; }
    bool yChange() const { return kind & Y; }
    bool widthChange() const { return kind & Width; }
    bool heightChange() const { return kind & Height; }

    bool positionChange() const { return xChange() || yChange(); }
    bool sizeChange() const { return widthChange() || heightChange(); }

    bool horizontalChange() const { return xChange() || widthChange(); }
    bool verticalChange() const { return yChange() || heightChange(); }

    void setXChange(bool enabled) { set(X, enabled); }
    void setYChange(bool enabled) { set(Y, enabled); }
    void setWidthChange(bool enabled) { set(Width, enabled); }
    void setHeightChange(bool enabled) { set(Height, enabled); }
    void setSizeChange(bool enabled) { set(Size, enabled); }
    void setAllChanged(bool enabled) { set(All, enabled); }
    void setHorizontalChange(bool enabled) { set(X | Width, enabled); }
    void setVerticalChange(bool enabled) { set(Y | Height, enabled); }

    void set(int bits, bool enabled)
    {
        if (enabled) {
            kind |= bits;
        } else {
            kind &= ~bits;
        }
    }

    bool matches(QQuickGeometryChange other) const { return kind & other.kind; }

private:
    int kind;
};

#define QT_QUICK_NEW_GEOMETRY_CHANGED_HANDLING

class Q_QUICK_EXPORT QQuickItemChangeListener
{
public:
    virtual ~QQuickItemChangeListener();

    virtual void itemGeometryChanged(QQuickItem *, QQuickGeometryChange, const QRectF & /* oldGeometry */) {}
    virtual void itemSiblingOrderChanged(QQuickItem *) {}
    virtual void itemVisibilityChanged(QQuickItem *) {}
    virtual void itemEnabledChanged(QQuickItem *) {}
    virtual void itemOpacityChanged(QQuickItem *) {}
    virtual void itemDestroyed(QQuickItem *) {}
    virtual void itemChildAdded(QQuickItem *, QQuickItem * /* child */ ) {}
    virtual void itemChildRemoved(QQuickItem *, QQuickItem * /* child */ ) {}
    virtual void itemParentChanged(QQuickItem *, QQuickItem * /* parent */ ) {}
    virtual void itemRotationChanged(QQuickItem *) {}
    virtual void itemImplicitWidthChanged(QQuickItem *) {}
    virtual void itemImplicitHeightChanged(QQuickItem *) {}
    virtual void itemFocusChanged(QQuickItem *, Qt::FocusReason /* reason */) {}
    virtual void itemScaleChanged(QQuickItem *) {}
    virtual void itemTransformChanged(QQuickItem *, QQuickItem * /* transformedItem */) {}

    virtual QQuickAnchorsPrivate *anchorPrivate() { return nullptr; }
    virtual bool baseDeleted(const QObject *caller) const
    {
        if (auto ptr = dynamic_cast<const QObjectPrivate *>(this))
            return ptr->q_ptr != caller && QQmlData::wasDeleted(ptr);
        return false;
    }
    virtual QString debugName() const {
        return QStringLiteral("QQuickItemChangeListener(0x%1)").arg(quintptr(this), 0, 16);
    }
    virtual void addSourceItem(QQuickItem *) {}
    virtual void removeSourceItem(QQuickItem *) {}
};

/*!
    \internal

    Helper class using CRTP to add tracking of listeners to a
    QQuickItemChangeListener implementation. Instantiated with the
    class that implements the QQuickItemChangeListener interface, like

    \code
    class QQuickThingPrivate : public QQuickItemPrivate,
                               public QSafeQuickItemChangeListener<QQuickThingPrivate>
    \endcode

    When destroyed, QSafeQuickItemChangeListener will emit warning messages
    in \c{-developer-build} configurations when listeners are still registered.
    As the listener is at this point partially destroyed, any call to the
    listener interface might result in undefined behavior, such as use-after-free.

    Notifications to registered listeners will in addition check if the listener
    is already in the destructor, and warn if that's the case. The \c{baseDeleted}
    override checks for an \c inDestructor or \c wasDeleted data member of the
    class implementing the QSafeQuickItemChangeListener interface.
*/
template<typename Self>
class QSafeQuickItemChangeListener : public QQuickItemChangeListener
{
public:
    ~QSafeQuickItemChangeListener() override
    {
#ifdef QT_BUILD_INTERNAL
        for (const auto &sourceItem : std::as_const(m_sourceItems)) {
            if (sourceItem)
                qCritical() << "Change Listener is still registered with item" << sourceItem;
        }
#endif
    }

    template <typename T>
    using InDestructorTest = decltype(T::inDestructor);
    template <typename T>
    using WasDeletedTest = decltype(T::wasDeleted);

    bool baseDeleted(const QObject *caller) const override
    {
        Q_UNUSED(caller);
        const Self *self = static_cast<const Self *>(this);
        if constexpr (qxp::is_detected_v<InDestructorTest, Self>) {
            bool same = false;
            if constexpr (std::is_convertible_v<Self *, QObjectPrivate *>)
                same = self->q_ptr == caller;
            return !same && self->inDestructor;
        } else if constexpr (qxp::is_detected_v<WasDeletedTest, Self>) {
            bool same = false;
            if constexpr (std::is_convertible_v<Self *, QObjectPrivate *>)
                same = self->q_ptr == caller;
            return !same && self->wasDeleted;
        } else if constexpr (std::is_convertible_v<Self *, QQuickItem *>) {
            return self != caller && QQmlData::wasDeleted(self);
        } else if constexpr (std::is_convertible_v<Self *, QObject *>) {
            return self != caller && QQmlData::wasDeleted(self);
        } else {
            static_assert(QtPrivate::type_dependent_false<Self>(),
                          "Don't know where destruction state is stored");
        }
    }

    QString debugName() const override
    {
        QString result;
        QDebug dbg(&result);
        dbg.nospace().noquote();
        const Self *self = static_cast<const Self *>(this);
        if constexpr (std::is_convertible_v<Self *, QObject *>) {
            dbg << self;
        } else if constexpr (std::is_convertible_v<Self *, QObjectPrivate *>) {
            dbg << self->q_ptr << "::d_ptr";
        } else {
            dbg << QMetaType::fromType<Self>().name() << '('
                << reinterpret_cast<const void *>(self) << ')';
        }
        return result;
    }

#ifdef QT_BUILD_INTERNAL
    void addSourceItem(QQuickItem *item) override {
        m_sourceItems << item;
    }
    void removeSourceItem(QQuickItem *item) override {
        m_sourceItems.removeAll(item);
    }

    QList<QPointer<QQuickItem>> m_sourceItems;
#endif
};

QT_END_NAMESPACE

#endif // QQUICKITEMCHANGELISTENER_P_H
