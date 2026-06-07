// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTEXTODFWRITER_H
#define QTEXTODFWRITER_H

#include <QtGui/private/qtguiglobal_p.h>

#ifndef QT_NO_TEXTODFWRITER

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qset.h>
#include <QtCore/qstack.h>
#include <QtCore/QXmlStreamWriter>

#include "qtextdocument_p.h"
#include "qtextdocumentwriter.h"

QT_BEGIN_NAMESPACE

class QTextDocumentPrivate;
class QTextCursor;
class QTextBlock;
class QIODevice;
class QXmlStreamWriter;
class QTextOdfWriterPrivate;
class QTextBlockFormat;
class QTextCharFormat;
class QTextListFormat;
class QTextFrameFormat;
class QTextTableCellFormat;
class QTextFrame;
class QTextFragment;
class QOutputStrategy;

class Q_AUTOTEST_EXPORT QTextOdfWriter {
public:
    QTextOdfWriter(const QTextDocument &document, QIODevice *device);
    bool writeAll();

    void setCreateArchive(bool on) { m_createArchive = on; }
    bool createArchive() const { return m_createArchive; }

    void writeBlock(QXmlStreamWriter &writer, const QTextBlock &block);
    void writeFormats(QXmlStreamWriter &writer, const QSet<int> &formatIds) const;
    void writeBlockFormat(QXmlStreamWriter &writer, QTextBlockFormat format, int formatIndex) const;
    void writeCharacterFormat(QXmlStreamWriter &writer, QTextCharFormat format, int formatIndex) const;
    void writeListFormat(QXmlStreamWriter &writer, QTextListFormat format, int formatIndex) const;
    void writeFrameFormat(QXmlStreamWriter &writer, QTextFrameFormat format, int formatIndex) const;
    void writeTableFormat(QXmlStreamWriter &writer, QTextTableFormat format, int formatIndex) const;
    void writeTableCellFormat(QXmlStreamWriter &writer, QTextTableCellFormat format,
                              int formatIndex, QList<QTextFormat> &styles) const;
    void writeFrame(QXmlStreamWriter &writer, const QTextFrame *frame);
    void writeInlineCharacter(QXmlStreamWriter &writer, const QTextFragment &fragment) const;

    const QString officeNS, textNS, styleNS, foNS, tableNS, drawNS, xlinkNS, svgNS;
    const int defaultImageResolution = 11811; // 11811 dots per meter = (about) 300 dpi

protected:
    void tableCellStyleElement(QXmlStreamWriter &writer, const int &formatIndex,
                               const QTextTableCellFormat &format,
                               bool hasBorder, int tableId = 0,
                               const QTextTableFormat tableFormatTmp = QTextTableFormat()) const;

private:
    const QTextDocument *m_document;
    QIODevice *m_device;

    QOutputStrategy *m_strategy;
    bool m_createArchive;

    QStack<QTextList *> m_listStack;

    QHash<int, QList<int>> m_cellFormatsInTablesWithBorders;
    QSet<int> m_tableFormatsWithBorders;
    mutable QSet<int> m_tableFormatsWithColWidthConstraints;
};

QT_END_NAMESPACE

#endif // QT_NO_TEXTODFWRITER
#endif // QTEXTODFWRITER_H
