// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFACTORYLOADER_P_H
#define QFACTORYLOADER_P_H

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

#include "QtCore/qglobal.h"
#ifndef QT_NO_QOBJECT

#include "QtCore/private/qplugin_p.h"
#include "QtCore/private/qduplicatetracker_p.h"
#include "QtCore/qcoreapplication.h"
#include "QtCore/qmap.h"
#include "QtCore/qmutex.h"
#include "QtCore/qobject.h"
#include "QtCore/qplugin.h"

#if QT_CONFIG(library)
#  include "QtCore/private/qlibrary_p.h"
#endif

QT_BEGIN_NAMESPACE

class QJsonObject;
class QLibraryPrivate;

class Q_CORE_EXPORT QFactoryLoader
{
    Q_DECLARE_TR_FUNCTIONS(QFactoryLoader);

public:
    explicit QFactoryLoader(const char *iid,
                   const QString &suffix = QString(),
                   Qt::CaseSensitivity = Qt::CaseSensitive);

#if QT_CONFIG(library)
    ~QFactoryLoader();

    void setLoadHints(QLibrary::LoadHints hints);
    void update();
    static void refreshAll();

#if defined(Q_OS_UNIX) && !defined (Q_OS_DARWIN)
    QLibraryPrivate *library(const QString &key) const;
#endif // Q_OS_UNIX && !Q_OS_DARWIN
#endif // QT_CONFIG(library)

    void setExtraSearchPath(const QString &path);
    QMultiMap<int, QString> keyMap() const;
    int indexOf(const QString &needle) const;

    using MetaDataList = QList<QPluginParsedMetaData>;

    MetaDataList metaData() const;
    QList<QCborArray> metaDataKeys() const;
    QObject *instance(int index) const;

private:
    struct Private {
        QByteArray iid;
        mutable QMutex mutex;
        mutable QList<QtPluginInstanceFunction> usedStaticInstances;
#if QT_CONFIG(library)
        QDuplicateTracker<QString> loadedPaths;
        std::vector<QLibraryPrivate::UniquePtr> libraries;
        mutable QList<bool> loadedLibraries;
        std::map<QString, QLibraryPrivate*> keyMap;
        QString suffix;
        QString extraSearchPath;
        Qt::CaseSensitivity cs;
        QLibrary::LoadHints loadHints;
        void updateSinglePath(const QString &pluginDir);
#endif

        // for compat when we d was a pointer
        auto operator->() { return this; }
        auto operator->() const { return this; }
    } d;

    inline QObject *instanceHelper_locked(int index) const;
};

template <class PluginInterface, class FactoryInterface, typename ...Args>
PluginInterface *qLoadPlugin(const QFactoryLoader *loader, const QString &key, Args &&...args)
{
    const int index = loader->indexOf(key);
    if (index != -1) {
        QObject *factoryObject = loader->instance(index);
        if (FactoryInterface *factory = qobject_cast<FactoryInterface *>(factoryObject))
            if (PluginInterface *result = factory->create(key, std::forward<Args>(args)...))
                return result;
    }
    return nullptr;
}

template <class PluginInterface, class FactoryInterface, typename Arg>
Q_DECL_DEPRECATED PluginInterface *qLoadPlugin1(const QFactoryLoader *loader, const QString &key, Arg &&arg)
{ return qLoadPlugin<PluginInterface, FactoryInterface>(loader, key, std::forward<Arg>(arg)); }

QT_END_NAMESPACE

#endif // QT_NO_QOBJECT

#endif // QFACTORYLOADER_P_H
