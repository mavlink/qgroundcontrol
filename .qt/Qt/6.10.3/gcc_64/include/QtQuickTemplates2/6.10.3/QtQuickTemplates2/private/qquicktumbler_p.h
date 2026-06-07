// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTUMBLER_P_H
#define QQUICKTUMBLER_P_H

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

#include <QtCore/qvariant.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQuickTumblerAttached;
class QQuickTumblerPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickTumbler : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL)
    Q_PROPERTY(QQuickItem *currentItem READ currentItem NOTIFY currentItemChanged FINAL)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PROPERTY(int visibleItemCount READ visibleItemCount WRITE setVisibleItemCount NOTIFY visibleItemCountChanged FINAL)
    // 2.1 (Qt 5.8)
    Q_PROPERTY(bool wrap READ wrap WRITE setWrap RESET resetWrap NOTIFY wrapChanged FINAL REVISION(2, 1))
    // 2.2 (Qt 5.9)
    Q_PROPERTY(bool moving READ isMoving NOTIFY movingChanged FINAL REVISION(2, 2))
    Q_PROPERTY(qreal flickDeceleration READ flickDeceleration WRITE setFlickDeceleration RESET resetFlickDeceleration NOTIFY flickDecelerationChanged FINAL REVISION(6, 9))
    QML_NAMED_ELEMENT(Tumbler)
    QML_ATTACHED(QQuickTumblerAttached)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickTumbler(QQuickItem *parent = nullptr);
    ~QQuickTumbler();

    QVariant model() const;
    void setModel(const QVariant &model);

    int count() const;

    int currentIndex() const;
    void setCurrentIndex(int currentIndex);
    QQuickItem *currentItem() const;

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    int visibleItemCount() const;
    void setVisibleItemCount(int visibleItemCount);

    static QQuickTumblerAttached *qmlAttachedProperties(QObject *object);

    // 2.1 (Qt 5.8)
    bool wrap() const;
    void setWrap(bool wrap);
    void resetWrap();

    // 2.2 (Qt 5.9)
    bool isMoving() const;

    enum PositionMode {
        Beginning,
        Center,
        End,
        Visible, // ListView-only
        Contain,
        SnapPosition
    };
    Q_ENUM(PositionMode)

    // 2.5 (Qt 5.12)
    Q_REVISION(2, 5) Q_INVOKABLE void positionViewAtIndex(int index, PositionMode mode);

    qreal flickDeceleration() const;
    void setFlickDeceleration(qreal newFlickDeceleration);
    void resetFlickDeceleration();

Q_SIGNALS:
    void modelChanged();
    void countChanged();
    void currentIndexChanged();
    void currentItemChanged();
    void delegateChanged();
    void visibleItemCountChanged();
    // 2.1 (Qt 5.8)
    Q_REVISION(2, 1) void wrapChanged();
    // 2.2 (Qt 5.9)
    Q_REVISION(2, 2) void movingChanged();
    Q_REVISION(6, 9) void flickDecelerationChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void componentComplete() override;
    void contentItemChange(QQuickItem *newItem, QQuickItem *oldItem) override;
    void keyPressEvent(QKeyEvent *event) override;
    void updatePolish() override;

    QFont defaultFont() const override;

private:
    Q_DISABLE_COPY(QQuickTumbler)
    Q_DECLARE_PRIVATE(QQuickTumbler)

    Q_PRIVATE_SLOT(d_func(), void _q_updateItemWidths())
    Q_PRIVATE_SLOT(d_func(), void _q_updateItemHeights())
    Q_PRIVATE_SLOT(d_func(), void _q_onViewCurrentIndexChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_onViewCountChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_onViewOffsetChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_onViewContentYChanged())
};

class QQuickTumblerAttachedPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickTumblerAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickTumbler *tumbler READ tumbler CONSTANT FINAL)
    Q_PROPERTY(qreal displacement READ displacement NOTIFY displacementChanged FINAL)

public:
    explicit QQuickTumblerAttached(QObject *parent = nullptr);

    QQuickTumbler *tumbler() const;
    qreal displacement() const;

Q_SIGNALS:
    void displacementChanged();

private:
    Q_DISABLE_COPY(QQuickTumblerAttached)
    Q_DECLARE_PRIVATE(QQuickTumblerAttached)
};

QT_END_NAMESPACE

#endif // QQUICKTUMBLER_P_H
