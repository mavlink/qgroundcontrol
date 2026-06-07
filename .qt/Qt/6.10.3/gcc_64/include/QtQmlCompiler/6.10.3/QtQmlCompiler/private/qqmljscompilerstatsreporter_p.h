// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSCOMPILERSTATSREPORTER_P_H
#define QQMLJSCOMPILERSTATSREPORTER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QTextStream>

#include <qtqmlcompilerexports.h>

#include <private/qqmljscompilerstats_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {

class Q_QMLCOMPILER_EXPORT AotStatsReporter
{
public:
    AotStatsReporter(const QQmlJS::AotStats &stats, const QStringList &emptyModules,
                     const QStringList &onlyBytecodeModules);

    QString format() const;

private:
    void formatDetailedStats(QTextStream &) const;
    void formatSummary(QTextStream &) const;
    QString formatSuccessRate(int codegens, int successes, int skips) const;

    const AotStats &m_aotstats;
    const QStringList &m_emptyModules;
    const QStringList &m_onlyBytecodeModules;

    struct Counters
    {
        int successes = 0;
        int skips = 0;
        int codegens = 0;
    };

    Counters m_totalCounters;
    QHash<QString, Counters> m_moduleCounters;
    QHash<QString, QHash<QString, Counters>> m_fileCounters;
    QList<std::chrono::microseconds> m_successDurations;
};

} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLJSCOMPILERSTATSREPORTER_P_H
