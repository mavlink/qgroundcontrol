// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLOADER_P_H
#define QQUICKLOADER_P_H

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

#include "qquickimplicitsizeitem_p.h"

QT_BEGIN_NAMESPACE

class QQuickLoaderPrivate;
class QQmlComponent;

class Q_QUICK_EXPORT QQuickLoader : public QQuickImplicitSizeItem
{
    Q_OBJECT

    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSourceWithoutResolve NOTIFY sourceChanged)
    Q_PROPERTY(QQmlComponent *sourceComponent READ sourceComponent WRITE setSourceComponent RESET resetSourceComponent NOTIFY sourceComponentChanged)
    Q_PROPERTY(QObject *item READ item NOTIFY itemChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool asynchronous READ asynchronous WRITE setAsynchronous NOTIFY asynchronousChanged)
    QML_NAMED_ELEMENT(Loader)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickLoader(QQuickItem *parent = nullptr);
    virtual ~QQuickLoader();

    bool active() const;
    void setActive(bool newVal);

    Q_INVOKABLE void setSource(const QUrl &source, QJSValue initialProperties);
    Q_INVOKABLE void setSource(const QUrl &source);

    QUrl source() const;
    void setSourceWithoutResolve(const QUrl &source);

    QQmlComponent *sourceComponent() const;
    void setSourceComponent(QQmlComponent *);
    void resetSourceComponent();

    enum Status { Null, Ready, Loading, Error };
    Q_ENUM(Status)
    Status status() const;
    qreal progress() const;

    bool asynchronous() const;
    void setAsynchronous(bool a);

    QObject *item() const;

Q_SIGNALS:
    void itemChanged();
    void activeChanged();
    void sourceChanged();
    void sourceComponentChanged();
    void statusChanged();
    void progressChanged();
    void loaded();
    void asynchronousChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void componentComplete() override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

private:
    QUrl setSourceUrlHelper(const QUrl &unresolvedUrl);
    void setSource(const QUrl &sourceUrl, bool needsClear);
    void loadFromSource();
    void loadFromSourceComponent();
    Q_DISABLE_COPY(QQuickLoader)
    Q_DECLARE_PRIVATE(QQuickLoader)
    Q_PRIVATE_SLOT(d_func(), void _q_sourceLoaded())
    Q_PRIVATE_SLOT(d_func(), void _q_updateSize())
};

QT_END_NAMESPACE

#endif // QQUICKLOADER_P_H
