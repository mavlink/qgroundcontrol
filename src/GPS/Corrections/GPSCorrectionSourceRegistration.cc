#include "GPSCorrectionSourceRegistration.h"

#include <utility>

#include "GPSCorrectionRouter.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSCorrectionSourceRegistrationLog, "GPS.Corrections.GPSCorrectionSourceRegistration")

GPSCorrectionSourceToken::GPSCorrectionSourceToken(GPSCorrectionRouter* router, GPSCorrectionSource source,
                                                   quint64 session, const QString& instance)
    : _router(router), _source(source), _session(session), _instance(instance)
{}

bool GPSCorrectionSourceToken::valid() const
{
    return _router && _router->isCurrentSource(_source, _session, _instance);
}

bool GPSCorrectionSourceToken::belongsTo(const GPSCorrectionRouter* router) const
{
    return _router == router;
}

GPSCorrectionIngress GPSCorrectionSourceToken::event(QByteArray data, qint64 receivedAtMs, int messageId,
                                                     bool validated, bool filtered, GPSCorrectionReason rejection,
                                                     const QString& peerInstance) const
{
    GPSCorrectionIngress ingress;
    ingress._token = *this;
    ingress._frame = {_source,   _session,  receivedAtMs, std::move(data),
                      messageId, validated, filtered,     peerInstance.isEmpty() ? _instance : peerInstance};
    ingress._rejection = rejection;
    return ingress;
}

GPSCorrectionSourceRegistration::GPSCorrectionSourceRegistration()
{
    qCDebug(GPSCorrectionSourceRegistrationLog) << this;
}

GPSCorrectionSourceRegistration::GPSCorrectionSourceRegistration(GPSCorrectionSourceToken token)
    : _token(std::move(token))
{
    qCDebug(GPSCorrectionSourceRegistrationLog) << this;
}

GPSCorrectionSourceRegistration::~GPSCorrectionSourceRegistration()
{
    qCDebug(GPSCorrectionSourceRegistrationLog) << this;
    reset();
}

GPSCorrectionSourceRegistration::GPSCorrectionSourceRegistration(GPSCorrectionSourceRegistration&& other) noexcept
    : _token(std::exchange(other._token, {}))
{
    qCDebug(GPSCorrectionSourceRegistrationLog) << this;
}

GPSCorrectionSourceRegistration& GPSCorrectionSourceRegistration::operator=(
    GPSCorrectionSourceRegistration&& other) noexcept
{
    GPSCorrectionSourceRegistration replacement(std::move(other));
    std::swap(_token, replacement._token);
    return *this;
}

void GPSCorrectionSourceRegistration::reset()
{
    const auto token = std::exchange(_token, {});
    if (token.valid()) {
        token._router->endSourceSession(token.source());
    }
}
