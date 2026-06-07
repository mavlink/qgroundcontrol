// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTOOLSEPARATOR_P_H
#define QQUICKTOOLSEPARATOR_P_H

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

#include <QtQuickTemplates2/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQuickToolSeparatorPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickToolSeparator : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged FINAL)
    Q_PROPERTY(bool horizontal READ isHorizontal NOTIFY orientationChanged FINAL)
    Q_PROPERTY(bool vertical READ isVertical NOTIFY orientationChanged FINAL)
    QML_NAMED_ELEMENT(ToolSeparator)
    QML_ADDED_IN_VERSION(2, 1)

public:
    explicit QQuickToolSeparator(QQuickItem *parent = nullptr);

    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);

    bool isHorizontal() const;
    bool isVertical() const;

Q_SIGNALS:
    void orientationChanged();

protected:
    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickToolSeparator)
    Q_DECLARE_PRIVATE(QQuickToolSeparator)
};

QT_END_NAMESPACE

#endif // QQUICKTOOLSEPARATOR_P_H
