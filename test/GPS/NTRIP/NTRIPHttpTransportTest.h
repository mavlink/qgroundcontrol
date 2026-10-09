#pragma once

#include <QtCore/QByteArrayView>

#include "UnitTest.h"

class NTRIPHttpSession;
class NTRIPHttpTransport;

class NTRIPHttpTransportTest : public UnitTest
{
    Q_OBJECT

    /// Gives @a transport a live session without a socket, as start() would, on the transport's scheduler.
    static NTRIPHttpSession* _attachSession(NTRIPHttpTransport& transport);
    /// Delivers caster bytes received at @a receivedAtMs to the transport's live session, attaching one if needed.
    static void _feed(NTRIPHttpTransport& transport, QByteArrayView bytes, qint64 receivedAtMs);

private slots:
    // RTCM framing and filtering
    void _rtcmWhitelist_data();
    void _rtcmWhitelist();
    void _filterRejectsInvalidFrame_data();
    void _filterRejectsInvalidFrame();
    void _filterConfigurationUpdatesWithoutReconnect();
    void _receiptTimesAndEvidence_data();
    void _receiptTimesAndEvidence();
    void _icyPartialFrame_data();
    void _icyPartialFrame();

    // Connection lifecycle
    void _invalidConfigurationIsNotConnected();
    void _connectionWaitsForHttpResponse();
    void _legacyCaster_data();
    void _legacyCaster();
    void _writeAdmissionFailure_data();
    void _writeAdmissionFailure();
    void _handshakeTimeoutClosesSocket();
    void _remoteCloseEmitsSingleError();
    void _socketTermination_data();
    void _socketTermination();
    void _errorBodyEnds_data();
    void _errorBodyEnds();
    void _validFrameWatchdog_data();
    void _validFrameWatchdog();
    void _silentStreamWatchdog();

    // Observers that stop or restart the transport
    void _handshakeRetiresAttempt_data();
    void _handshakeRetiresAttempt();
    void _failureCanRestart_data();
    void _failureCanRestart();
    void _pendingErrorRetiresAttempt_data();
    void _pendingErrorRetiresAttempt();
    void _bodyPublicationRetiresAttempt_data();
    void _bodyPublicationRetiresAttempt();
};
