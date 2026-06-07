// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLENGINEDEBUGCLIENT_H
#define QQMLENGINEDEBUGCLIENT_H

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

#include <private/qqmldebugclient_p.h>
#include <private/qpacket_p.h>

#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

struct QQmlEngineDebugPropertyReference
{
    qint32 objectDebugId = -1;
    QString name;
    QVariant value;
    QString valueTypeName;
    QString binding;
    bool hasNotifySignal = false;
};

struct QQmlEngineDebugFileReference
{
    QUrl url;
    qint32 lineNumber = -1;
    qint32 columnNumber = -1;
};

struct QQmlEngineDebugObjectReference
{
    qint32 debugId = -1;
    QString className;
    QString idString;
    QString name;
    QQmlEngineDebugFileReference source;
    qint32 contextDebugId = -1;
    QList<QQmlEngineDebugPropertyReference> properties;
    QList<QQmlEngineDebugObjectReference> children;
};

struct QQmlEngineDebugContextReference
{
    qint32 debugId = -1;
    QString name;
    QList<QQmlEngineDebugObjectReference> objects;
    QList<QQmlEngineDebugContextReference> contexts;
};

struct QQmlEngineDebugEngineReference
{
    qint32 debugId = -1;
    QString name;
};

class QQmlEngineDebugClientPrivate;
class QQmlEngineDebugClient : public QQmlDebugClient
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlEngineDebugClient)

public:
    explicit QQmlEngineDebugClient(QQmlDebugConnection *conn);

    qint32 addWatch(const QQmlEngineDebugPropertyReference &,
                    bool *success);
    qint32 addWatch(const QQmlEngineDebugContextReference &, const QString &,
                    bool *success);
    qint32 addWatch(const QQmlEngineDebugObjectReference &, const QString &,
                    bool *success);
    qint32 addWatch(const QQmlEngineDebugObjectReference &,
                    bool *success);
    qint32 addWatch(const QQmlEngineDebugFileReference &,
                     bool *success);

    void removeWatch(qint32 watch, bool *success);

    qint32 queryAvailableEngines(bool *success);
    qint32 queryRootContexts(const QQmlEngineDebugEngineReference &,
                             bool *success);
    qint32 queryObject(const QQmlEngineDebugObjectReference &,
                       bool *success);
    qint32 queryObjectsForLocation(const QString &file,
           qint32 lineNumber, qint32 columnNumber, bool *success);
    qint32 queryObjectRecursive(const QQmlEngineDebugObjectReference &,
                                bool *success);
    qint32 queryObjectsForLocationRecursive(const QString &file,
           qint32 lineNumber, qint32 columnNumber, bool *success);
    qint32 queryExpressionResult(qint32 objectDebugId,
                                 const QString &expr,
                                 bool *success);
    qint32 queryExpressionResultBC(qint32 objectDebugId,
                                 const QString &expr,
                                 bool *success);
    qint32 setBindingForObject(qint32 objectDebugId, const QString &propertyName,
                               const QVariant &bindingExpression,
                               bool isLiteralValue,
                               const QString &source, qint32 line, bool *success);
    qint32 resetBindingForObject(qint32 objectDebugId,
                                 const QString &propertyName, bool *success);
    qint32 setMethodBody(qint32 objectDebugId, const QString &methodName,
                         const QString &methodBody, bool *success);

    qint32 getId();

    void decode(QPacket &ds, QQmlEngineDebugContextReference &);
    void decode(QPacket &ds, QQmlEngineDebugObjectReference &, bool simple);
    void decode(QPacket &ds, QList<QQmlEngineDebugObjectReference> &o, bool simple);

    QList<QQmlEngineDebugEngineReference> engines() const;
    QQmlEngineDebugContextReference rootContext() const;
    QQmlEngineDebugObjectReference object() const;
    QList<QQmlEngineDebugObjectReference> objects() const;
    QVariant resultExpr() const;
    bool valid() const;

Q_SIGNALS:
    void newObject(qint32 objectId);
    void valueChanged(QByteArray,QVariant);
    void result();

protected:
    void messageReceived(const QByteArray &) override;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQmlEngineDebugObjectReference)

#endif // QQMLENGINEDEBUGCLIENT_H
