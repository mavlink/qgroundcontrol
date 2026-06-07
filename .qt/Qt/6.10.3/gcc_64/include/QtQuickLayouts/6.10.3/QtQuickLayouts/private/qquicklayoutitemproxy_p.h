// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKLAYOUTITEMPROXY_P_H
#define QQUICKLAYOUTITEMPROXY_P_H

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

#include <private/qquickitem_p.h>
#include <private/qquickrectangle_p.h>

QT_BEGIN_NAMESPACE

class QQuickLayoutItemProxyAttachedData;
class QQuickLayoutItemProxyPrivate;
class QQuickLayoutItemProxy : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged)
    QML_NAMED_ELEMENT(LayoutItemProxy)
    QML_ADDED_IN_VERSION(6, 6)

public:
    QQuickLayoutItemProxy(QQuickItem *parent = nullptr);
    ~QQuickLayoutItemProxy() override;

    void geometryChange(const QRectF &newGeom, const QRectF &oldGeom) override;
    void itemChange(ItemChange c, const ItemChangeData &d) override;

    QQuickItem *target() const;
    void setTarget(QQuickItem *newTarget);
    Q_INVOKABLE QQuickItem *effectiveTarget() const;
    void clearTarget();

    void maybeTakeControl();
public slots:
    void updatePos();

private slots:
    // We define some slots to react to changes to the Layout attached properties.
    // They are all named following the same scheme, which allows us to use a macro.
#define propertyForwarding(Property) \
    void target##Property##Changed(); \
    void proxy##Property##Changed();

    propertyForwarding(MinimumWidth)
    propertyForwarding(MinimumHeight)
    propertyForwarding(PreferredWidth)
    propertyForwarding(PreferredHeight)
    propertyForwarding(MaximumWidth)
    propertyForwarding(MaximumHeight)
    propertyForwarding(FillWidth)
    propertyForwarding(FillHeight)
    propertyForwarding(Alignment)
    propertyForwarding(HorizontalStretchFactor)
    propertyForwarding(VerticalStretchFactor)
    propertyForwarding(Margins)
    propertyForwarding(LeftMargin)
    propertyForwarding(TopMargin)
    propertyForwarding(RightMargin)
    propertyForwarding(BottomMargin)
#undef propertyForwarding

signals:
    void targetChanged();

private:
    Q_DECLARE_PRIVATE(QQuickLayoutItemProxy)
};

class QQuickLayoutItemProxyPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickLayoutItemProxy)

public:
    QQuickLayoutItemProxyPrivate();

    // the target of the LayoutItem
    QQuickItem *target = nullptr;

    // These values are required to know why the Layout property of the proxy changed
    // If it changed because the target changed we should keep the connection valid
    // If a Layout property change is not invoked by the target, it was set
    // explicitly by the application developer and we should disconnect the connection
    // between target and proxy for this property.
    unsigned m_expectProxyMinimumWidthChange : 1;
    unsigned m_expectProxyMinimumHeightChange : 1;
    unsigned m_expectProxyPreferredWidthChange : 1;
    unsigned m_expectProxyPreferredHeightChange : 1;
    unsigned m_expectProxyMaximumWidthChange : 1;
    unsigned m_expectProxyMaximumHeightChange : 1;
    unsigned m_expectProxyFillWidthChange : 1;
    unsigned m_expectProxyFillHeightChange : 1;
    unsigned m_expectProxyAlignmentChange : 1;
    unsigned m_expectProxyHorizontalStretchFactorChange : 1;
    unsigned m_expectProxyVerticalStretchFactorChange : 1;
    unsigned m_expectProxyMarginsChange : 1;
    unsigned m_expectProxyLeftMarginChange : 1;
    unsigned m_expectProxyTopMarginChange : 1;
    unsigned m_expectProxyRightMarginChange : 1;
    unsigned m_expectProxyBottomMarginChange : 1;

    friend class QQuickLayoutItemProxy;
};

class QQuickLayoutItemProxyAttachedData : public QObject
{
    Q_OBJECT

    QML_ANONYMOUS
    Q_PROPERTY(bool proxyHasControl READ proxyHasControl NOTIFY controllingProxyChanged)
    Q_PROPERTY(QQuickLayoutItemProxy* controllingProxy READ getControllingProxy NOTIFY controllingProxyChanged)
    Q_PROPERTY(QQmlListProperty<QQuickLayoutItemProxy> proxies READ getProxies NOTIFY proxiesChanged)

public:
    QQuickLayoutItemProxyAttachedData(QObject *parent);
    ~QQuickLayoutItemProxyAttachedData() override;
    void registerProxy(QQuickLayoutItemProxy *proxy);
    void releaseProxy(QQuickLayoutItemProxy *proxy);
    bool takeControl(QQuickLayoutItemProxy *proxy);
    void releaseControl(QQuickLayoutItemProxy *proxy);
    QQuickLayoutItemProxy *getControllingProxy() const;
    QQmlListProperty<QQuickLayoutItemProxy> getProxies();
    bool proxyHasControl() const;

signals:
    void controlTaken();
    void controlReleased();
    void controllingProxyChanged();
    void proxiesChanged();

private:
    QList<QQuickLayoutItemProxy*> proxies;
    QQuickLayoutItemProxy* controllingProxy;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQuickLayoutItemProxyAttachedData*);

#endif // QQUICKLAYOUTITEMPROXY_P_H
