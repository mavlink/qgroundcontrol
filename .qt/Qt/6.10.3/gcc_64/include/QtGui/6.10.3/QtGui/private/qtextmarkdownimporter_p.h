// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTEXTMARKDOWNIMPORTER_H
#define QTEXTMARKDOWNIMPORTER_H

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

#include <QtGui/qfont.h>
#include <QtGui/qtguiglobal.h>
#include <QtGui/qpalette.h>
#include <QtGui/qtextdocument.h>
#include <QtGui/qtextlist.h>
#include <QtCore/qpointer.h>
#include <QtCore/qstack.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QTextCursor;
class QTextDocument;
class QTextTable;

class Q_GUI_EXPORT QTextMarkdownImporter
{
public:
    enum Feature {
        FeatureCollapseWhitespace =       0x0001,
        FeaturePermissiveATXHeaders =     0x0002,
        FeaturePermissiveURLAutoLinks =   0x0004,
        FeaturePermissiveMailAutoLinks =  0x0008,
        FeatureNoIndentedCodeBlocks =     0x0010,
        FeatureNoHTMLBlocks =             0x0020,
        FeatureNoHTMLSpans =              0x0040,
        FeatureTables =                   0x0100,
        FeatureStrikeThrough =            0x0200,
        FeaturePermissiveWWWAutoLinks =   0x0400,
        FeatureTasklists =                0x0800,
        FeatureUnderline =                0x4000,
        FeatureFrontMatter =            0x100000, // Qt feature, not yet in MD4C
        // composite flags
        FeaturePermissiveAutoLinks = FeaturePermissiveMailAutoLinks
            | FeaturePermissiveURLAutoLinks | FeaturePermissiveWWWAutoLinks,
        FeatureNoHTML = QTextDocument::MarkdownNoHTML,
        DialectCommonMark = QTextDocument::MarkdownDialectCommonMark,
        DialectGitHub = QTextDocument::MarkdownDialectGitHub
    };
    Q_DECLARE_FLAGS(Features, Feature)

    QTextMarkdownImporter(QTextDocument *doc, Features features);
    QTextMarkdownImporter(QTextDocument *doc, QTextDocument::MarkdownFeatures features);

    void import(const QString &markdown);

public:
    // MD4C callbacks
    int cbEnterBlock(int blockType, void* detail);
    int cbLeaveBlock(int blockType, void* detail);
    int cbEnterSpan(int spanType, void* detail);
    int cbLeaveSpan(int spanType, void* detail);
    int cbText(int textType, const char* text, unsigned size);

private:
    void insertBlock();

private:
    QTextCursor m_cursor;
    QTextTable *m_currentTable = nullptr; // because m_cursor->currentTable() doesn't work
#if QT_CONFIG(regularexpression)
    QString m_htmlAccumulator;
#endif
    QString m_blockCodeLanguage;
    QList<int> m_nonEmptyTableCells; // in the current row
    QStack<QPointer<QTextList>> m_listStack;
    QStack<QTextCharFormat> m_spanFormatStack;
    QFont m_monoFont;
    QPalette m_palette;
#if QT_CONFIG(regularexpression)
    int m_htmlTagDepth = 0;
#endif
    int m_blockQuoteDepth = 0;
    int m_tableColumnCount = 0;
    int m_tableRowCount = 0;
    int m_tableCol = -1; // because relative cell movements (e.g. m_cursor->movePosition(QTextCursor::NextCell)) don't work
    int m_paragraphMargin = 0;
    int m_blockType = 0;
    char m_blockCodeFence = 0;
    Features m_features;
    QTextImageFormat m_imageFormat;
    QTextListFormat m_listFormat;
    QTextBlockFormat::MarkerType m_markerType = QTextBlockFormat::MarkerType::NoMarker;
    bool m_needsInsertBlock = false;
    bool m_needsInsertList = false;
    bool m_listItem = false; // true from the beginning of LI to the end of the first P
    bool m_codeBlock = false;
    bool m_imageSpan = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QTextMarkdownImporter::Features)

QT_END_NAMESPACE

#endif // QTEXTMARKDOWNIMPORTER_H
