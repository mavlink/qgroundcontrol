// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROXYMETAOBJECT_P_H
#define QQMLPROXYMETAOBJECT_P_H

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

#include <private/qtqmlglobal_p.h>
#include <private/qmetaobjectbuilder_p.h>

#include <QtCore/QMetaObject>
#include <QtCore/QObject>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class Q_QML_EXPORT QQmlProxyMetaObject : public QDynamicMetaObjectData
{
public:
    struct ProxyData {
        typedef QObject *(*CreateFunc)(QObject *);
        QMetaObject *metaObject;
        CreateFunc createFunc;
        int propertyOffset;
        int methodOffset;
    };

    QQmlProxyMetaObject(QObject *, const QList<ProxyData> *);
    ~QQmlProxyMetaObject();

    static constexpr int extensionObjectId(int id) noexcept
    {
        Q_ASSERT(id >= 0);
        Q_ASSERT(id <= MaxExtensionCount); // MaxExtensionCount is a valid index
        return ExtensionObjectId | id;
    }

protected:
    int metaCall(QObject *o, QMetaObject::Call _c, int _id, void **_a) override;
#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0)
    const QMetaObject *toDynamicMetaObject(QObject *) const override
#else
    QMetaObject *toDynamicMetaObject(QObject *) override
#endif
    {
        return metaObject;
    }
    void objectDestroyed(QObject *object) override;

private:
    QObject *getProxy(int index);

    const QList<ProxyData> *metaObjects;
    QObject **proxies;

    QDynamicMetaObjectData *parent;
    QMetaObject *metaObject;
    QObject *object;

    // ExtensionObjectId acts as a flag for whether we should interpret a
    // QMetaObject::CustomCall as a call to fetch the extension object (see
    // QQmlProxyMetaObject::metaCall()). MaxExtensionCount is a limit on how
    // many extensions we can query via such mechanism
    enum : int {
        MaxExtensionCount = 127, // magic number so that low bits are all '1'
        ExtensionObjectId = ~MaxExtensionCount,
    };
};

QT_END_NAMESPACE

#endif // QQMLPROXYMETAOBJECT_P_H

