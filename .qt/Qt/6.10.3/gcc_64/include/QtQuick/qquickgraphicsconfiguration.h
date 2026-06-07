// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKGRAPHICSCONFIGURATION_H
#define QQUICKGRAPHICSCONFIGURATION_H

#include <QtQuick/qtquickglobal.h>
#include <QtCore/qbytearraylist.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

class QQuickGraphicsConfigurationPrivate;

class Q_QUICK_EXPORT QQuickGraphicsConfiguration
{
public:
    QQuickGraphicsConfiguration();
    ~QQuickGraphicsConfiguration();
    QQuickGraphicsConfiguration(const QQuickGraphicsConfiguration &other);
    QQuickGraphicsConfiguration &operator=(const QQuickGraphicsConfiguration &other);

    static QByteArrayList preferredInstanceExtensions();

    void setDeviceExtensions(const QByteArrayList &extensions);
    QByteArrayList deviceExtensions() const;

    void setDepthBufferFor2D(bool enable);
    bool isDepthBufferEnabledFor2D() const;

    void setDebugLayer(bool enable);
    bool isDebugLayerEnabled() const;

    void setDebugMarkers(bool enable);
    bool isDebugMarkersEnabled() const;

    void setTimestamps(bool enable);
    bool timestampsEnabled() const;

    void setPreferSoftwareDevice(bool enable);
    bool prefersSoftwareDevice() const;

    void setAutomaticPipelineCache(bool enable);
    bool isAutomaticPipelineCacheEnabled() const;

    void setPipelineCacheSaveFile(const QString &filename);
    QString pipelineCacheSaveFile() const;

    void setPipelineCacheLoadFile(const QString &filename);
    QString pipelineCacheLoadFile() const;

private:
    void detach();
    QQuickGraphicsConfigurationPrivate *d;
    friend class QQuickGraphicsConfigurationPrivate;
#ifndef QT_NO_DEBUG_STREAM
    friend Q_QUICK_EXPORT QDebug operator<<(QDebug dbg, const QQuickGraphicsConfiguration &config);
#endif
};


QT_END_NAMESPACE

#endif // QQUICKGRAPHICSCONFIGURATION_H
