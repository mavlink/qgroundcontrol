// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKICONLABEL_P_P_H
#define QQUICKICONLABEL_P_P_H

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

#include <QtQuick/private/qquickitem_p.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>
#include <QtQuickControls2Impl/private/qquickiconlabel_p.h>

QT_BEGIN_NAMESPACE

class QQuickIconImage;
class QQuickMnemonicLabel;

class Q_AUTOTEST_EXPORT QQuickIconLabelPrivate : public QQuickItemPrivate,
                                                 public QSafeQuickItemChangeListener<QQuickIconLabelPrivate>
{
    Q_DECLARE_PUBLIC(QQuickIconLabel)

public:
    ~QQuickIconLabelPrivate() override;

    bool hasIcon() const;
    bool hasText() const;

    bool createImage();
    bool destroyImage();
    bool updateImage();
    void syncImage();
    void updateOrSyncImage();

    bool createLabel();
    bool destroyLabel();
    bool updateLabel();
    void syncLabel();
    void updateOrSyncLabel();

    void updateImplicitSize();
    void layout();

    void watchChanges(QQuickItem *item);
    void unwatchChanges(QQuickItem *item);
    void setPositioningDirty();

    bool isLeftToRight() const;

    void itemImplicitWidthChanged(QQuickItem *) override;
    void itemImplicitHeightChanged(QQuickItem *) override;
    void itemDestroyed(QQuickItem *item) override;

    bool mirrored = false;
    QQuickIconLabel::Display display = QQuickIconLabel::TextBesideIcon;
    Qt::Alignment alignment = Qt::AlignCenter;
    qreal spacing = 0;
    qreal topPadding = 0;
    qreal leftPadding = 0;
    qreal rightPadding = 0;
    qreal bottomPadding = 0;
    QFont font;
    QColor color;
    QString text;
    QQuickIcon icon;
    QQuickIconImage *image = nullptr;
    QQuickMnemonicLabel *label = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKICONLABEL_P_P_H
