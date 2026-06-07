// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKROUNDBUTTON_P_H
#define QQUICKROUNDBUTTON_P_H

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

#include <QtQuickTemplates2/private/qquickbutton_p.h>

QT_BEGIN_NAMESPACE

class QQuickRoundButtonPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickRoundButton : public QQuickButton
{
    Q_OBJECT
    Q_PROPERTY(qreal radius READ radius WRITE setRadius RESET resetRadius NOTIFY radiusChanged FINAL)
    QML_NAMED_ELEMENT(RoundButton)
    QML_ADDED_IN_VERSION(2, 1)

public:
    explicit QQuickRoundButton(QQuickItem *parent = nullptr);

    qreal radius() const;
    void setRadius(qreal radius);
    void resetRadius();

Q_SIGNALS:
    void radiusChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    Q_DISABLE_COPY(QQuickRoundButton)
    Q_DECLARE_PRIVATE(QQuickRoundButton)
};

QT_END_NAMESPACE

#endif // QQUICKROUNDBUTTON_P_H
