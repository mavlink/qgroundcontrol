// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTEXTDOCUMENT_P_H
#define QQUICKTEXTDOCUMENT_P_H

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

#include "qquicktextdocument.h"

#include <QtGui/qabstracttextdocumentlayout.h>
#include <QtGui/qtextdocument.h>
#include <QtGui/qtextdocumentfragment.h>
#include <QtGui/qtextformat.h>
#include <QtCore/qrect.h>
#include <QtCore/private/qobject_p_p.h>

#if QT_CONFIG(mimetype)
#include <QtCore/qmimedatabase.h>
#endif

QT_BEGIN_NAMESPACE

class QQuickPixmap;
class QQuickTextEdit;

/*! \internal
    QTextImageHandler would attempt to resolve relative paths, and load the
    image itself if the document returns an invalid image from loadResource().
    We replace it with this version instead, because Qt Quick's text resources
    are resolved against the Text item's context, and because we override
    intrinsicSize(). drawObject() is empty because we don't need to use this
    handler to paint images: they get put into scene graph nodes instead.
*/
class QQuickTextImageHandler : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
public:
    QQuickTextImageHandler(QObject *parent = nullptr);
    ~QQuickTextImageHandler() override = default;
    QSizeF intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format) override;
    void drawObject(QPainter *, const QRectF &, QTextDocument *, int, const QTextFormat &) override { }
};

class QQuickTextDocumentPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickTextDocument)
public:
    static QQuickTextDocumentPrivate *get(QQuickTextDocument *doc) { return doc->d_func(); }
    static const QQuickTextDocumentPrivate *get(const QQuickTextDocument *doc) { return doc->d_func(); }

    void load();
    void writeTo(const QUrl &fileUrl);
    QTextDocument *document() const;
    void setDocument(QTextDocument *doc);
    void setStatus(QQuickTextDocument::Status s, const QString &err);

    // so far the QQuickItem given to the QQuickTextDocument ctor is always a QQuickTextEdit
    QQuickTextEdit *editor = nullptr;
    QUrl url;
    QString errorString;
    Qt::TextFormat detectedFormat = Qt::AutoText; // url's extension, independent of TextEdit.textFormat
    std::optional<QStringConverter::Encoding> encoding; // only relevant for HTML (Qt::RichText)
    QQuickTextDocument::Status status = QQuickTextDocument::Status::Null;
};

namespace QtPrivate {
class ProtectedLayoutAccessor: public QAbstractTextDocumentLayout
{
public:
    inline QTextCharFormat formatAccessor(int pos)
    {
        return format(pos);
    }
};
} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QQUICKTEXTDOCUMENT_P_H
