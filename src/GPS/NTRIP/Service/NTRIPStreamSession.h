#pragma once

#include <functional>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QVector>

#include "GPSCorrectionSourceRegistration.h"
#include "GPSRevision.h"
#include "NTRIPError.h"
#include "RTCMDecodedFrame.h"

class GPSCorrectionManager;
class NTRIPConnectionStats;
class NTRIPGgaProvider;
class NTRIPTransport;
struct NTRIPConnectionConfig;

/// One caster connection: its transport, correction-source registration and RTCM forwarding, plus the per-connection
/// start and stop of the owner's GGA provider and statistics. The session is the connection's identity: once its
/// transport is closed it ignores late transport callbacks and emits nothing further. Observers may close, stop or
/// delete the session's owner from any of its signals.
class NTRIPStreamSession : public QObject
{
    Q_OBJECT

public:
    using TransportFactory = std::function<NTRIPTransport*()>;

    /// The GGA provider and statistics belong to the owner and must outlive the session's use of them.
    NTRIPStreamSession(NTRIPGgaProvider& gga, NTRIPConnectionStats& stats, QObject* parent = nullptr);

    /// Resets the statistics, creates the transport, registers the correction source when corrections is set, and
    /// starts the transport. Stops early when a re-entrant close or stop supersedes it.
    void open(const NTRIPConnectionConfig& connection, GPSCorrectionManager* corrections,
              const TransportFactory& createTransport);
    /// Starts GGA and statistics once the caster accepted the stream.
    void startStreaming();
    /// Stops the transport and ends the correction-source registration. GGA and statistics keep running so a
    /// replacement session can take them over.
    void closeTransport();
    /// Closes the transport, then stops GGA and statistics. Returns false when a re-entrant close or stop superseded
    /// it, or the session was deleted meanwhile.
    [[nodiscard]] bool stop();

    bool hasTransport() const { return !_transport.isNull(); }

    void setRtcmWhitelist(const QVector<int>& messageIds);

signals:
    void connected();
    void failed(const NTRIPFailure& failure);
    /// A valid, unfiltered frame, after the statistics recorded it.
    void rtcmReceived(const RTCMDecodedFrame& frame);
    void plaintextCredentialsWarning();

private:
    void _closeTransport();
    void _onCorrectionFrame(const QPointer<GPSCorrectionManager>& corrections,
                            const GPSCorrectionSourceRegistration::Weak& source, const RTCMDecodedFrame& frame);
    bool _isCurrent() const;

    NTRIPGgaProvider& _gga;
    NTRIPConnectionStats& _stats;
    QPointer<NTRIPTransport> _transport;
    GPSCorrectionSourceRegistration _registration;
    bool _registered = false;
    GPSRevision _revision;
};
