// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKINFORMATION_P_H
#define QNETWORKINFORMATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of the Network Information API. This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include <QtNetwork/qnetworkinformation.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qreadwritelock.h>

QT_BEGIN_NAMESPACE

class Q_NETWORK_EXPORT QNetworkInformationBackend : public QObject
{
    Q_OBJECT

    using Reachability = QNetworkInformation::Reachability;
    using TransportMedium = QNetworkInformation::TransportMedium;

public:
    static inline const char16_t PluginNames[4][24] = {
        { u"networklistmanager" },
        { u"applenetworkinformation" },
        { u"android" },
        { u"networkmanager" },
    };
    static constexpr int PluginNamesWindowsIndex = 0;
    static constexpr int PluginNamesAppleIndex = 1;
    static constexpr int PluginNamesAndroidIndex = 2;
    static constexpr int PluginNamesLinuxIndex = 3;

    QNetworkInformationBackend() = default;
    ~QNetworkInformationBackend() override;

    virtual QString name() const = 0;
    virtual QNetworkInformation::Features featuresSupported() const = 0;

    Reachability reachability() const
    {
        QReadLocker locker(&m_lock);
        return m_reachability;
    }

    bool behindCaptivePortal() const
    {
        QReadLocker locker(&m_lock);
        return m_behindCaptivePortal;
    }

    TransportMedium transportMedium() const
    {
        QReadLocker locker(&m_lock);
        return m_transportMedium;
    }

    bool isMetered() const
    {
        QReadLocker locker(&m_lock);
        return m_metered;
    }

Q_SIGNALS:
    void reachabilityChanged(QNetworkInformation::Reachability reachability);
    void behindCaptivePortalChanged(bool behindPortal);
    void transportMediumChanged(QNetworkInformation::TransportMedium medium);
    void isMeteredChanged(bool isMetered);

protected:
    void setReachability(QNetworkInformation::Reachability reachability)
    {
        QWriteLocker locker(&m_lock);
        if (m_reachability != reachability) {
            m_reachability = reachability;
            locker.unlock();
            emit reachabilityChanged(reachability);
        }
    }

    void setBehindCaptivePortal(bool behindPortal)
    {
        QWriteLocker locker(&m_lock);
        if (m_behindCaptivePortal != behindPortal) {
            m_behindCaptivePortal = behindPortal;
            locker.unlock();
            emit behindCaptivePortalChanged(behindPortal);
        }
    }

    void setTransportMedium(TransportMedium medium)
    {
        QWriteLocker locker(&m_lock);
        if (m_transportMedium != medium) {
            m_transportMedium = medium;
            locker.unlock();
            emit transportMediumChanged(medium);
        }
    }

    void setMetered(bool isMetered)
    {
        QWriteLocker locker(&m_lock);
        if (m_metered != isMetered) {
            m_metered = isMetered;
            locker.unlock();
            emit isMeteredChanged(isMetered);
        }
    }

private:
    mutable QReadWriteLock m_lock;
    Reachability m_reachability = Reachability::Unknown;
    TransportMedium m_transportMedium = TransportMedium::Unknown;
    bool m_behindCaptivePortal = false;
    bool m_metered = false;

    Q_DISABLE_COPY_MOVE(QNetworkInformationBackend)
    friend class QNetworkInformation;
    friend class QNetworkInformationPrivate;
};

class Q_NETWORK_EXPORT QNetworkInformationBackendFactory : public QObject
{
    Q_OBJECT

    using Features = QNetworkInformation::Features;

public:
    QNetworkInformationBackendFactory();
    virtual ~QNetworkInformationBackendFactory();
    virtual QString name() const = 0;
    virtual QNetworkInformationBackend *create(Features requiredFeatures) const = 0;
    virtual Features featuresSupported() const = 0;

private:
    Q_DISABLE_COPY_MOVE(QNetworkInformationBackendFactory)
};
#define QNetworkInformationBackendFactory_iid "org.qt-project.Qt.NetworkInformationBackendFactory"
Q_DECLARE_INTERFACE(QNetworkInformationBackendFactory, QNetworkInformationBackendFactory_iid);

QT_END_NAMESPACE

#endif
