// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLERROR_H
#define QQMLERROR_H

#include <QtQml/qtqmlglobal.h>

#include <QtCore/qurl.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

// ### Qt 7: should this be called QQmlMessage, since it can have a message type?
class QDebug;
class QQmlErrorPrivate;
class Q_QML_EXPORT QQmlError
{
public:
    QQmlError();
    QQmlError(const QQmlError &);
    QQmlError(QQmlError &&other) noexcept
        : d(std::exchange(other.d, nullptr))
    {}

    QQmlError &operator=(const QQmlError &);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QQmlError)
    ~QQmlError();

    void swap(QQmlError &other)
    { qt_ptr_swap(d, other.d); }

    bool isValid() const;

    QUrl url() const;
    void setUrl(const QUrl &);
    QString description() const;
    void setDescription(const QString &);
    int line() const;
    void setLine(int);
    int column() const;
    void setColumn(int);

    QObject *object() const;
    void setObject(QObject *);

    QtMsgType messageType() const;
    void setMessageType(QtMsgType messageType);

    QString toString() const;
    friend bool Q_QML_EXPORT operator==(const QQmlError &a, const QQmlError &b);
private:
    QQmlErrorPrivate *d;
};

QDebug Q_QML_EXPORT operator<<(QDebug debug, const QQmlError &error);

Q_DECLARE_TYPEINFO(QQmlError, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif // QQMLERROR_H
