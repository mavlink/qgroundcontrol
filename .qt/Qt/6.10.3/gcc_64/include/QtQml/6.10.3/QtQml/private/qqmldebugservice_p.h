// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDEBUGSERVICE_H
#define QQMLDEBUGSERVICE_H

#include <QtCore/qobject.h>
#include <QtCore/qhash.h>

#include <private/qtqmlglobal_p.h>

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

QT_REQUIRE_CONFIG(qml_debug);

QT_BEGIN_NAMESPACE

class QJSEngine;

class QQmlDebugServicePrivate;
class Q_QML_EXPORT QQmlDebugService : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlDebugService)

public:
    ~QQmlDebugService() override;

    const QString &name() const;
    float version() const;

    enum State { NotConnected, Unavailable, Enabled };
    State state() const;
    void setState(State newState);

    virtual void stateAboutToBeChanged(State) {}
    virtual void stateChanged(State) {}
    virtual void messageReceived(const QByteArray &) {}

    virtual void engineAboutToBeAdded(QJSEngine *engine) { Q_EMIT attachedToEngine(engine); }
    virtual void engineAboutToBeRemoved(QJSEngine *engine) { Q_EMIT detachedFromEngine(engine); }

    virtual void engineAdded(QJSEngine *) {}
    virtual void engineRemoved(QJSEngine *) {}

    static const QHash<int, QObject *> &objectsForIds();
    static int idForObject(QObject *);
    static QObject *objectForId(int id) { return objectsForIds().value(id); }

protected:
    explicit QQmlDebugService(const QString &, float version, QObject *parent = nullptr);

Q_SIGNALS:
    void attachedToEngine(QJSEngine *);
    void detachedFromEngine(QJSEngine *);

    void messageToClient(const QString &name, const QByteArray &message);
    void messagesToClient(const QString &name, const QList<QByteArray> &messages);
};

QT_END_NAMESPACE

#endif // QQMLDEBUGSERVICE_H

