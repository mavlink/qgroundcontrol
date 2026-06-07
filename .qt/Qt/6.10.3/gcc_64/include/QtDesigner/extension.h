// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef EXTENSION_H
#define EXTENSION_H

#include <QtDesigner/extension_global.h>

#include <QtCore/qstring.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

#define Q_TYPEID(IFace) QLatin1StringView(IFace##_iid)

class QDESIGNER_EXTENSION_EXPORT QAbstractExtensionFactory
{
public:
    virtual ~QAbstractExtensionFactory();

    virtual QObject *extension(QObject *object, const QString &iid) const = 0;
};
Q_DECLARE_INTERFACE(QAbstractExtensionFactory, "org.qt-project.Qt.QAbstractExtensionFactory")

class QDESIGNER_EXTENSION_EXPORT QAbstractExtensionManager
{
public:
    virtual ~QAbstractExtensionManager();

    virtual void registerExtensions(QAbstractExtensionFactory *factory, const QString &iid) = 0;
    virtual void unregisterExtensions(QAbstractExtensionFactory *factory, const QString &iid) = 0;

    virtual QObject *extension(QObject *object, const QString &iid) const = 0;
};
Q_DECLARE_INTERFACE(QAbstractExtensionManager, "org.qt-project.Qt.QAbstractExtensionManager")

template <class T>
inline T qt_extension(QAbstractExtensionManager *, QObject *)
{ return nullptr; }

#define Q_DECLARE_EXTENSION_INTERFACE(IFace, IId) \
const char * const IFace##_iid = IId; \
Q_DECLARE_INTERFACE(IFace, IId) \
template <> inline IFace *qt_extension<IFace *>(QAbstractExtensionManager *manager, QObject *object) \
{ QObject *extension = manager->extension(object, Q_TYPEID(IFace)); return extension ? static_cast<IFace *>(extension->qt_metacast(IFace##_iid)) : static_cast<IFace *>(nullptr); }

QT_END_NAMESPACE

#endif // EXTENSION_H
