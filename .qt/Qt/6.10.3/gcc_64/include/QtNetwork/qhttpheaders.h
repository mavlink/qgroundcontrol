// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QHTTPHEADERS_H
#define QHTTPHEADERS_H

#include <QtNetwork/qtnetworkglobal.h>

#include <QtCore/qdatetime.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qobjectdefs.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qcontainerfwd.h>

QT_BEGIN_NAMESPACE

class QDebug;

class QHttpHeadersPrivate;
QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(QHttpHeadersPrivate, Q_NETWORK_EXPORT)
class QHttpHeaders
{
    Q_GADGET_EXPORT(Q_NETWORK_EXPORT)
public:
    enum class WellKnownHeader {
        // IANA Permanent status:
        AIM,
        Accept,
        AcceptAdditions,
        AcceptCH,
        AcceptDatetime,
        AcceptEncoding,
        AcceptFeatures,
        AcceptLanguage,
        AcceptPatch,
        AcceptPost,
        AcceptRanges,
        AcceptSignature,
        AccessControlAllowCredentials,
        AccessControlAllowHeaders,
        AccessControlAllowMethods,
        AccessControlAllowOrigin,
        AccessControlExposeHeaders,
        AccessControlMaxAge,
        AccessControlRequestHeaders,
        AccessControlRequestMethod,
        Age,
        Allow,
        ALPN,
        AltSvc,
        AltUsed,
        Alternates,
        ApplyToRedirectRef,
        AuthenticationControl,
        AuthenticationInfo,
        Authorization,
        CacheControl,
        CacheStatus,
        CalManagedID,
        CalDAVTimezones,
        CapsuleProtocol,
        CDNCacheControl,
        CDNLoop,
        CertNotAfter,
        CertNotBefore,
        ClearSiteData,
        ClientCert,
        ClientCertChain,
        Close,
        Connection,
        ContentDigest,
        ContentDisposition,
        ContentEncoding,
        ContentID,
        ContentLanguage,
        ContentLength,
        ContentLocation,
        ContentRange,
        ContentSecurityPolicy,
        ContentSecurityPolicyReportOnly,
        ContentType,
        Cookie,
        CrossOriginEmbedderPolicy,
        CrossOriginEmbedderPolicyReportOnly,
        CrossOriginOpenerPolicy,
        CrossOriginOpenerPolicyReportOnly,
        CrossOriginResourcePolicy,
        DASL,
        Date,
        DAV,
        DeltaBase,
        Depth,
        Destination,
        DifferentialID,
        DPoP,
        DPoPNonce,
        EarlyData,
        ETag,
        Expect,
        ExpectCT,
        Expires,
        Forwarded,
        From,
        Hobareg,
        Host,
        If,
        IfMatch,
        IfModifiedSince,
        IfNoneMatch,
        IfRange,
        IfScheduleTagMatch,
        IfUnmodifiedSince,
        IM,
        IncludeReferredTokenBindingID,
        KeepAlive,
        Label,
        LastEventID,
        LastModified,
        Link,
        Location,
        LockToken,
        MaxForwards,
        MementoDatetime,
        Meter,
        MIMEVersion,
        Negotiate,
        NEL,
        ODataEntityId,
        ODataIsolation,
        ODataMaxVersion,
        ODataVersion,
        OptionalWWWAuthenticate,
        OrderingType,
        Origin,
        OriginAgentCluster,
        OSCORE,
        OSLCCoreVersion,
        Overwrite,
        PingFrom,
        PingTo,
        Position,
        Prefer,
        PreferenceApplied,
        Priority,
        ProxyAuthenticate,
        ProxyAuthenticationInfo,
        ProxyAuthorization,
        ProxyStatus,
        PublicKeyPins,
        PublicKeyPinsReportOnly,
        Range,
        RedirectRef,
        Referer,
        Refresh,
        ReplayNonce,
        ReprDigest,
        RetryAfter,
        ScheduleReply,
        ScheduleTag,
        SecPurpose,
        SecTokenBinding,
        SecWebSocketAccept,
        SecWebSocketExtensions,
        SecWebSocketKey,
        SecWebSocketProtocol,
        SecWebSocketVersion,
        Server,
        ServerTiming,
        SetCookie,
        Signature,
        SignatureInput,
        SLUG,
        SoapAction,
        StatusURI,
        StrictTransportSecurity,
        Sunset,
        SurrogateCapability,
        SurrogateControl,
        TCN,
        TE,
        Timeout,
        Topic,
        Traceparent,
        Tracestate,
        Trailer,
        TransferEncoding,
        TTL,
        Upgrade,
        Urgency,
        UserAgent,
        VariantVary,
        Vary,
        Via,
        WantContentDigest,
        WantReprDigest,
        WWWAuthenticate,
        XContentTypeOptions,
        XFrameOptions,
        // IANA Deprecated status:
        AcceptCharset,
        CPEPInfo,
        Pragma,
        ProtocolInfo,
        ProtocolQuery,
    };
    Q_ENUM(WellKnownHeader)

    Q_NETWORK_EXPORT QHttpHeaders() noexcept;
    Q_NETWORK_EXPORT ~QHttpHeaders();

    Q_NETWORK_EXPORT QHttpHeaders(const QHttpHeaders &other);
    QHttpHeaders(QHttpHeaders &&other) noexcept = default;
    Q_NETWORK_EXPORT QHttpHeaders &operator=(const QHttpHeaders &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QHttpHeaders)
    void swap(QHttpHeaders &other) noexcept { d.swap(other.d); }

    Q_NETWORK_EXPORT bool append(QAnyStringView name, QAnyStringView value);
    Q_NETWORK_EXPORT bool append(WellKnownHeader name, QAnyStringView value);

    Q_NETWORK_EXPORT bool insert(qsizetype i, QAnyStringView name, QAnyStringView value);
    Q_NETWORK_EXPORT bool insert(qsizetype i, WellKnownHeader name, QAnyStringView value);

    Q_NETWORK_EXPORT bool replace(qsizetype i, QAnyStringView name, QAnyStringView newValue);
    Q_NETWORK_EXPORT bool replace(qsizetype i, WellKnownHeader name, QAnyStringView newValue);

    Q_NETWORK_EXPORT bool replaceOrAppend(QAnyStringView name, QAnyStringView newValue);
    Q_NETWORK_EXPORT bool replaceOrAppend(WellKnownHeader name, QAnyStringView newValue);

    Q_NETWORK_EXPORT bool contains(QAnyStringView name) const;
    Q_NETWORK_EXPORT bool contains(WellKnownHeader name) const;

    Q_NETWORK_EXPORT void clear();
    Q_NETWORK_EXPORT void removeAll(QAnyStringView name);
    Q_NETWORK_EXPORT void removeAll(WellKnownHeader name);
    Q_NETWORK_EXPORT void removeAt(qsizetype i);

    Q_NETWORK_EXPORT QByteArrayView value(QAnyStringView name, QByteArrayView defaultValue = {}) const noexcept;
    Q_NETWORK_EXPORT QByteArrayView value(WellKnownHeader name, QByteArrayView defaultValue = {}) const noexcept;

    Q_NETWORK_EXPORT QList<QByteArray> values(QAnyStringView name) const;
    Q_NETWORK_EXPORT QList<QByteArray> values(WellKnownHeader name) const;

    Q_NETWORK_EXPORT QByteArrayView valueAt(qsizetype i) const noexcept;
    Q_NETWORK_EXPORT QLatin1StringView nameAt(qsizetype i) const noexcept;

    Q_NETWORK_EXPORT QByteArray combinedValue(QAnyStringView name) const;
    Q_NETWORK_EXPORT QByteArray combinedValue(WellKnownHeader name) const;

    Q_NETWORK_EXPORT std::optional<qint64> intValue(QAnyStringView name) const noexcept;
    Q_NETWORK_EXPORT std::optional<qint64> intValue(WellKnownHeader name) const noexcept;

    Q_NETWORK_EXPORT std::optional<QList<qint64>> intValues(QAnyStringView name) const;
    Q_NETWORK_EXPORT std::optional<QList<qint64>> intValues(WellKnownHeader name) const;

    Q_NETWORK_EXPORT std::optional<qint64> intValueAt(qsizetype i) const noexcept;

    Q_NETWORK_EXPORT std::optional<QDateTime> dateTimeValue(QAnyStringView name) const;
    Q_NETWORK_EXPORT std::optional<QDateTime> dateTimeValue(WellKnownHeader name) const;

    Q_NETWORK_EXPORT std::optional<QList<QDateTime>> dateTimeValues(QAnyStringView name) const;
    Q_NETWORK_EXPORT std::optional<QList<QDateTime>> dateTimeValues(WellKnownHeader name) const;

    Q_NETWORK_EXPORT std::optional<QDateTime> dateTimeValueAt(qsizetype i) const;

    Q_NETWORK_EXPORT void setDateTimeValue(QAnyStringView name, const QDateTime &dateTime);
    Q_NETWORK_EXPORT void setDateTimeValue(WellKnownHeader name, const QDateTime &dateTime);

    Q_NETWORK_EXPORT qsizetype size() const noexcept;
    Q_NETWORK_EXPORT void reserve(qsizetype size);
    bool isEmpty() const noexcept { return size() == 0; }

    Q_NETWORK_EXPORT static QByteArrayView wellKnownHeaderName(WellKnownHeader name) noexcept;

    Q_NETWORK_EXPORT static QHttpHeaders
    fromListOfPairs(const QList<std::pair<QByteArray, QByteArray>> &headers);
    Q_NETWORK_EXPORT static QHttpHeaders
    fromMultiMap(const QMultiMap<QByteArray, QByteArray> &headers);
    Q_NETWORK_EXPORT static QHttpHeaders
    fromMultiHash(const QMultiHash<QByteArray, QByteArray> &headers);

    Q_NETWORK_EXPORT QList<std::pair<QByteArray, QByteArray>> toListOfPairs() const;
    Q_NETWORK_EXPORT QMultiMap<QByteArray, QByteArray> toMultiMap() const;
    Q_NETWORK_EXPORT QMultiHash<QByteArray, QByteArray> toMultiHash() const;

private:
#ifndef QT_NO_DEBUG_STREAM
    friend Q_NETWORK_EXPORT QDebug operator<<(QDebug debug, const QHttpHeaders &headers);
#endif
    Q_ALWAYS_INLINE void verify([[maybe_unused]] qsizetype pos = 0,
                                [[maybe_unused]] qsizetype n = 1) const
    {
        Q_ASSERT(pos >= 0);
        Q_ASSERT(pos <= size());
        Q_ASSERT(n >= 0);
        Q_ASSERT(n <= size() - pos);
    }
    QExplicitlySharedDataPointer<QHttpHeadersPrivate> d;
};

Q_DECLARE_SHARED(QHttpHeaders)

QT_END_NAMESPACE

#endif // QHTTPHEADERS_H
