// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QHSTS_P_H
#define QHSTS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include <QtNetwork/qhstspolicy.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qstring.h>
#include <QtCore/qurl.h>
#include <QtCore/qcontainerfwd.h>

#include <map>

QT_BEGIN_NAMESPACE

class QHttpHeaders;

class Q_AUTOTEST_EXPORT QHstsCache
{
public:

    void updateFromHeaders(const QHttpHeaders &headers,
                           const QUrl &url);
    void updateFromPolicies(const QList<QHstsPolicy> &hosts);
    void updateKnownHost(const QUrl &url, const QDateTime &expires,
                         bool includeSubDomains);
    bool isKnownHost(const QUrl &url) const;
    void clear();

    QList<QHstsPolicy> policies() const;

#if QT_CONFIG(settings)
    void setStore(class QHstsStore *store);
#endif // QT_CONFIG(settings)

private:

    void updateKnownHost(const QString &hostName, const QDateTime &expires,
                         bool includeSubDomains);

    struct HostName
    {
        explicit HostName(const QString &n) : name(n) { }
        explicit HostName(QStringView r) : fragment(r) { }

        bool operator < (const HostName &rhs) const
        {
            if (fragment.size()) {
                if (rhs.fragment.size())
                    return fragment < rhs.fragment;
                return fragment < QStringView{rhs.name};
            }

            if (rhs.fragment.size())
                return QStringView{name} < rhs.fragment;
            return name < rhs.name;
        }

        // We use 'name' for a HostName object contained in our dictionary;
        // we use 'fragment' only during lookup, when chopping the complete host
        // name, removing subdomain names (such HostName object is 'transient', it
        // must not outlive the original QString object.
        QString name;
        QStringView fragment;
    };

    mutable std::map<HostName, QHstsPolicy> knownHosts;
#if QT_CONFIG(settings)
    QHstsStore *hstsStore = nullptr;
#endif // QT_CONFIG(settings)
};

class Q_AUTOTEST_EXPORT QHstsHeaderParser
{
public:

    bool parse(const QHttpHeaders &headers);

    QDateTime expirationDate() const { return expiry; }
    bool includeSubDomains() const { return subDomainsFound; }

private:

    bool parseSTSHeader();
    bool parseDirective();
    bool processDirective(const QByteArray &name, const QByteArray &value);
    bool nextToken();

    QByteArray header;
    QByteArray token;

    QDateTime expiry;
    int tokenPos = 0;
    bool maxAgeFound = false;
    qint64 maxAge = 0;
    bool subDomainsFound = false;
};

QT_END_NAMESPACE

#endif
