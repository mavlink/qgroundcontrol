// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLLSCOMPLETIONCONTEXTSTRINGS_H
#define QQMLLSCOMPLETIONCONTEXTSTRINGS_H

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

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringview.h>

QT_BEGIN_NAMESPACE

// finds the filter string, the base (for fully qualified accesses) and the whole string
// just before pos in code
struct CompletionContextStrings
{
    CompletionContextStrings(QString code, qsizetype pos);

public:
    // line up until pos
    QStringView preLine() const
    {
        return QStringView(m_code).mid(m_lineStart, m_pos - m_lineStart);
    }
    // the part used to filter the completion (normally actual filtering is left to the client)
    QStringView filterChars() const
    {
        return QStringView(m_code).mid(m_filterStart, m_pos - m_filterStart);
    }
    // the base part (qualified access)
    QStringView base() const
    {
        return QStringView(m_code).mid(m_baseStart, m_filterStart - m_baseStart);
    }
    // if we are at line start
    bool atLineStart() const { return m_atLineStart; }

    qsizetype offset() const { return m_pos; }

private:
    QString m_code; // the current code
    qsizetype m_pos = {}; // current position of the cursor
    qsizetype m_filterStart = {}; // start of the characters that are used to filter the suggestions
    qsizetype m_lineStart = {}; // start of the current line
    qsizetype m_baseStart = {}; // start of the dotted expression that ends at the cursor position
    bool m_atLineStart = {}; // if there are only spaces before base
};

QT_END_NAMESPACE

#endif // QQMLLSCOMPLETIONCONTEXTSTRINGS_H
