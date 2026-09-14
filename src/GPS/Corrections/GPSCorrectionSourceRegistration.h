#pragma once

#include <QtCore/QPointer>

#include "GPSCorrectionDiagnostics.h"

class GPSCorrectionRouter;
class GPSCorrectionIngress;

/// Copyable identity captured by producer callbacks; retirement never changes a captured epoch.
class GPSCorrectionSourceToken
{
public:
    GPSCorrectionSourceToken() = default;
    bool valid() const;
    bool belongsTo(const GPSCorrectionRouter* router) const;

    GPSCorrectionSource source() const { return _source; }

    quint64 session() const { return _session; }

    QString instance() const { return _instance; }

    GPSCorrectionIngress event(QByteArray data, qint64 receivedAtMs, int messageId, bool validated,
                               bool filtered = false, GPSCorrectionReason rejection = GPSCorrectionReason::None,
                               const QString& peerInstance = {}) const;

private:
    friend class GPSCorrectionRouter;
    friend class GPSCorrectionSourceRegistration;
    GPSCorrectionSourceToken(GPSCorrectionRouter* router, GPSCorrectionSource source, quint64 session,
                             const QString& instance);
    QPointer<GPSCorrectionRouter> _router;
    GPSCorrectionSource _source = GPSCorrectionSource::Unknown;
    quint64 _session = 0;
    QString _instance;
};

class GPSCorrectionIngress
{
public:
    const GPSCorrectionSourceToken& token() const { return _token; }

    const GPSCorrectionFrame& frame() const { return _frame; }

    GPSCorrectionReason rejection() const { return _rejection; }

private:
    friend class GPSCorrectionSourceToken;
    GPSCorrectionSourceToken _token;
    GPSCorrectionFrame _frame;
    GPSCorrectionReason _rejection = GPSCorrectionReason::None;
};
Q_DECLARE_METATYPE(GPSCorrectionIngress)

/// Owns one router registration. Destroying a retired registration cannot stop its replacement.
class GPSCorrectionSourceRegistration
{
public:
    GPSCorrectionSourceRegistration();
    ~GPSCorrectionSourceRegistration();
    GPSCorrectionSourceRegistration(GPSCorrectionSourceRegistration&& other) noexcept;
    GPSCorrectionSourceRegistration& operator=(GPSCorrectionSourceRegistration&& other) noexcept;
    GPSCorrectionSourceRegistration(const GPSCorrectionSourceRegistration&) = delete;
    GPSCorrectionSourceRegistration& operator=(const GPSCorrectionSourceRegistration&) = delete;

    const GPSCorrectionSourceToken& token() const { return _token; }

    void reset();

private:
    friend class GPSCorrectionRouter;
    explicit GPSCorrectionSourceRegistration(GPSCorrectionSourceToken token);
    GPSCorrectionSourceToken _token;
};
