// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSAFEAREA_P_H
#define QQUICKSAFEAREA_P_H

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

#include <QtQml/qqml.h>

#include <QtQuick/private/qtquickglobal_p.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>

QT_BEGIN_NAMESPACE

class QQuickAnchorLine;
class QQuickItem;
class QQuickControl;

class Q_QUICK_EXPORT QQuickSafeArea : public QObject, public QQuickItemChangeListener
{
    Q_OBJECT

    Q_PROPERTY(QMarginsF margins READ margins NOTIFY marginsChanged FINAL)
    Q_PROPERTY(QMarginsF additionalMargins READ additionalMargins WRITE setAdditionalMargins NOTIFY additionalMarginsChanged FINAL)

    QML_NAMED_ELEMENT(SafeArea)
    QML_ADDED_IN_VERSION(6, 9)
    QML_ATTACHED(QQuickSafeArea)
    QML_UNCREATABLE("SafeArea can only be used via the attached property.")

public:
    QQuickSafeArea(QQuickItem *attachee = nullptr);
    ~QQuickSafeArea();

    static QQuickSafeArea *qmlAttachedProperties(QObject *attachee);

    QMarginsF margins() const;

    QMarginsF additionalMargins() const;
    void setAdditionalMargins(const QMarginsF &additionalMargins);

    static void updateSafeAreasRecursively(QQuickItem *fromItem);

Q_SIGNALS:
    void marginsChanged();
    void additionalMarginsChanged();

private:
    void windowChanged();
    void updateSafeArea();

    void itemTransformChanged(QQuickItem *, QQuickItem *) override;
    void itemGeometryChanged(QQuickItem *, QQuickGeometryChange, const QRectF &) override;

#ifndef QT_NO_DEBUG_STREAM
    friend Q_QUICK_EXPORT QDebug operator<<(QDebug debug, const QQuickSafeArea *safeArea);
#endif

    QMarginsF m_safeAreaMargins;
    QMarginsF m_additionalMargins;
    bool emittingMarginsUpdate = false;
    bool detectedPossibleBindingLoop = false;

    void addSourceItem(QQuickItem *item) override;
    void removeSourceItem(QQuickItem *item) override;
    QList<QPointer<QQuickItem>> m_listenedItems;

    friend class QQuickItem;
    friend class QQuickControl;
};

class Q_QUICK_EXPORT QQuickSafeAreaAttachable
{
public:
    virtual ~QQuickSafeAreaAttachable();
    virtual QQuickItem *safeAreaAttachmentItem() = 0;

private:
    friend class QQuickSafeArea;
};

#define QQuickSafeAreaAttachable_iid "org.qt-project.Qt.QQuickSafeAreaAttachable"
Q_DECLARE_INTERFACE(QQuickSafeAreaAttachable, QQuickSafeAreaAttachable_iid)

QT_END_NAMESPACE

#endif // QQUICKSAFEAREA_P_H
