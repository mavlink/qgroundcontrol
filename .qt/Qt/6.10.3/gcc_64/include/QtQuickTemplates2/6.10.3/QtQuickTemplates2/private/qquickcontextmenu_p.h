// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKCONTEXTMENU_P_H
#define QQUICKCONTEXTMENU_P_H

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
#include <QtQml/qqmlregistration.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>

QT_BEGIN_NAMESPACE

class QQuickContextMenuPrivate;
class QQuickMenu;

class Q_QUICKTEMPLATES2_EXPORT QQuickContextMenu : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QQuickMenu *menu READ menu WRITE setMenu NOTIFY menuChanged FINAL)
    Q_CLASSINFO("DeferredPropertyNames", "menu")
    QML_NAMED_ELEMENT(ContextMenu)
    QML_ATTACHED(QQuickContextMenu)
    QML_UNCREATABLE("")
    QML_ADDED_IN_VERSION(6, 9)

public:
    explicit QQuickContextMenu(QObject *parent = nullptr);

    static QQuickContextMenu *qmlAttachedProperties(QObject *object);

    QQuickMenu *menu() const;
    void setMenu(QQuickMenu *menu);

Q_SIGNALS:
    void menuChanged();
    void requested(QPointF position);

protected:
    bool event(QEvent *) override;

private:
    Q_DECLARE_PRIVATE(QQuickContextMenu)

    void classBegin() override;
    void componentComplete() override;
};

QT_END_NAMESPACE

#endif // QQUICKCONTEXTMENU_P_H
