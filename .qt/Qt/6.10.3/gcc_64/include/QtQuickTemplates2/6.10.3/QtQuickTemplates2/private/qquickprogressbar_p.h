// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPROGRESSBAR_P_H
#define QQUICKPROGRESSBAR_P_H

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

class QQuickProgressBarPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickProgressBar : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(qreal from READ from WRITE setFrom NOTIFY fromChanged FINAL)
    Q_PROPERTY(qreal to READ to WRITE setTo NOTIFY toChanged FINAL)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged FINAL)
    Q_PROPERTY(qreal visualPosition READ visualPosition NOTIFY visualPositionChanged FINAL)
    Q_PROPERTY(bool indeterminate READ isIndeterminate WRITE setIndeterminate NOTIFY indeterminateChanged FINAL)
    QML_NAMED_ELEMENT(ProgressBar)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickProgressBar(QQuickItem *parent = nullptr);

    qreal from() const;
    void setFrom(qreal from);

    qreal to() const;
    void setTo(qreal to);

    qreal value() const;
    void setValue(qreal value);

    qreal position() const;
    qreal visualPosition() const;

    bool isIndeterminate() const;
    void setIndeterminate(bool indeterminate);

Q_SIGNALS:
    void fromChanged();
    void toChanged();
    void valueChanged();
    void positionChanged();
    void visualPositionChanged();
    void indeterminateChanged();

protected:
    void mirrorChange() override;
    void componentComplete() override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickProgressBar)
    Q_DECLARE_PRIVATE(QQuickProgressBar)
};

QT_END_NAMESPACE

#endif // QQUICKPROGRESSBAR_P_H
