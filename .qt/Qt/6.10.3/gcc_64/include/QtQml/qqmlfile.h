// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLFILE_H
#define QQMLFILE_H

#include <QtQml/qtqmlglobal.h>

QT_BEGIN_NAMESPACE

class QUrl;
class QString;
class QObject;
class QQmlEngine;
class QQmlFilePrivate;

class Q_QML_EXPORT QQmlFile
{
public:
    QQmlFile();
    QQmlFile(QQmlEngine *engine, const QUrl &url);
    QQmlFile(QQmlEngine *engine, const QString &url);
    ~QQmlFile();

    enum Status { Null, Ready, Error, Loading };

    bool isNull() const;
    bool isReady() const;
    bool isError() const;
    bool isLoading() const;

    QUrl url() const;

    Status status() const;
    QString error() const;

    qint64 size() const;
    const char *data() const;
    QByteArray dataByteArray() const;

    void load(QQmlEngine *, const QUrl &);
    void load(QQmlEngine *, const QString &);

    void clear();
    void clear(QObject *object);

#if QT_CONFIG(qml_network)
    bool connectFinished(QObject *, const char *);
    bool connectFinished(QObject *, int);
    bool connectDownloadProgress(QObject *, const char *);
    bool connectDownloadProgress(QObject *, int);
#endif

    static bool isSynchronous(const QString &url);
    static bool isSynchronous(const QUrl &url);

    static bool isLocalFile(const QString &url);
    static bool isLocalFile(const QUrl &url);

    static QString urlToLocalFileOrQrc(const QString &);
    static QString urlToLocalFileOrQrc(const QUrl &);
private:
    Q_DISABLE_COPY(QQmlFile)
    QQmlFilePrivate *d;
};

QT_END_NAMESPACE

#endif // QQMLFILE_H
