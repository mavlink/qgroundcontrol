// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLENGINECONTROLCLIENT_P_H
#define QQMLENGINECONTROLCLIENT_P_H

#include "qqmldebugclient_p.h"

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

QT_BEGIN_NAMESPACE

class QQmlEngineControlClientPrivate;
class QQmlEngineControlClient : public QQmlDebugClient
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlEngineControlClient)
public:
    QQmlEngineControlClient(QQmlDebugConnection *connection);

    void blockEngine(int engineId);
    void releaseEngine(int engineId);

    QList<int> blockedEngines() const;

Q_SIGNALS:
    void engineAboutToBeAdded(int engineId, const QString &name);
    void engineAdded(int engineId, const QString &name);
    void engineAboutToBeRemoved(int engineId, const QString &name);
    void engineRemoved(int engineId, const QString &name);

protected:
    QQmlEngineControlClient(QQmlEngineControlClientPrivate &dd);

private:
    void messageReceived(const QByteArray &) override;
};

QT_END_NAMESPACE

#endif // QQMLENGINECONTROLCLIENT_P_H
