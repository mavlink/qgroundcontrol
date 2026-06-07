// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QABSTRACTTEXTDOCUMENTLAYOUT_P_H
#define QABSTRACTTEXTDOCUMENTLAYOUT_P_H

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
#include "private/qobject_p.h"
#include "qtextdocument_p.h"
#include "qabstracttextdocumentlayout.h"

#include "QtCore/qhash.h"
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

struct QTextObjectHandler
{
    QTextObjectHandler() : iface(nullptr) {}
    QTextObjectInterface *iface;
    QPointer<QObject> component;
};
typedef QHash<int, QTextObjectHandler> HandlerHash;

class Q_GUI_EXPORT QAbstractTextDocumentLayoutPrivate : public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QAbstractTextDocumentLayout)

    inline QAbstractTextDocumentLayoutPrivate()
        : paintDevice(nullptr) {}
    ~QAbstractTextDocumentLayoutPrivate();

    inline void setDocument(QTextDocument *doc) {
        document = doc;
        docPrivate = nullptr;
        if (doc)
            docPrivate = QTextDocumentPrivate::get(doc);
    }

    static QAbstractTextDocumentLayoutPrivate *get(QAbstractTextDocumentLayout *layout)
    {
        return layout->d_func();
    }

    bool hasHandlers() const
    {
        return !handlers.isEmpty();
    }

    inline int _q_dynamicPageCountSlot() const
    { return q_func()->pageCount(); }
    inline QSizeF _q_dynamicDocumentSizeSlot() const
    { return q_func()->documentSize(); }

    HandlerHash handlers;

    void _q_handlerDestroyed(QObject *obj);
    QPaintDevice *paintDevice;

    QTextDocument *document;
    QTextDocumentPrivate *docPrivate;
};

QT_END_NAMESPACE

#endif // QABSTRACTTEXTDOCUMENTLAYOUT_P_H
