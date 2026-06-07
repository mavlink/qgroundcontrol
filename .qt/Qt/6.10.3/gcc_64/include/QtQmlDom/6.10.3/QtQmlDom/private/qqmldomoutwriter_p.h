// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMLDOMOUTWRITER_P_H
#define QMLDOMOUTWRITER_P_H

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
#include "qqmldom_fwd_p.h"
#include "qqmldomlinewriter_p.h"
#include "qqmldomcomments_p.h"

#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

class QMLDOM_EXPORT OutWriter
{
public:
    int indent = 0;
    int indenterId = -1;
    bool indentNextlines = false;
    bool skipComments = false;
    LineWriter &lineWriter;
    Path currentPath;
    QString writtenStr;
    using RegionToCommentMap = QMap<FileLocationRegion, CommentedElement>;
    QStack<RegionToCommentMap> pendingComments;

    explicit OutWriter(LineWriter &lw) : lineWriter(lw)
    {
        lineWriter.addInnerSink([this](QStringView s) { writtenStr.append(s); });
        indenterId =
                lineWriter.addTextAddCallback([this](LineWriter &, LineWriter::TextAddType tt) {
                    if (indentNextlines && tt == LineWriter::TextAddType::Normal
                        && QStringView(lineWriter.currentLine()).trimmed().isEmpty())
                        lineWriter.setLineIndent(indent);
                    return true;
                });
    }

    int increaseIndent(int level = 1)
    {
        int oldIndent = indent;
        indent += lineWriter.options().formatOptions.indentSize * level;
        return oldIndent;
    }
    int decreaseIndent(int level = 1, int expectedIndent = -1)
    {
        indent -= lineWriter.options().formatOptions.indentSize * level;
        Q_ASSERT(expectedIndent < 0 || expectedIndent == indent);
        return indent;
    }

    void itemStart(const DomItem &it);
    void itemEnd();
    void regionStart(FileLocationRegion region);
    void regionEnd(FileLocationRegion regino);

    quint32 counter() const { return lineWriter.counter(); }
    OutWriter &writeRegion(FileLocationRegion region, QStringView toWrite);
    OutWriter &writeRegion(FileLocationRegion region);
    OutWriter &ensureNewline(int nNewlines = 1)
    {
        lineWriter.ensureNewline(nNewlines);
        return *this;
    }
    OutWriter &ensureSpace()
    {
        lineWriter.ensureSpace();
        return *this;
    }
    OutWriter &ensureSpace(QStringView space)
    {
        lineWriter.ensureSpace(space);
        return *this;
    }
    OutWriter &newline()
    {
        lineWriter.newline();
        return *this;
    }
    OutWriter &write(QStringView v, LineWriter::TextAddType t = LineWriter::TextAddType::Normal)
    {
        lineWriter.write(v, t);
        return *this;
    }
    void flush() { lineWriter.flush(); }
    void eof(bool ensureNewline = true) { lineWriter.eof(ensureNewline); }
    int addNewlinesAutospacerCallback(int nLines)
    {
        return lineWriter.addNewlinesAutospacerCallback(nLines);
    }
    int addTextAddCallback(std::function<bool(LineWriter &, LineWriter::TextAddType)> callback)
    {
        return lineWriter.addTextAddCallback(callback);
    }
    bool removeTextAddCallback(int i) { return lineWriter.removeTextAddCallback(i); }

};

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
#endif // QMLDOMOUTWRITER_P_H
