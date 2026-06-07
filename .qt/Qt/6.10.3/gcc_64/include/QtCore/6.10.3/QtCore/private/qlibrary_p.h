// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLIBRARY_P_H
#define QLIBRARY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qlibrary.h"

#include "QtCore/private/qplugin_p.h"
#include "QtCore/qloggingcategory.h"
#include "QtCore/qmutex.h"
#include "QtCore/qplugin.h"
#include "QtCore/qpointer.h"
#include "QtCore/qstringlist.h"
#ifdef Q_OS_WIN
#  include "QtCore/qt_windows.h"
#endif

#include <memory>

QT_REQUIRE_CONFIG(library);

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qt_lcDebugPlugins)

struct QLibraryScanResult
{
    qsizetype pos;
    qsizetype length;
#if defined(Q_OF_MACH_O)
    bool isEncrypted = false;
#endif
};

class QLibraryStore;
class QLibraryPrivate
{
public:
#ifdef Q_OS_WIN
    using Handle = HINSTANCE;
#else
    using Handle = void *;
#endif
    enum UnloadFlag { UnloadSys, NoUnloadSys };

    struct Deleter {
        // QLibraryPrivate::release() is not, yet, and cannot easily be made, noexcept:
        void operator()(QLibraryPrivate *p) const { p->release(); }
    };
    using UniquePtr = std::unique_ptr<QLibraryPrivate, Deleter>;

    const QString fileName;
    const QString fullVersion;

    bool load();
    QtPluginInstanceFunction loadPlugin(); // loads and resolves instance
    Q_AUTOTEST_EXPORT bool unload(UnloadFlag flag = UnloadSys);
    void release();
    QFunctionPointer resolve(const char *);

    QLibrary::LoadHints loadHints() const
    { return QLibrary::LoadHints(loadHintsInt.loadRelaxed()); }
    void setLoadHints(QLibrary::LoadHints lh);
    QObject *pluginInstance();

    static QLibraryPrivate *findOrCreate(const QString &fileName, const QString &version = QString(),
                                         QLibrary::LoadHints loadHints = { });
    static QStringList suffixes_sys(const QString &fullVersion);
    static constexpr QStringView prefix_sys()
    {
#ifdef Q_OS_WIN
        return {};
#elif defined(Q_OS_CYGWIN)
        return u"cyg";
#else
        return u"lib";
#endif
    }

    QAtomicPointer<std::remove_pointer<QtPluginInstanceFunction>::type> instanceFactory;
    QAtomicPointer<std::remove_pointer<Handle>::type> pHnd;

    // the mutex protects the fields below
    QMutex mutex;
    QPointer<QObject> inst;         // used by QFactoryLoader
    QPluginParsedMetaData metaData;
    QString errorString;
    QString qualifiedFileName;

    void updatePluginState();
    bool isPlugin();

    static QLibraryPrivate* get(QLibrary* lib)
    {
        return lib->d.data();
    }

private:
    explicit QLibraryPrivate(const QString &canonicalFileName, const QString &version, QLibrary::LoadHints loadHints);
    ~QLibraryPrivate();
    void mergeLoadHints(QLibrary::LoadHints loadHints);

    bool load_sys();
    bool unload_sys();
    QFunctionPointer resolve_sys(const char *);

    QAtomicInt loadHintsInt;

    /// counts how many QLibrary or QPluginLoader are attached to us, plus 1 if it's loaded
    QAtomicInt libraryRefCount;
    /// counts how many times load() or loadPlugin() were called
    QAtomicInt libraryUnloadCount;

    enum { IsAPlugin, IsNotAPlugin, MightBeAPlugin } pluginState;
    friend class QLibraryStore;
};

QT_END_NAMESPACE

#endif // QLIBRARY_P_H
