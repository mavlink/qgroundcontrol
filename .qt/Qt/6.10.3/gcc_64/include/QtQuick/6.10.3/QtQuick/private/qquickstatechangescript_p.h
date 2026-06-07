// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSTATEOPERATIONS_H
#define QQUICKSTATEOPERATIONS_H

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

#include "qquickstate_p.h"
#include <qqmlscriptstring.h>

QT_BEGIN_NAMESPACE

class QQuickStateChangeScriptPrivate;
class Q_QUICK_EXPORT QQuickStateChangeScript : public QQuickStateOperation, public QQuickStateActionEvent
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickStateChangeScript)

    Q_PROPERTY(QQmlScriptString script READ script WRITE setScript)
    Q_PROPERTY(QString name READ name WRITE setName)
    QML_NAMED_ELEMENT(StateChangeScript)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickStateChangeScript(QObject *parent=nullptr);

    ActionList actions() override;

    EventType type() const override;

    QQmlScriptString script() const;
    void setScript(const QQmlScriptString &);

    QString name() const;
    void setName(const QString &);

    void execute() override;
};


QT_END_NAMESPACE

#endif // QQUICKSTATEOPERATIONS_H
