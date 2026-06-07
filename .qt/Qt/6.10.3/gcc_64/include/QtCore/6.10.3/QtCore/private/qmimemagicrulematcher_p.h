// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QMIMEMAGICRULEMATCHER_P_H
#define QMIMEMAGICRULEMATCHER_P_H

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

#include "qmimemagicrule_p.h"

QT_REQUIRE_CONFIG(mimetype);

#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QMimeMagicRuleMatcher
{
public:
    explicit QMimeMagicRuleMatcher(const QString &mime, unsigned priority = 65535);

    void swap(QMimeMagicRuleMatcher &other) noexcept
    {
        qSwap(m_list,     other.m_list);
        qSwap(m_priority, other.m_priority);
        qSwap(m_mimetype, other.m_mimetype);
    }

    bool operator==(const QMimeMagicRuleMatcher &other) const;

    void addRule(const QMimeMagicRule &rule);
    void addRules(const QList<QMimeMagicRule> &rules);
    QList<QMimeMagicRule> magicRules() const;

    bool matches(const QByteArray &data) const;

    unsigned priority() const;

    QString mimetype() const { return m_mimetype; }

private:
    QList<QMimeMagicRule> m_list;
    unsigned m_priority;
    QString m_mimetype;
};
Q_DECLARE_SHARED(QMimeMagicRuleMatcher)

QT_END_NAMESPACE

#endif // QMIMEMAGICRULEMATCHER_P_H
