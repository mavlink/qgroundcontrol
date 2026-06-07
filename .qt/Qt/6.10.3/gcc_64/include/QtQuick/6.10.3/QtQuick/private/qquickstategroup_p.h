// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSTATEGROUP_H
#define QQUICKSTATEGROUP_H

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

QT_BEGIN_NAMESPACE

class QQuickStateGroupPrivate;
class Q_QUICK_EXPORT QQuickStateGroup : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_DECLARE_PRIVATE(QQuickStateGroup)

    Q_PROPERTY(QString state READ state WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(QQmlListProperty<QQuickState> states READ statesProperty DESIGNABLE false)
    Q_PROPERTY(QQmlListProperty<QQuickTransition> transitions READ transitionsProperty DESIGNABLE false)
    QML_NAMED_ELEMENT(StateGroup)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickStateGroup(QObject * = nullptr);
    virtual ~QQuickStateGroup();

    QString state() const;
    void setState(const QString &);

    QQmlListProperty<QQuickState> statesProperty();
    QList<QQuickState *> states() const;

    QQmlListProperty<QQuickTransition> transitionsProperty();

    QQuickState *findState(const QString &name) const;
    void removeState(QQuickState *state);

    void classBegin() override;
    void componentComplete() override;
Q_SIGNALS:
    void stateChanged(const QString &);

private:
    friend class QQuickState;
    friend class QQuickStatePrivate;
    bool updateAutoState();
    void stateAboutToComplete();
};

QT_END_NAMESPACE

#endif // QQUICKSTATEGROUP_H
