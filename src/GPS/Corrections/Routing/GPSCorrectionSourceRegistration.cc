#include "GPSCorrectionSourceRegistration.h"

#include "GPSCorrectionRouter.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSCorrectionSourceRegistrationLog, "GPS.Corrections.GPSCorrectionSourceRegistration")

GPSCorrectionSourceRegistration::Weak::Weak(GPSCorrectionRouter* router, GPSCorrectionSource source, quint64 generation,
                                            const QString& instance)
    : _router(router)
    , _source(source)
    , _generation(generation)
    , _instance(instance)
{}

bool GPSCorrectionSourceRegistration::Weak::valid() const
{
    return _router && _router->_isCurrent(*this);
}

GPSCorrectionIngress GPSCorrectionSourceRegistration::Weak::event(QByteArray data, qint64 receivedAtMs, int messageId,
                                                                  bool validated, bool filtered,
                                                                  GPSCorrectionReason rejection,
                                                                  const QString& peerInstance) const
{
    GPSCorrectionIngress ingress;
    ingress._source = *this;
    ingress._frame = {_source,   _generation, receivedAtMs, std::move(data),
                      messageId, validated,   filtered,     peerInstance.isEmpty() ? _instance : peerInstance};
    ingress._rejection = rejection;
    return ingress;
}

GPSCorrectionIngress GPSCorrectionSourceRegistration::Weak::event(const RTCMDecodedFrame& result) const
{
    return event(result.data, result.receivedAtMs, result.messageId, result.valid, result.filtered,
                 result.valid ? GPSCorrectionReason::None : GPSCorrectionReason::InvalidFrame);
}

GPSCorrectionIngress GPSCorrectionSourceRegistration::Weak::event(const GPSCorrectionFrame& frame,
                                                                  GPSCorrectionReason rejection) const
{
    return event(frame.data, frame.receivedAtMs, frame.messageId,
                 frame.validated && rejection == GPSCorrectionReason::None, frame.filtered, rejection,
                 frame.sourceInstance);
}

GPSCorrectionSourceRegistration::GPSCorrectionSourceRegistration()
{
    qCDebug(GPSCorrectionSourceRegistrationLog) << this;
}

GPSCorrectionSourceRegistration::GPSCorrectionSourceRegistration(Weak weak)
    : _weak(std::move(weak))
{
    qCDebug(GPSCorrectionSourceRegistrationLog) << this;
}

GPSCorrectionSourceRegistration::~GPSCorrectionSourceRegistration()
{
    qCDebug(GPSCorrectionSourceRegistrationLog) << this;
    reset();
}

GPSCorrectionSourceRegistration::GPSCorrectionSourceRegistration(GPSCorrectionSourceRegistration&& other) noexcept
    : _weak(std::exchange(other._weak, {}))
{
    qCDebug(GPSCorrectionSourceRegistrationLog) << this;
}

GPSCorrectionSourceRegistration& GPSCorrectionSourceRegistration::operator=(
    GPSCorrectionSourceRegistration&& other) noexcept
{
    GPSCorrectionSourceRegistration replacement(std::move(other));
    std::swap(_weak, replacement._weak);
    return *this;
}

void GPSCorrectionSourceRegistration::reset()
{
    const auto weak = std::exchange(_weak, {});
    if (weak.valid()) {
        weak._router->endSourceSession(weak._source);
    }
}
