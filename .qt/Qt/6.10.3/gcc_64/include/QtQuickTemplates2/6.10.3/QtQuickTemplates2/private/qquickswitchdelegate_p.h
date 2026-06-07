// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSWITCHDELEGATE_P_H
#define QQUICKSWITCHDELEGATE_P_H

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

#include <QtQuickTemplates2/private/qquickitemdelegate_p.h>

QT_BEGIN_NAMESPACE

class QQuickSwitchDelegatePrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickSwitchDelegate : public QQuickItemDelegate
{
    Q_OBJECT
    Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(qreal visualPosition READ visualPosition NOTIFY visualPositionChanged FINAL)
    QML_NAMED_ELEMENT(SwitchDelegate)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickSwitchDelegate(QQuickItem *parent = nullptr);

    qreal position() const;
    void setPosition(qreal position);

    qreal visualPosition() const;

Q_SIGNALS:
    void positionChanged();
    void visualPositionChanged();

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
#if QT_CONFIG(quicktemplates2_multitouch)
    void touchEvent(QTouchEvent *event) override;
#endif

    QFont defaultFont() const override;

    void mirrorChange() override;

    void nextCheckState() override;
    void buttonChange(ButtonChange change) override;

private:
    Q_DISABLE_COPY(QQuickSwitchDelegate)
    Q_DECLARE_PRIVATE(QQuickSwitchDelegate)
};

QT_END_NAMESPACE

#endif // QQUICKSWITCHDELEGATE_P_H
