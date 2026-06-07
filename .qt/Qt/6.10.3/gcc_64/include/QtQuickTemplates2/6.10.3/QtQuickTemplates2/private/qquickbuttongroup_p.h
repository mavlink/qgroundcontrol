// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKBUTTONGROUP_P_H
#define QQUICKBUTTONGROUP_P_H

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

#include <QtCore/qobject.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlparserstatus.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractButton;
class QQuickButtonGroupPrivate;
class QQuickButtonGroupAttached;
class QQuickButtonGroupAttachedPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickButtonGroup : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(QQuickAbstractButton *checkedButton READ checkedButton WRITE setCheckedButton NOTIFY checkedButtonChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QQuickAbstractButton> buttons READ buttons NOTIFY buttonsChanged FINAL)
    // 2.3 (Qt 5.10)
    Q_PROPERTY(bool exclusive READ isExclusive WRITE setExclusive NOTIFY exclusiveChanged FINAL REVISION(2, 3))
    // 2.4 (Qt 5.11)
    Q_PROPERTY(Qt::CheckState checkState READ checkState WRITE setCheckState NOTIFY checkStateChanged FINAL REVISION(2, 4))
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(ButtonGroup)
    QML_ATTACHED(QQuickButtonGroupAttached)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickButtonGroup(QObject *parent = nullptr);
    ~QQuickButtonGroup();

    static QQuickButtonGroupAttached *qmlAttachedProperties(QObject *object);

    QQuickAbstractButton *checkedButton() const;
    void setCheckedButton(QQuickAbstractButton *checkedButton);

    QQmlListProperty<QQuickAbstractButton> buttons();

    bool isExclusive() const;
    void setExclusive(bool exclusive);

    // 2.4 (Qt 5.11)
    Qt::CheckState checkState() const;
    void setCheckState(Qt::CheckState state);

public Q_SLOTS:
    void addButton(QQuickAbstractButton *button);
    void removeButton(QQuickAbstractButton *button);

Q_SIGNALS:
    void checkedButtonChanged();
    void buttonsChanged();
    // 2.1 (Qt 5.8)
    Q_REVISION(2, 1) void clicked(QQuickAbstractButton *button);
    // 2.3 (Qt 5.10)
    Q_REVISION(2, 3) void exclusiveChanged();
    // 2.4 (Qt 5.11)
    Q_REVISION(2, 4) void checkStateChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

private:
    Q_DISABLE_COPY(QQuickButtonGroup)
    Q_DECLARE_PRIVATE(QQuickButtonGroup)

    Q_PRIVATE_SLOT(d_func(), void _q_updateCurrent())
};

class Q_QUICKTEMPLATES2_EXPORT QQuickButtonGroupAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickButtonGroup *group READ group WRITE setGroup NOTIFY groupChanged FINAL)

public:
    explicit QQuickButtonGroupAttached(QObject *parent = nullptr);

    QQuickButtonGroup *group() const;
    void setGroup(QQuickButtonGroup *group);

Q_SIGNALS:
    void groupChanged();

private:
    Q_DISABLE_COPY(QQuickButtonGroupAttached)
    Q_DECLARE_PRIVATE(QQuickButtonGroupAttached)
};

QT_END_NAMESPACE

#endif // QQUICKBUTTONGROUP_P_H
