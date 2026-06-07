// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMINDENTIGLINEWRITER_P
#define QQMLDOMINDENTIGLINEWRITER_P

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

#include "qqmldom_global.h"
#include "qqmldomcodeformatter_p.h"
#include "qqmldomlinewriter_p.h"

#include <QtQml/private/qqmljssourcelocation_p.h>
#include <QtCore/QAtomicInt>
#include <functional>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

QMLDOM_EXPORT class IndentingLineWriter : public LineWriter
{
    Q_GADGET
public:
    IndentingLineWriter(const SinkF &innerSink, const QString &fileName,
                        const LineWriterOptions &options = LineWriterOptions(),
                        const FormatTextStatus &initialStatus = FormatTextStatus::initialStatus(),
                        int lineNr = 0, int columnNr = 0, int utf16Offset = 0,
                        QString currentLine = QString())
        : LineWriter(innerSink, fileName, options, lineNr, columnNr, utf16Offset, currentLine),
          m_preCachedStatus(initialStatus)
    {
    }
    void reindentAndSplit(const QString &eol, bool eof = false) override;
    void handleTrailingSpace();
    FormatPartialStatus &fStatus();

    void lineChanged() override { m_fStatusValid = false; }
    void willCommit() override;
    bool reindent() const { return m_reindent; }
    void setReindent(bool v) { m_reindent = v; }

    void splitOnMaxLength(const QString &eol, bool eof);

private:
    Q_DISABLE_COPY_MOVE(IndentingLineWriter)
    int findSplitLocation(const QList<Token> &tokens, int minSplitLength);
protected:
    FormatTextStatus m_preCachedStatus;
    bool m_fStatusValid = false;
    FormatPartialStatus m_fStatus;
};

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
#endif
