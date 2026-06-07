// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSCROLLINDICATOR_P_H
#define QQUICKSCROLLINDICATOR_P_H

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

class QQuickFlickable;
class QQuickScrollIndicatorAttached;
class QQuickScrollIndicatorPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickScrollIndicator : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(qreal size READ size WRITE setSize NOTIFY sizeChanged FINAL)
    Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged FINAL)
    // 2.3 (Qt 5.10)
    Q_PROPERTY(bool horizontal READ isHorizontal NOTIFY orientationChanged FINAL REVISION(2, 3))
    Q_PROPERTY(bool vertical READ isVertical NOTIFY orientationChanged FINAL REVISION(2, 3))
    // 2.4 (Qt 5.11)
    Q_PROPERTY(qreal minimumSize READ minimumSize WRITE setMinimumSize NOTIFY minimumSizeChanged FINAL REVISION(2, 4))
    Q_PROPERTY(qreal visualSize READ visualSize NOTIFY visualSizeChanged FINAL REVISION(2, 4))
    Q_PROPERTY(qreal visualPosition READ visualPosition NOTIFY visualPositionChanged FINAL REVISION(2, 4))
    QML_NAMED_ELEMENT(ScrollIndicator)
    QML_ATTACHED(QQuickScrollIndicatorAttached)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickScrollIndicator(QQuickItem *parent = nullptr);

    static QQuickScrollIndicatorAttached *qmlAttachedProperties(QObject *object);

    qreal size() const;
    qreal position() const;

    bool isActive() const;
    void setActive(bool active);

    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);

    // 2.3 (Qt 5.10)
    bool isHorizontal() const;
    bool isVertical() const;

    // 2.4 (Qt 5.11)
    qreal minimumSize() const;
    void setMinimumSize(qreal minimumSize);

    qreal visualSize() const;
    qreal visualPosition() const;

public Q_SLOTS:
    void setSize(qreal size);
    void setPosition(qreal position);

Q_SIGNALS:
    void sizeChanged();
    void positionChanged();
    void activeChanged();
    void orientationChanged();
    // 2.4 (Qt 5.11)
    Q_REVISION(2, 4) void minimumSizeChanged();
    Q_REVISION(2, 4) void visualSizeChanged();
    Q_REVISION(2, 4) void visualPositionChanged();

protected:
#if QT_CONFIG(quicktemplates2_multitouch)
    void touchEvent(QTouchEvent *event) override;
#endif

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickScrollIndicator)
    Q_DECLARE_PRIVATE(QQuickScrollIndicator)
};

class QQuickScrollIndicatorAttachedPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickScrollIndicatorAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickScrollIndicator *horizontal READ horizontal WRITE setHorizontal NOTIFY horizontalChanged FINAL)
    Q_PROPERTY(QQuickScrollIndicator *vertical READ vertical WRITE setVertical NOTIFY verticalChanged FINAL)

public:
    explicit QQuickScrollIndicatorAttached(QObject *parent = nullptr);
    ~QQuickScrollIndicatorAttached();

    QQuickScrollIndicator *horizontal() const;
    void setHorizontal(QQuickScrollIndicator *horizontal);

    QQuickScrollIndicator *vertical() const;
    void setVertical(QQuickScrollIndicator *vertical);

Q_SIGNALS:
    void horizontalChanged();
    void verticalChanged();

private:
    Q_DISABLE_COPY(QQuickScrollIndicatorAttached)
    Q_DECLARE_PRIVATE(QQuickScrollIndicatorAttached)
};

QT_END_NAMESPACE

#endif // QQUICKSCROLLINDICATOR_P_H
