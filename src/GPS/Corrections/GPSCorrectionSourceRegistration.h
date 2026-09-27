#pragma once

#include <utility>

#include <QtCore/QPointer>

#include "GPSCorrectionDiagnostics.h"
#include "RTCMDecodedFrame.h"

class GPSCorrectionIngress;
class GPSCorrectionRouter;

/// Owns one router registration and its immutable generation. Ingress built from a replaced or retired registration,
/// or from any of its weak views, is ignored, and destroying a retired registration cannot end its replacement.
class GPSCorrectionSourceRegistration
{
public:
    /// Copyable, non-owning view that queued callbacks capture; it never keeps the registration current.
    class Weak
    {
    public:
        Weak() = default;
        bool valid() const;

        GPSCorrectionSource source() const { return _source; }

        quint64 generation() const { return _generation; }

        GPSCorrectionIngress event(QByteArray data, qint64 receivedAtMs, int messageId, bool validated,
                                   bool filtered = false, GPSCorrectionReason rejection = GPSCorrectionReason::None,
                                   const QString& peerInstance = {}) const;
        GPSCorrectionIngress event(const RTCMDecodedFrame& result) const;
        GPSCorrectionIngress event(const GPSCorrectionFrame& frame,
                                   GPSCorrectionReason rejection = GPSCorrectionReason::None) const;

    private:
        friend class GPSCorrectionRouter;
        friend class GPSCorrectionSourceRegistration;
        Weak(GPSCorrectionRouter* router, GPSCorrectionSource source, quint64 generation, const QString& instance);
        QPointer<GPSCorrectionRouter> _router;
        GPSCorrectionSource _source = GPSCorrectionSource::Unknown;
        quint64 _generation = 0;
        QString _instance;
    };

    GPSCorrectionSourceRegistration();
    ~GPSCorrectionSourceRegistration();
    GPSCorrectionSourceRegistration(GPSCorrectionSourceRegistration&& other) noexcept;
    GPSCorrectionSourceRegistration& operator=(GPSCorrectionSourceRegistration&& other) noexcept;
    GPSCorrectionSourceRegistration(const GPSCorrectionSourceRegistration&) = delete;
    GPSCorrectionSourceRegistration& operator=(const GPSCorrectionSourceRegistration&) = delete;

    bool valid() const { return _weak.valid(); }

    GPSCorrectionSource source() const { return _weak.source(); }

    quint64 generation() const { return _weak.generation(); }

    Weak weak() const { return _weak; }

    /// Builds ingress stamped with this registration; see Weak::event().
    template <typename... Args>
    auto event(Args&&... args) const
    {
        return _weak.event(std::forward<Args>(args)...);
    }

    void reset();

private:
    friend class GPSCorrectionRouter;
    explicit GPSCorrectionSourceRegistration(Weak weak);
    Weak _weak;
};

class GPSCorrectionIngress
{
public:
    const GPSCorrectionFrame& frame() const { return _frame; }

    GPSCorrectionReason rejection() const { return _rejection; }

private:
    friend class GPSCorrectionRouter;
    friend class GPSCorrectionSourceRegistration::Weak;
    GPSCorrectionSourceRegistration::Weak _source;
    GPSCorrectionFrame _frame;
    GPSCorrectionReason _rejection = GPSCorrectionReason::None;
};
Q_DECLARE_METATYPE(GPSCorrectionIngress)
