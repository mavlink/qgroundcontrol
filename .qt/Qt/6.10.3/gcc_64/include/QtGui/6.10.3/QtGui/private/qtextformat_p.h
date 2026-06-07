// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTEXTFORMAT_P_H
#define QTEXTFORMAT_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include "QtGui/qtextformat.h"
#include "QtCore/qlist.h"
#include <QtCore/qhash.h> // QMultiHash

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QTextFormatCollection
{
public:
    QTextFormatCollection() {}
    ~QTextFormatCollection();

    void clear();

    inline QTextFormat objectFormat(int objectIndex) const
    { return format(objectFormatIndex(objectIndex)); }
    inline void setObjectFormat(int objectIndex, const QTextFormat &format)
    { setObjectFormatIndex(objectIndex, indexForFormat(format)); }

    int objectFormatIndex(int objectIndex) const;
    void setObjectFormatIndex(int objectIndex, int formatIndex);

    int createObjectIndex(const QTextFormat &f);

    int indexForFormat(const QTextFormat &f);
    bool hasFormatCached(const QTextFormat &format) const;

    QTextFormat format(int idx) const;
    inline QTextBlockFormat blockFormat(int index) const
    { return format(index).toBlockFormat(); }
    inline QTextCharFormat charFormat(int index) const
    { return format(index).toCharFormat(); }
    inline QTextListFormat listFormat(int index) const
    { return format(index).toListFormat(); }
    inline QTextTableFormat tableFormat(int index) const
    { return format(index).toTableFormat(); }
    inline QTextImageFormat imageFormat(int index) const
    { return format(index).toImageFormat(); }

    inline int numFormats() const { return formats.size(); }

    typedef QList<QTextFormat> FormatVector;

    FormatVector formats;
    QList<qint32> objFormats;
    QMultiHash<size_t,int> hashes;

    inline QFont defaultFont() const { return defaultFnt; }
    void setDefaultFont(const QFont &f);

    inline void setSuperScriptBaseline(qreal baseline) { defaultFormat.setSuperScriptBaseline(baseline); }
    inline void setSubScriptBaseline(qreal baseline) { defaultFormat.setSubScriptBaseline(baseline); }
    inline void setBaselineOffset(qreal baseline) { defaultFormat.setBaselineOffset(baseline); }

    inline QTextCharFormat defaultTextFormat() const { return defaultFormat; }

private:
    QFont defaultFnt;
    QTextCharFormat defaultFormat;

    Q_DISABLE_COPY_MOVE(QTextFormatCollection)
};

QT_END_NAMESPACE

#endif // QTEXTFORMAT_P_H
