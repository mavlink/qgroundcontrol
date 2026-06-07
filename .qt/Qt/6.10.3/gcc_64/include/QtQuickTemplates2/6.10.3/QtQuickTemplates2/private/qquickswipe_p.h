// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSWIPE_P_H
#define QQUICKSWIPE_P_H

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
#include <QtQuickTemplates2/private/qquickswipedelegate_p.h>

QT_BEGIN_NAMESPACE

class QQmlComponent;
class QQuickItem;
class QQuickTransition;
class QQuickSwipePrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickSwipe : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(bool complete READ isComplete NOTIFY completeChanged FINAL)
    Q_PROPERTY(QQmlComponent *left READ left WRITE setLeft NOTIFY leftChanged FINAL)
    Q_PROPERTY(QQmlComponent *behind READ behind WRITE setBehind NOTIFY behindChanged FINAL)
    Q_PROPERTY(QQmlComponent *right READ right WRITE setRight NOTIFY rightChanged FINAL)
    Q_PROPERTY(QQuickItem *leftItem READ leftItem NOTIFY leftItemChanged FINAL)
    Q_PROPERTY(QQuickItem *behindItem READ behindItem NOTIFY behindItemChanged FINAL)
    Q_PROPERTY(QQuickItem *rightItem READ rightItem NOTIFY rightItemChanged FINAL)
    // 2.2 (Qt 5.9)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged FINAL) // REVISION(2, 2)
    Q_PROPERTY(QQuickTransition *transition READ transition WRITE setTransition NOTIFY transitionChanged FINAL) // REVISION(2, 2)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickSwipe(QQuickSwipeDelegate *control);

    qreal position() const;
    void setPosition(qreal position);

    bool isComplete() const;
    void setComplete(bool complete);

    QQmlComponent *left() const;
    void setLeft(QQmlComponent *left);

    QQmlComponent *behind() const;
    void setBehind(QQmlComponent *behind);

    QQmlComponent *right() const;
    void setRight(QQmlComponent *right);

    QQuickItem *leftItem() const;
    void setLeftItem(QQuickItem *item);

    QQuickItem *behindItem() const;
    void setBehindItem(QQuickItem *item);

    QQuickItem *rightItem() const;
    void setRightItem(QQuickItem *item);

    // 2.1 (Qt 5.8)
    Q_REVISION(2, 1) Q_INVOKABLE void close();

    // 2.2 (Qt 5.9)
    bool isEnabled() const;
    void setEnabled(bool enabled);

    QQuickTransition *transition() const;
    void setTransition(QQuickTransition *transition);

    Q_REVISION(2, 2) Q_INVOKABLE void open(QQuickSwipeDelegate::Side side);

Q_SIGNALS:
    void positionChanged();
    void completeChanged();
    void leftChanged();
    void behindChanged();
    void rightChanged();
    void leftItemChanged();
    void behindItemChanged();
    void rightItemChanged();
    // 2.1 (Qt 5.8)
    /*Q_REVISION(2, 1)*/ void completed();
    // 2.2 (Qt 5.9)
    /*Q_REVISION(2, 2)*/ void opened();
    /*Q_REVISION(2, 2)*/ void closed();
    /*Q_REVISION(2, 2)*/ void enabledChanged();
    /*Q_REVISION(2, 2)*/ void transitionChanged();

private:
    Q_DISABLE_COPY(QQuickSwipe)
    Q_DECLARE_PRIVATE(QQuickSwipe)
};

QT_END_NAMESPACE

#endif // QQUICKSWIPE_P_H
