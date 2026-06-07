// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSWITCH_P_H
#define QQUICKSWITCH_P_H

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

#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>

QT_BEGIN_NAMESPACE

class QQuickSwitchPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickSwitch : public QQuickAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(qreal visualPosition READ visualPosition NOTIFY visualPositionChanged FINAL)
    QML_NAMED_ELEMENT(Switch)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickSwitch(QQuickItem *parent = nullptr);

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

    void mirrorChange() override;

    void nextCheckState() override;
    void buttonChange(ButtonChange change) override;

    QFont defaultFont() const override;

private:
    Q_DISABLE_COPY(QQuickSwitch)
    Q_DECLARE_PRIVATE(QQuickSwitch)
};

QT_END_NAMESPACE

#endif // QQUICKSWITCH_P_H
