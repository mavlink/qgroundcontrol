// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef HTMLHIGHLIGHTER_H
#define HTMLHIGHLIGHTER_H

#include <QtGui/qsyntaxhighlighter.h>

QT_BEGIN_NAMESPACE

class QTextEdit;

namespace qdesigner_internal {

/* HTML syntax highlighter based on Qt Quarterly example */
class HtmlHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    enum Construct {
        Entity,
        Tag,
        Comment,
        Attribute,
        Value,
        LastConstruct = Value
    };

    HtmlHighlighter(QTextEdit *textEdit);

    void setFormatFor(Construct construct, const QTextCharFormat &format);

    QTextCharFormat formatFor(Construct construct) const
    { return m_formats[construct]; }

protected:
    enum State {
        NormalState = -1,
        InComment,
        InTag
    };

    void highlightBlock(const QString &text) override;

private:
    QTextCharFormat m_formats[LastConstruct + 1];
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // HTMLHIGHLIGHTER_H
