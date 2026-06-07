// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMONTHGRID_P_H
#define QQUICKMONTHGRID_P_H

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
class QQuickMonthGridPrivate;

class QQuickMonthGrid : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(int month READ month WRITE setMonth NOTIFY monthChanged FINAL)
    Q_PROPERTY(int year READ year WRITE setYear NOTIFY yearChanged FINAL)
    Q_PROPERTY(QVariant source READ source WRITE setSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    QML_NAMED_ELEMENT(AbstractMonthGrid)
    QML_ADDED_IN_VERSION(6, 3)

public:
    explicit QQuickMonthGrid(QQuickItem *parent = nullptr);

    int month() const;
    void setMonth(int month);

    int year() const;
    void setYear(int year);

    QVariant source() const;
    void setSource(const QVariant &source);

    QString title() const;
    void setTitle(const QString &title);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

Q_SIGNALS:
    void monthChanged();
    void yearChanged();
    void sourceChanged();
    void titleChanged();
    void delegateChanged();

    void pressed(QDateTime date);
    void released(QDateTime date);
    void clicked(QDateTime date);
    void pressAndHold(QDateTime date);

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void localeChange(const QLocale &newLocale, const QLocale &oldLocale) override;
    void paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding) override;
    void updatePolish() override;

    void timerEvent(QTimerEvent *event) override;

private:
    Q_DISABLE_COPY(QQuickMonthGrid)
    Q_DECLARE_PRIVATE(QQuickMonthGrid)
};

QT_END_NAMESPACE

#endif // QQUICKMONTHGRID_P_H
