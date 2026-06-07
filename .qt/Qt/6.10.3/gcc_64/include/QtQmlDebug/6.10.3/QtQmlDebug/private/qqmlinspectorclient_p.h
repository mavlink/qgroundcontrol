// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLINSPECTORCLIENT_P_H
#define QQMLINSPECTORCLIENT_P_H

#include <private/qqmldebugclient_p.h>

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

class QQmlInspectorClientPrivate;
class QQmlInspectorClient : public QQmlDebugClient
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlInspectorClient)

public:
    QQmlInspectorClient(QQmlDebugConnection *connection);

    int setInspectToolEnabled(bool enabled);
    int setShowAppOnTop(bool showOnTop);
    int setAnimationSpeed(qreal speed);
    int select(const QList<int> &objectIds);
    int createObject(const QString &qml, int parentId, const QStringList &imports,
                     const QString &filename);
    int moveObject(int childId, int newParentId);
    int destroyObject(int objectId);

Q_SIGNALS:
    void responseReceived(int requestId, bool result);

protected:
    void messageReceived(const QByteArray &message) override;
};

QT_END_NAMESPACE

#endif // QQMLINSPECTORCLIENT_P_H
