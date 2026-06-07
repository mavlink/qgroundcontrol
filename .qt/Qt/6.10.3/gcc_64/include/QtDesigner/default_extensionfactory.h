// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef DEFAULT_EXTENSIONFACTORY_H
#define DEFAULT_EXTENSIONFACTORY_H

#include <QtDesigner/extension_global.h>
#include <QtDesigner/extension.h>

#include <QtCore/qmap.h>
#include <QtCore/qhash.h>
#include <QtCore/qpair.h>

QT_BEGIN_NAMESPACE

class QExtensionManager;

class QDESIGNER_EXTENSION_EXPORT QExtensionFactory : public QObject, public QAbstractExtensionFactory
{
    Q_OBJECT
    Q_INTERFACES(QAbstractExtensionFactory)
public:
    explicit QExtensionFactory(QExtensionManager *parent = nullptr);

    QObject *extension(QObject *object, const QString &iid) const override;
    QExtensionManager *extensionManager() const;

private Q_SLOTS:
    void objectDestroyed(QObject *object);

protected:
    virtual QObject *createExtension(QObject *object, const QString &iid, QObject *parent) const;

private:
    mutable QMap<std::pair<QString, QObject *>, QObject *> m_extensions;
    // ### FIXME Qt 7: Use QSet, add out of line destructor.
    mutable QHash<QObject*, bool>  m_extended;
};

QT_END_NAMESPACE

#endif // DEFAULT_EXTENSIONFACTORY_H
