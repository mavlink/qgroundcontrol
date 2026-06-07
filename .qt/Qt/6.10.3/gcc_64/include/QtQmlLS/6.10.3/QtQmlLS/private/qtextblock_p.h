// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTEXTBLOCK_P_H
#define QTEXTBLOCK_P_H

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

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

namespace Utils {

class TextDocument;
class TextBlockUserData;

class TextBlock
{
public:
    bool isValid() const;

    void setBlockNumber(int blockNumber);
    int blockNumber() const;

    void setPosition(int position);
    int position() const;

    void setLength(int length);
    int length() const;

    TextBlock next() const;
    TextBlock previous() const;

    int userState() const;
    void setUserState(int state);

    bool isVisible() const;
    void setVisible(bool visible);

    void setLineCount(int count);
    int lineCount() const;

    void setDocument(TextDocument *document);
    TextDocument *document() const;

    QString text() const;

    int revision() const;
    void setRevision(int rev);

    friend bool operator==(const TextBlock &t1, const TextBlock &t2);
    friend bool operator!=(const TextBlock &t1, const TextBlock &t2);

private:
    TextDocument *m_document = nullptr;
    int m_revision = 0;

    int m_position = 0;
    int m_length = 0;
    int m_blockNumber = -1;
};

} // namespace Utils

QT_END_NAMESPACE

#endif // TEXTBLOCK_P_H
