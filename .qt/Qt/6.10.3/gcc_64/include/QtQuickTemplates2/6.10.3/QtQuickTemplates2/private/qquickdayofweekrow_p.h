// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKDAYOFWEEKROW_P_H
#define QQUICKDAYOFWEEKROW_P_H

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

class QQmlComponent;
class QQuickDayOfWeekRowPrivate;

class QQuickDayOfWeekRow : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(QVariant source READ source WRITE setSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    QML_NAMED_ELEMENT(AbstractDayOfWeekRow)
    QML_ADDED_IN_VERSION(6, 3)

public:
    explicit QQuickDayOfWeekRow(QQuickItem *parent = nullptr);

    QVariant source() const;
    void setSource(const QVariant &source);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

Q_SIGNALS:
    void sourceChanged();
    void delegateChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void localeChange(const QLocale &newLocale, const QLocale &oldLocale) override;
    void paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding) override;

private:
    Q_DISABLE_COPY(QQuickDayOfWeekRow)
    Q_DECLARE_PRIVATE(QQuickDayOfWeekRow)
};

QT_END_NAMESPACE

#endif // QQUICKDAYOFWEEKROW_P_H
