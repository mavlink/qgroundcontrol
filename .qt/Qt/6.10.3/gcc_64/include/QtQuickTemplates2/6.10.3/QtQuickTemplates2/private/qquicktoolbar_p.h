// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTOOLBAR_P_H
#define QQUICKTOOLBAR_P_H

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

#include <QtQuickTemplates2/private/qquickpane_p.h>

QT_BEGIN_NAMESPACE

class QQuickToolBarPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickToolBar : public QQuickPane
{
    Q_OBJECT
    Q_PROPERTY(Position position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    QML_NAMED_ELEMENT(ToolBar)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickToolBar(QQuickItem *parent = nullptr);

    enum Position {
        Header,
        Footer
    };
    Q_ENUM(Position)

    Position position() const;
    void setPosition(Position position);

Q_SIGNALS:
    void positionChanged();

protected:
    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickToolBar)
    Q_DECLARE_PRIVATE(QQuickToolBar)
};

QT_END_NAMESPACE

#endif // QQUICKTOOLBAR_P_H
