// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQMLTESTUTILS_P_H
#define QQMLTESTUTILS_P_H

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

#include <QtCore/QTemporaryDir>
#include <QtCore/QDir>
#include <QtCore/QUrl>
#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtTest/QTest>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

/* Base class for tests with data that are located in a "data" subfolder. */

class QQmlDataTest : public QObject
{
    Q_OBJECT
public:
    enum class FailOnWarningsPolicy {
        DoNotFailOnWarnings,
        FailOnWarnings
    };

    QQmlDataTest(
            const char *qmlTestDataDir,
            FailOnWarningsPolicy failOnWarningsPolicy = FailOnWarningsPolicy::DoNotFailOnWarnings,
            const char *dataSubdir = "data");
    virtual ~QQmlDataTest();

    QString testFile(const QString &fileName) const;
    inline QString testFile(const char *fileName) const
        { return testFile(QLatin1String(fileName)); }
    inline QUrl testFileUrl(const QString &fileName) const
        {
            const QString fn = testFile(fileName);
            return fn.startsWith(QLatin1Char(':'))
                ? QUrl(QLatin1String("qrc") + fn)
                : QUrl::fromLocalFile(fn);
        }
    inline QUrl testFileUrl(const char *fileName) const
        { return testFileUrl(QLatin1String(fileName)); }

    inline QString dataDirectory() const { return m_dataDirectory; }
    inline QUrl dataDirectoryUrl() const { return m_dataDirectoryUrl; }
    inline QString directory() const  { return m_directory; }

    static inline QQmlDataTest *instance() { return m_instance; }

    bool canImportModule(const QString &importTestQmlSource) const;

public Q_SLOTS:
    virtual void initTestCase();
    virtual void init();

private:
    static QQmlDataTest *m_instance;

    // The directory in which to search for m_dataSubDir.
    const char *m_qmlTestDataDir = nullptr;
    // The subdirectory containing the actual data. Typically "data"
    const char *m_dataSubDir = nullptr;
    // The path to m_dataSubDir, if found.
    const QString m_dataDirectory;
    const QUrl m_dataDirectoryUrl;
    QTemporaryDir m_cacheDir;
    QString m_directory;
    bool m_usesOwnCacheDir = false;
    FailOnWarningsPolicy m_failOnWarningsPolicy = FailOnWarningsPolicy::DoNotFailOnWarnings;
};

class QQmlTestMessageHandler
{
    Q_DISABLE_COPY(QQmlTestMessageHandler)
public:
    QQmlTestMessageHandler();
    ~QQmlTestMessageHandler();

    const QStringList &messages() const { return m_messages; }
    const QString messageString() const { return m_messages.join(QLatin1Char('\n')); }

    void clear() { m_messages.clear(); }

    void setIncludeCategoriesEnabled(bool enabled) { m_includeCategories = enabled; }

private:
    static void messageHandler(QtMsgType, const QMessageLogContext &context, const QString &message);

    static QQmlTestMessageHandler *m_instance;
    QStringList m_messages;
    QtMessageHandler m_oldHandler;
    bool m_includeCategories;
};

class QQmlEngine;

namespace QV4 {
struct ExecutionEngine;
}

enum class GCFlags {
    None = 0,
    DontSendPostedEvents = 1
};

bool gcDone(const QV4::ExecutionEngine *engine);
void gc(QV4::ExecutionEngine &engine, GCFlags flags = GCFlags::None);
bool gcDone(QQmlEngine *engine);
void gc(QQmlEngine &engine, GCFlags flags = GCFlags::None);

QT_END_NAMESPACE

#endif // QQMLTESTUTILS_P_H
