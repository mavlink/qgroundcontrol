// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTEXTDOCUMENT_H
#define QQUICKTEXTDOCUMENT_H

#include <QtGui/QTextDocument>
#include <QtQuick/QQuickItem>

QT_BEGIN_NAMESPACE

class QQuickTextDocumentPrivate;
class Q_QUICK_EXPORT QQuickTextDocument : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged REVISION(6, 7))
    Q_PROPERTY(bool modified READ isModified WRITE setModified NOTIFY modifiedChanged REVISION(6, 7))
    Q_PROPERTY(Status status READ status NOTIFY statusChanged REVISION(6, 7))
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged REVISION(6, 7))

    QML_NAMED_ELEMENT(TextDocument)
    QML_UNCREATABLE("TextDocument is only available as a property of TextEdit or TextArea.")
    QML_ADDED_IN_VERSION(2, 0)

public:
    enum class Status : quint8 {
        Null = 0,
        Loading,
        Loaded,
        Saving,
        Saved,
        ReadError,
        WriteError,
        NonLocalFileError,
    };
    Q_ENUM(Status)

    QQuickTextDocument(QQuickItem *parent);

    QUrl source() const;
    void setSource(const QUrl &url);

    bool isModified() const;
    void setModified(bool modified);

    QTextDocument *textDocument() const;
    void setTextDocument(QTextDocument *document);

    Q_REVISION(6, 7) Q_INVOKABLE void save();
    Q_REVISION(6, 7) Q_INVOKABLE void saveAs(const QUrl &url);

    Status status() const;
    QString errorString() const;

Q_SIGNALS:
    Q_REVISION(6,7) void textDocumentChanged();
    Q_REVISION(6, 7) void sourceChanged();
    Q_REVISION(6, 7) void modifiedChanged();
    Q_REVISION(6, 7) void statusChanged();
    Q_REVISION(6, 7) void errorStringChanged();

private:
    Q_DISABLE_COPY(QQuickTextDocument)
    Q_DECLARE_PRIVATE(QQuickTextDocument)
};

QT_END_NAMESPACE

#endif
