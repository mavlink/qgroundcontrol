// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLSCRIPTSTRING_H
#define QQMLSCRIPTSTRING_H

#include <QtQml/qtqmlglobal.h>
#include <QtQml/qqml.h>
#include <QtCore/qstring.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE


class QObject;
class QQmlContext;
class QQmlScriptStringPrivate;
class QQmlObjectCreator;
namespace QV4 {
    struct QObjectWrapper;
}
class Q_QML_EXPORT QQmlScriptString
{
    Q_GADGET
public:
    QQmlScriptString();
    QQmlScriptString(const QQmlScriptString &);
    ~QQmlScriptString();

    QQmlScriptString &operator=(const QQmlScriptString &);

    bool operator==(const QQmlScriptString &) const;
    bool operator!=(const QQmlScriptString &) const;

    bool isEmpty() const;

    bool isUndefinedLiteral() const;
    bool isNullLiteral() const;
    QString stringLiteral() const;
    qreal numberLiteral(bool *ok) const;
    bool booleanLiteral(bool *ok) const;

private:
    QQmlScriptString(const QString &script, QQmlContext *context, QObject *scope);
    QSharedDataPointer<QQmlScriptStringPrivate> d;

    friend class QQmlObjectCreator;
    friend class QQmlScriptStringPrivate;
    friend class QQmlExpression;
    friend class QQmlBinding;
    friend class QQmlPropertyBinding;
    friend struct QV4::QObjectWrapper;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQmlScriptString)

#endif // QQMLSCRIPTSTRING_H

