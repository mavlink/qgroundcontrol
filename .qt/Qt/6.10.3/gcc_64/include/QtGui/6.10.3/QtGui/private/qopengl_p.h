// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOPENGL_P_H
#define QOPENGL_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <qopengl.h>
#include <private/qopenglcontext_p.h>
#include <QtCore/qset.h>
#include <QtCore/qstring.h>
#include <QtCore/qversionnumber.h>

QT_BEGIN_NAMESPACE

class QJsonDocument;

class Q_GUI_EXPORT QOpenGLExtensionMatcher
{
public:
    QOpenGLExtensionMatcher();

    bool match(const QByteArray &extension) const
    {
        return m_extensions.contains(extension);
    }

    QSet<QByteArray> extensions() const { return m_extensions; }

private:
    QSet<QByteArray> m_extensions;
};

class Q_GUI_EXPORT QOpenGLConfig
{
public:
    struct Q_GUI_EXPORT Gpu {
        Gpu() : vendorId(0), deviceId(0) {}
        bool isValid() const { return deviceId || !glVendor.isEmpty(); }
        bool equals(const Gpu &other) const {
            return vendorId == other.vendorId && deviceId == other.deviceId && driverVersion == other.driverVersion
                && driverDescription == other.driverDescription && glVendor == other.glVendor;
        }

        uint vendorId;
        uint deviceId;
        QVersionNumber driverVersion;
        QByteArray driverDescription;
        QByteArray glVendor;

        static Gpu fromDevice(uint vendorId, uint deviceId, QVersionNumber driverVersion, const QByteArray &driverDescription) {
            Gpu gpu;
            gpu.vendorId = vendorId;
            gpu.deviceId = deviceId;
            gpu.driverVersion = driverVersion;
            gpu.driverDescription = driverDescription;
            return gpu;
        }

        static Gpu fromGLVendor(const QByteArray &glVendor) {
            Gpu gpu;
            gpu.glVendor = glVendor;
            return gpu;
        }

        static Gpu fromContext();
    };

    static QSet<QString> gpuFeatures(const Gpu &gpu,
                                     const QString &osName, const QVersionNumber &kernelVersion, const QString &osVersion,
                                     const QJsonDocument &doc);
    static QSet<QString> gpuFeatures(const Gpu &gpu,
                                     const QString &osName, const QVersionNumber &kernelVersion, const QString &osVersion,
                                     const QString &fileName);
    static QSet<QString> gpuFeatures(const Gpu &gpu, const QJsonDocument &doc);
    static QSet<QString> gpuFeatures(const Gpu &gpu, const QString &fileName);
};

inline bool operator==(const QOpenGLConfig::Gpu &a, const QOpenGLConfig::Gpu &b)
{
    return a.equals(b);
}

inline bool operator!=(const QOpenGLConfig::Gpu &a, const QOpenGLConfig::Gpu &b)
{
    return !a.equals(b);
}

inline size_t qHash(const QOpenGLConfig::Gpu &gpu, size_t seed = 0)
{
    return (qHash(gpu.vendorId) + qHash(gpu.deviceId) + qHash(gpu.driverVersion)) ^ seed;
}

QT_END_NAMESPACE

#endif // QOPENGL_H
