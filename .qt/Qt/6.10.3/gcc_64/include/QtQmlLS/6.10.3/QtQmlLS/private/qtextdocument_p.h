// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTEXTDOCUMENT_P_H
#define QTEXTDOCUMENT_P_H

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

#include "qtextblock_p.h"

#include <QtCore/qchar.h>
#include <QtCore/qvector.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qmutex.h>

#include <optional>

QT_BEGIN_NAMESPACE

namespace Utils {

class TextBlockUserData;

class TextDocument
{
public:
    TextDocument() = default;
    explicit TextDocument(const QString &text);

    TextBlock findBlockByNumber(int blockNumber) const;
    TextBlock findBlockByLineNumber(int lineNumber) const;
    QChar characterAt(int pos) const;
    int characterCount() const;
    TextBlock begin() const;
    TextBlock firstBlock() const;
    TextBlock lastBlock() const;

    std::optional<int> version() const;
    void setVersion(std::optional<int>);

    QString toPlainText() const;
    void setPlainText(const QString &text);

    bool isModified() const;
    void setModified(bool modified);

    void setUndoRedoEnabled(bool enable);

    void clear();

    void setUserState(int blockNumber, int state);
    int userState(int blockNumber) const;
    QMutex *mutex() const;

private:
    struct Block
    {
        TextBlock textBlock;
        int userState = -1;
    };

    QVector<Block> m_blocks;

    QString m_content;
    bool m_modified = false;
    std::optional<int> m_version;
    mutable QMutex m_mutex;
};
} // namespace Utils

QT_END_NAMESPACE

#endif // TEXTDOCUMENT_P_H
