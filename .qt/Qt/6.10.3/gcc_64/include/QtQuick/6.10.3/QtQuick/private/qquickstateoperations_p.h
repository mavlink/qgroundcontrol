// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSTATEOPERATIONS_P_H
#define QQUICKSTATEOPERATIONS_P_H

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

#include "qquickitem.h"
#include "qquickanchors_p.h"

#include <QtQuick/private/qquickstate_p.h>

#include <QtQml/qqmlscriptstring.h>

QT_BEGIN_NAMESPACE

class QQuickParentChangePrivate;
class Q_QUICK_EXPORT QQuickParentChange : public QQuickStateOperation, public QQuickStateActionEvent
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickParentChange)

    Q_PROPERTY(QQuickItem *target READ object WRITE setObject)
    Q_PROPERTY(QQuickItem *parent READ parent WRITE setParent)
    Q_PROPERTY(QQmlScriptString x READ x WRITE setX)
    Q_PROPERTY(QQmlScriptString y READ y WRITE setY)
    Q_PROPERTY(QQmlScriptString width READ width WRITE setWidth)
    Q_PROPERTY(QQmlScriptString height READ height WRITE setHeight)
    Q_PROPERTY(QQmlScriptString scale READ scale WRITE setScale)
    Q_PROPERTY(QQmlScriptString rotation READ rotation WRITE setRotation)
    Q_CLASSINFO("ParentProperty", "parent")
    QML_NAMED_ELEMENT(ParentChange)
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickParentChange(QObject *parent=nullptr);

    QQuickItem *object() const;
    void setObject(QQuickItem *);

    QQuickItem *parent() const;
    void setParent(QQuickItem *);

    QQuickItem *originalParent() const;

    QQmlScriptString x() const;
    void setX(const QQmlScriptString &x);
    bool xIsSet() const;

    QQmlScriptString y() const;
    void setY(const QQmlScriptString &y);
    bool yIsSet() const;

    QQmlScriptString width() const;
    void setWidth(const QQmlScriptString &width);
    bool widthIsSet() const;

    QQmlScriptString height() const;
    void setHeight(const QQmlScriptString &height);
    bool heightIsSet() const;

    QQmlScriptString scale() const;
    void setScale(const QQmlScriptString &scale);
    bool scaleIsSet() const;

    QQmlScriptString rotation() const;
    void setRotation(const QQmlScriptString &rotation);
    bool rotationIsSet() const;

    ActionList actions() override;

    void saveOriginals() override;
    //virtual void copyOriginals(QQuickStateActionEvent*);
    void execute() override;
    bool isReversable() override;
    void reverse() override;
    EventType type() const override;
    bool mayOverride(QQuickStateActionEvent*other) override;
    void rewind() override;
    void saveCurrentValues() override;
};

class QQuickAnchorChanges;
class QQuickAnchorSetPrivate;
class Q_QUICK_EXPORT QQuickAnchorSet : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQmlScriptString left READ left WRITE setLeft RESET resetLeft FINAL)
    Q_PROPERTY(QQmlScriptString right READ right WRITE setRight RESET resetRight FINAL)
    Q_PROPERTY(QQmlScriptString horizontalCenter READ horizontalCenter WRITE setHorizontalCenter RESET resetHorizontalCenter FINAL)
    Q_PROPERTY(QQmlScriptString top READ top WRITE setTop RESET resetTop FINAL)
    Q_PROPERTY(QQmlScriptString bottom READ bottom WRITE setBottom RESET resetBottom FINAL)
    Q_PROPERTY(QQmlScriptString verticalCenter READ verticalCenter WRITE setVerticalCenter RESET resetVerticalCenter FINAL)
    Q_PROPERTY(QQmlScriptString baseline READ baseline WRITE setBaseline RESET resetBaseline FINAL)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickAnchorSet(QObject *parent=nullptr);
    virtual ~QQuickAnchorSet();

    QQmlScriptString left() const;
    void setLeft(const QQmlScriptString &edge);
    void resetLeft();

    QQmlScriptString right() const;
    void setRight(const QQmlScriptString &edge);
    void resetRight();

    QQmlScriptString horizontalCenter() const;
    void setHorizontalCenter(const QQmlScriptString &edge);
    void resetHorizontalCenter();

    QQmlScriptString top() const;
    void setTop(const QQmlScriptString &edge);
    void resetTop();

    QQmlScriptString bottom() const;
    void setBottom(const QQmlScriptString &edge);
    void resetBottom();

    QQmlScriptString verticalCenter() const;
    void setVerticalCenter(const QQmlScriptString &edge);
    void resetVerticalCenter();

    QQmlScriptString baseline() const;
    void setBaseline(const QQmlScriptString &edge);
    void resetBaseline();

    QQuickAnchors::Anchors usedAnchors() const;

private:
    friend class QQuickAnchorChanges;
    Q_DISABLE_COPY(QQuickAnchorSet)
    Q_DECLARE_PRIVATE(QQuickAnchorSet)
};

class QQuickAnchorChangesPrivate;
class Q_QUICK_EXPORT QQuickAnchorChanges : public QQuickStateOperation, public QQuickStateActionEvent
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickAnchorChanges)

    Q_PROPERTY(QQuickItem *target READ object WRITE setObject)
    Q_PROPERTY(QQuickAnchorSet *anchors READ anchors CONSTANT)
    QML_NAMED_ELEMENT(AnchorChanges)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickAnchorChanges(QObject *parent=nullptr);

    ActionList actions() override;

    QQuickAnchorSet *anchors() const;

    QQuickItem *object() const;
    void setObject(QQuickItem *);

    void execute() override;
    bool isReversable() override;
    void reverse() override;
    EventType type() const override;
    bool mayOverride(QQuickStateActionEvent*other) override;
    bool changesBindings() override;
    void saveOriginals() override;
    bool needsCopy() override { return true; }
    void copyOriginals(QQuickStateActionEvent*) override;
    void clearBindings() override;
    void rewind() override;
    void saveCurrentValues() override;

    QList<QQuickStateAction> additionalActions() const;
    void saveTargetValues() override;
};

QT_END_NAMESPACE

#endif // QQUICKSTATEOPERATIONS_P_H

