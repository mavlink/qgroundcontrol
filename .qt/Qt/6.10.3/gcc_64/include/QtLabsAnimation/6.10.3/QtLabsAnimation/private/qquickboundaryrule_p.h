// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKBOUNDARYRULE_H
#define QQUICKBOUNDARYRULE_H

#include "qqmlanimationglobal_p.h"

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

#include <private/qtquickglobal_p.h>

#include <private/qqmlpropertyvalueinterceptor_p.h>
#include <qqml.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractAnimation;
class QQuickBoundaryRulePrivate;
class Q_LABSANIMATION_EXPORT QQuickBoundaryRule : public QObject, public QQmlPropertyValueInterceptor, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_DECLARE_PRIVATE(QQuickBoundaryRule)

    Q_INTERFACES(QQmlPropertyValueInterceptor)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(qreal minimum READ minimum WRITE setMinimum NOTIFY minimumChanged FINAL)
    Q_PROPERTY(qreal minimumOvershoot READ minimumOvershoot WRITE setMinimumOvershoot NOTIFY minimumOvershootChanged FINAL)
    Q_PROPERTY(qreal maximum READ maximum WRITE setMaximum NOTIFY maximumChanged FINAL)
    Q_PROPERTY(qreal maximumOvershoot READ maximumOvershoot WRITE setMaximumOvershoot NOTIFY maximumOvershootChanged FINAL)
    Q_PROPERTY(qreal overshootScale READ overshootScale WRITE setOvershootScale NOTIFY overshootScaleChanged FINAL)
    Q_PROPERTY(qreal currentOvershoot READ currentOvershoot NOTIFY currentOvershootChanged FINAL)
    Q_PROPERTY(qreal peakOvershoot READ peakOvershoot NOTIFY peakOvershootChanged FINAL)
    Q_PROPERTY(OvershootFilter overshootFilter READ overshootFilter WRITE setOvershootFilter NOTIFY overshootFilterChanged FINAL)
    Q_PROPERTY(QEasingCurve easing READ easing WRITE setEasing NOTIFY easingChanged FINAL)
    Q_PROPERTY(int returnDuration READ returnDuration WRITE setReturnDuration NOTIFY returnDurationChanged FINAL)
    QML_NAMED_ELEMENT(BoundaryRule)
    QML_ADDED_IN_VERSION(1, 0)

public:
    enum OvershootFilter {
        None,
        Peak
    };
    Q_ENUM(OvershootFilter)

    QQuickBoundaryRule(QObject *parent=nullptr);
    ~QQuickBoundaryRule();

    void setTarget(const QQmlProperty &) override;
    void write(const QVariant &value) override;

    bool enabled() const;
    void setEnabled(bool enabled);

    qreal minimum() const;
    void setMinimum(qreal minimum);
    qreal minimumOvershoot() const;
    void setMinimumOvershoot(qreal minimum);

    qreal maximum() const;
    void setMaximum(qreal maximum);
    qreal maximumOvershoot() const;
    void setMaximumOvershoot(qreal maximum);

    qreal overshootScale() const;
    void setOvershootScale(qreal scale);

    qreal currentOvershoot() const;
    qreal peakOvershoot() const;

    OvershootFilter overshootFilter() const;
    void setOvershootFilter(OvershootFilter overshootFilter);

    Q_INVOKABLE bool returnToBounds();

    QEasingCurve easing() const;
    void setEasing(const QEasingCurve &easing);

    int returnDuration() const;
    void setReturnDuration(int duration);

    // QQmlParserStatus interface
    void classBegin() override;
    void componentComplete() override;

Q_SIGNALS:
    void enabledChanged();
    void minimumChanged();
    void minimumOvershootChanged();
    void maximumChanged();
    void maximumOvershootChanged();
    void overshootScaleChanged();
    void currentOvershootChanged();
    void peakOvershootChanged();
    void overshootFilterChanged();
    void easingChanged();
    void returnDurationChanged();
    void returnedToBounds();
};

QT_END_NAMESPACE

#endif // QQUICKBOUNDARYRULE_H
