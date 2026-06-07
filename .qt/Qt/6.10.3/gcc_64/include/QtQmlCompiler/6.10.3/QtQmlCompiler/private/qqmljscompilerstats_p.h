// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSCOMPILERSTATS_P_H
#define QQMLJSCOMPILERSTATS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <qtqmlcompilerexports.h>

#include <QHash>
#include <QJsonDocument>

#include <private/qqmljsdiagnosticmessage_p.h>
#include <private/qqmljssourcelocation_p.h>

#include <chrono>
#include <memory>

QT_BEGIN_NAMESPACE

namespace QQmlJS {

enum class CodegenResult : quint8 { Success, Skip, Failure };

struct Q_QMLCOMPILER_EXPORT AotStatsEntry
{
    std::chrono::microseconds codegenDuration;
    QString functionName;
    QString message;
    int line = 0;
    int column = 0;
    CodegenResult codegenResult = CodegenResult::Success;

    bool operator<(const AotStatsEntry &) const;
};

class Q_QMLCOMPILER_EXPORT AotStats
{
    friend class QQmlJSAotCompilerStats;

public:
    const QHash<QString, QHash<QString, QList<AotStatsEntry>>> &entries() const
    {
        return m_entries;
    }

    void registerFile(const QString &moduleId, const QString &filepath);
    void addEntry(const QString &moduleId, const QString &filepath, const AotStatsEntry &entry);
    void insert(const AotStats &other);

    static std::optional<QStringList> readAllLines(const QString &path);
    bool saveToDisk(const QString &filepath) const;

    static std::optional<AotStats> parseAotstatsFile(const QString &aotstatsPath);
    static std::optional<AotStats> aggregateAotstatsList(const QString &aotstatsListPath);

    static std::optional<AotStats> fromJsonDocument(const QJsonDocument &);
    QJsonDocument toJsonDocument() const;

private:
    // module Id -> filename -> stats m_entries
    QHash<QString, QHash<QString, QList<AotStatsEntry>>> m_entries;
};

class Q_QMLCOMPILER_EXPORT QQmlJSAotCompilerStats
{
public:
    static AotStats *instance() { return s_instance.get(); }

    static bool recordAotStats() { return s_recordAotStats; }
    static void setRecordAotStats(bool recordAotStats) { s_recordAotStats = recordAotStats; }

    static QString moduleId() { return s_moduleId; }
    static void setModuleId(const QString &moduleId) { s_moduleId = moduleId; }

    static void registerFile(const QString &filepath);
    static void addEntry(const QString &filepath, const QQmlJS::AotStatsEntry &entry);

private:
    static std::unique_ptr<AotStats> s_instance;
    static QString s_moduleId;
    static bool s_recordAotStats;
};

} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLJSCOMPILERSTATS_P_H
