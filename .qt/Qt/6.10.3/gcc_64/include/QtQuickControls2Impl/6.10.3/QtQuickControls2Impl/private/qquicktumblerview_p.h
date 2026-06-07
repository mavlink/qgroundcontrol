// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTUMBLERVIEW_P_H
#define QQUICKTUMBLERVIEW_P_H

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

#include <QQuickItem>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QQuickListView;
class QQuickPath;
class QQuickPathView;
class QQmlComponent;

class QQuickTumbler;

class Q_QUICKCONTROLS2IMPL_EXPORT QQuickTumblerView : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(QQuickPath *path READ path WRITE setPath NOTIFY pathChanged)
    QML_NAMED_ELEMENT(TumblerView)
    QML_ADDED_IN_VERSION(2, 1)

public:
    QQuickTumblerView(QQuickItem *parent = nullptr);

    QVariant model() const;
    void setModel(const QVariant &model);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    QQuickPath *path() const;
    void setPath(QQuickPath *path);

Q_SIGNALS:
    void modelChanged();
    void delegateChanged();
    void pathChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void componentComplete() override;
    void itemChange(ItemChange change, const ItemChangeData &data) override;

private:
    QQuickItem *view();
    void createView();
    void updateFlickDeceleration();
    void updateView();
    void updateModel();

    void wrapChange();

    QQuickTumbler *m_tumbler = nullptr;
    QVariant m_model;
    QQmlComponent *m_delegate = nullptr;
    QQuickPathView *m_pathView = nullptr;
    QQuickListView *m_listView = nullptr;
    QQuickPath *m_path = nullptr;
};

QT_END_NAMESPACE

#endif // TUMBLERVIEW_H
