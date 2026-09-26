#include "Femto/FemtoConfigurator.h"

#include <variant>

#include "Femto/FemtoDecoder.h"
#include "Femto/FemtoPlan.h"
#include "GPSCommandChannel.h"
#include "GPSEventSink.h"
#include "GPSFamilyProtocol.h"
#include "GPSStreamDemux.h"

namespace Femto {

GPSTask<bool> Configurator::configure(GPSCommandChannel& channel, GPSConfig config, unsigned& baud)
{
    Session& session = _decoder.session();
    session = {};
    // Corrections are only framed from a receiver that answered.
    GPSStreamDemux& stream = channel.stream();
    stream.setEnabled(GPSFrameKind::RTCM3, false);
    stream.reset(GPSFrameKind::NMEASentence);
    if (!channel.validateConfiguration(config)) {
        co_return false;
    }
    _base = config.base;
    if (baud != 0 && baud != Plan::BAUD) {
        co_return false;
    }
    (void) co_await channel.setBaudrate(Plan::BAUD);
    bool identified = false;
    for (unsigned round = 0; round < Plan::IDENTIFY_ROUNDS && !identified; ++round) {
        identified = (co_await channel.runSequence(Plan::sequence(Plan::IDENTIFY))).succeeded();
    }
    if (!identified) {
        co_return false;
    }

    baud = Plan::BAUD;
    stream.reset(GPSFrameKind::NMEASentence);
    stream.setEnabled(GPSFrameKind::RTCM3, true);
    co_await _activateCorrectionOutput(channel);
    session.configured = !channel.failed();
    co_return session.configured;
}

GPSTask<void> Configurator::serviceStreaming(GPSCommandChannel& channel)
{
    Session& session = _decoder.session();
    if (session.rtcmActivationPending) {
        session.rtcmActivationPending = false;
        co_await _activateRTCMOutput(channel);
    }
}

GPSTask<void> Configurator::_activateCorrectionOutput(GPSCommandChannel& channel)
{
    Session& session = _decoder.session();
    GPSEventSink& sink = channel.context().sink();
    const auto* fixed = std::get_if<GPSBaseStationConfig::Fixed>(&_base.mode);
    if (!fixed) {
        if (!(co_await channel.runSequence(Plan::sequence(Plan::SURVEY_IN))).succeeded()) {
            channel.failControl();
            co_return;
        }
        session.surveyClock.start(channel.nowUs());
        sink.publishSurvey(true, false, session.surveyClock.duration());
        co_return;
    }
    if (!(co_await channel.runSequence(Plan::fixedBase(fixed->position))).succeeded()) {
        channel.failControl();
        co_return;
    }
    co_await _activateRTCMOutput(channel);
    if (session.correctionOutputActive) {
        sink.publishSurvey(false, true, {}, fixed->position);
    }
}

GPSTask<void> Configurator::_activateRTCMOutput(GPSCommandChannel& channel)
{
    if (!(co_await channel.runSequence(Plan::sequence(Plan::RTCM_OUTPUT))).succeeded()) {
        channel.failControl();
        co_return;
    }
    _decoder.session().correctionOutputActive = true;
}

}  // namespace Femto
