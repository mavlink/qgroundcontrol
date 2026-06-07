// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTEXTOBJECT_P_H
#define QTEXTOBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include "QtGui/qtextobject.h"
#include "private/qobject_p.h"
#include "QtGui/qtextdocument.h"

QT_BEGIN_NAMESPACE

class QTextDocumentPrivate;

class QTextObjectPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QTextObject)
public:
    QTextObjectPrivate(QTextDocument *doc)
        : pieceTable(doc->d_func()), objectIndex(-1)
    {
    }
    QTextDocumentPrivate *pieceTable;
    int objectIndex;
};

class QTextBlockGroupPrivate : public QTextObjectPrivate
{
    Q_DECLARE_PUBLIC(QTextBlockGroup)
public:
    QTextBlockGroupPrivate(QTextDocument *doc)
        : QTextObjectPrivate(doc)
    {
    }
    typedef QList<QTextBlock> BlockList;
    BlockList blocks;
    void markBlocksDirty();
};

class QTextFrameLayoutData;

class QTextFramePrivate : public QTextObjectPrivate
{
    friend class QTextDocumentPrivate;
    Q_DECLARE_PUBLIC(QTextFrame)
public:
    QTextFramePrivate(QTextDocument *doc)
        : QTextObjectPrivate(doc), fragment_start(0), fragment_end(0), parentFrame(nullptr), layoutData(nullptr)
    {
    }
    virtual void fragmentAdded(QChar type, uint fragment);
    virtual void fragmentRemoved(QChar type, uint fragment);
    void remove_me();

    uint fragment_start;
    uint fragment_end;

    QTextFrame *parentFrame;
    QList<QTextFrame *> childFrames;
    QTextFrameLayoutData *layoutData;
};

QT_END_NAMESPACE

#endif // QTEXTOBJECT_P_H
