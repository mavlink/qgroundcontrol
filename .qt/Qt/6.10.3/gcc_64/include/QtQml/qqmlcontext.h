// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLCONTEXT_H
#define QQMLCONTEXT_H

#include <QtCore/qurl.h>
#include <QtCore/qobject.h>
#include <QtCore/qlist.h>
#include <QtQml/qjsvalue.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE


class QString;
class QQmlEngine;
class QQmlContextPrivate;
class QQmlCompositeTypeData;
class QQmlContextData;

class Q_QML_EXPORT QQmlContext : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlContext)

public:
    struct PropertyPair { QString name; QVariant value; };

    QQmlContext(QQmlEngine *parent, QObject *objParent = nullptr);
    QQmlContext(QQmlContext *parent, QObject *objParent = nullptr);
    ~QQmlContext() override;

    bool isValid() const;

    QQmlEngine *engine() const;
    QQmlContext *parentContext() const;

    QObject *contextObject() const;
    void setContextObject(QObject *);

    QVariant contextProperty(const QString &) const;
    void setContextProperty(const QString &, QObject *);
    void setContextProperty(const QString &, const QVariant &);
    void setContextProperties(const QList<PropertyPair> &properties);

    QString nameForObject(const QObject *) const;
    QObject *objectForName(const QString &) const;

    QUrl resolvedUrl(const QUrl &) const;

    void setBaseUrl(const QUrl &);
    QUrl baseUrl() const;

    QJSValue importedScript(const QString &name) const;

private:
    friend class QQmlEngine;
    friend class QQmlEnginePrivate;
    friend class QQmlExpression;
    friend class QQmlExpressionPrivate;
    friend class QQmlComponent;
    friend class QQmlComponentPrivate;
    friend class QQmlScriptPrivate;
    friend class QQmlContextData;
    QQmlContext(QQmlContextPrivate &dd, QObject *parent = nullptr);
    QQmlContext(QQmlEngine *, bool);
    Q_DISABLE_COPY(QQmlContext)
};
QT_END_NAMESPACE

#endif // QQMLCONTEXT_H
