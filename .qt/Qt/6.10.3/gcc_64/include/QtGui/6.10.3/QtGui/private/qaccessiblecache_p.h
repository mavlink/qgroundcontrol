// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QACCESSIBLECACHE_P
#define QACCESSIBLECACHE_P

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/qobject.h>
#include <QtCore/qhash.h>

#include "qaccessible.h"

#if QT_CONFIG(accessibility)

Q_FORWARD_DECLARE_OBJC_CLASS(QT_MANGLE_NAMESPACE(QMacAccessibilityElement));

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QAccessibleCache  :public QObject
{
    Q_OBJECT

public:
    ~QAccessibleCache() override;
    static QAccessibleCache *instance();
    QAccessibleInterface *interfaceForId(QAccessible::Id id) const;
    QAccessible::Id idForInterface(QAccessibleInterface *iface) const;
    QAccessible::Id idForObject(QObject *obj) const;
    bool containsObject(QObject *obj) const;
    QAccessible::Id insert(QObject *object, QAccessibleInterface *iface) const;
    void deleteInterface(QAccessible::Id id, QObject *obj = nullptr);

#ifdef Q_OS_APPLE
    QT_MANGLE_NAMESPACE(QMacAccessibilityElement) *elementForId(QAccessible::Id axid) const;
    bool insertElement(QAccessible::Id axid, QT_MANGLE_NAMESPACE(QMacAccessibilityElement) *element) const;
#endif

private Q_SLOTS:
    void objectDestroyed(QObject *obj);

private:
    QAccessible::Id acquireId() const;

    mutable QHash<QAccessible::Id, QAccessibleInterface *> idToInterface;
    mutable QHash<QAccessibleInterface *, QAccessible::Id> interfaceToId;
    mutable QMultiHash<QObject *, std::pair<QAccessible::Id, const QMetaObject*>> objectToId;

#ifdef Q_OS_APPLE
    void removeAccessibleElement(QAccessible::Id axid);
    mutable QHash<QAccessible::Id, QT_MANGLE_NAMESPACE(QMacAccessibilityElement) *> accessibleElements;
#endif

    friend class QAccessible;
    friend class QAccessibleInterface;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif
