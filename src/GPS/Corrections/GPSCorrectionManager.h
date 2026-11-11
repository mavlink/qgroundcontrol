#pragma once

#include <functional>
#include <memory>
#include <optional>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

#include "DataRateTracker.h"
#include "GPSCorrectionSelector.h"
#include "RTCMLogFile.h"
#include "RTCMMAVLink.h"
#include "RTCMMessageCountModel.h"
#include "RTCMUdpInput.h"
#include "ScheduledTask.h"
#include "UdpForwarder.h"

class GPSCorrectionManager;
class RuntimeScheduler;

/// What a GPSCorrectionManager is given for the log of the routed stream.
struct GPSCorrectionManagerDependencies
{
    /// The folder a log goes to, asked as each log starts. Without it, or while it returns an empty path, the stream
    /// is not logged.
    std::function<QString()> logDirectory = nullptr;
    /// A log keeps the frames that fit in this many bytes and then ends.
    qint64 maxLogFileBytes = RTCMLogFile::MAX_FILE_BYTES;
};

/// A begun correction source and the stream it delivers; destroying it ends the source.
class GPSCorrectionSourceHandle
{
public:
    ~GPSCorrectionSourceHandle();
    Q_DISABLE_COPY_MOVE(GPSCorrectionSourceHandle)

    /// Offers one valid RTCM frame of the source's stream, received at @a receivedAtMs on the scheduler's clock. Fresh
    /// frames of the selected stream that the message filter takes go to vehicles and UDP forwarding. A non-empty
    /// @a instance names the frame's stream instead, for a source that names streams per frame.
    void submit(const QByteArray& data, qint64 receivedAtMs, const QString& instance = {}) const;

private:
    friend class GPSCorrectionManager;
    GPSCorrectionSourceHandle(GPSCorrectionManager* manager, GPSCorrectionSettings::CorrectionSource source,
                              const QString& instance);

    const QPointer<GPSCorrectionManager> _manager;
    const GPSCorrectionSettings::CorrectionSource _source;
    const QString _instance;
};

/// Owns the shared MAVLink sequence domain and the UDP correction input and output for all GPS sources. The selected
/// stream, as far as the message filter takes it, goes to vehicles over MAVLink and to UDP forwarding. Receipt times
/// are on the scheduler's clock; producers on worker threads stamp with the steady clock, which the application's
/// QtRuntimeScheduler uses.
class GPSCorrectionManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the GPS manager")
    Q_PROPERTY(State state READ state NOTIFY stateChanged FINAL)
    Q_PROPERTY(GPSCorrectionStream selectedStream READ selectedStream NOTIFY stateChanged FINAL)
    Q_PROPERTY(quint64 selectedBytesPerSecond READ selectedBytesPerSecond NOTIFY selectedBytesPerSecondChanged FINAL)
    Q_PROPERTY(quint64 vehicleBytesSubmitted READ vehicleBytesSubmitted NOTIFY vehicleBytesSubmittedChanged FINAL)
    Q_PROPERTY(QString udpInputError READ udpInputError NOTIFY udpInputChanged FINAL)
    Q_PROPERTY(QString udpOutputError READ udpOutputError NOTIFY udpOutputChanged FINAL)
    Q_PROPERTY(QString messageFilterError READ messageFilterError NOTIFY messageFilterChanged FINAL)
    /// Logs the frames routed from now on to a new file. Off at startup and never saved, and off again once the log
    /// ended after a file error or at its size limit.
    Q_PROPERTY(bool logToFile READ logToFile WRITE setLogToFile NOTIFY logChanged FINAL)
    /// The file the routed stream is logged to; empty while it is not logged.
    Q_PROPERTY(QString logFile READ logFile NOTIFY logChanged FINAL)
    Q_PROPERTY(RTCMMessageCountModel* messageCounts READ messageCounts CONSTANT FINAL)
    Q_PROPERTY(QGeoCoordinate basePosition READ basePosition NOTIFY basePositionChanged FINAL)

    friend class GPSCorrectionSourceHandle;

public:
    /// Whether vehicles receive RTK corrections, across NTRIP, UDP input, and the local receiver.
    enum class State
    {
        /// No correction source is enabled.
        Inactive,
        /// A source is enabled, but no fresh stream is selected for vehicles.
        Waiting,
        /// A fresh stream is selected for vehicles.
        Fresh,
    };
    Q_ENUM(State)

    struct UdpInputConfiguration
    {
        bool enabled = false;
        quint16 port = 0;
        bool operator==(const UdpInputConfiguration&) const = default;
    };

    struct UdpOutputConfiguration
    {
        bool enabled = false;
        QString address{};
        quint16 port = 0;
        bool operator==(const UdpOutputConfiguration&) const = default;
    };

    /// The correction settings as values; the application's settings binding supplies them.
    struct Configuration
    {
        /// The only source routed to vehicles, or HighestPriority to select automatically.
        GPSCorrectionSettings::CorrectionSource source = GPSCorrectionSettings::HighestPriority;
        /// The vehicles the selected stream is sent to; UDP forwarding does not depend on it.
        GPSCorrectionSettings::CorrectionVehicles vehicles = GPSCorrectionSettings::AllVehicles;
        /// Listens for RTCM on a UDP port as the Udp source. A port that cannot be bound is retried every second.
        UdpInputConfiguration udpInput;
        /// Forwards the selected stream to a UDP peer.
        UdpOutputConfiguration udpOutput;
        /// Comma-separated RTCM message IDs, from 1 to MAX_MESSAGE_ID: the only messages taken from any source. The
        /// others count as not received, so they are not routed, not counted and no sign of a fresh stream. Empty
        /// takes every message. With an entry that is not a message ID, none is taken until the filter is corrected.
        /// messageFilterError() reports that, and a filter that leaves no stream to route.
        QString messageFilter{};
        bool operator==(const Configuration&) const = default;
    };

    /// RTCM message numbers are 12 bits.
    static constexpr int MAX_MESSAGE_ID = 4095;

    using Dependencies = GPSCorrectionManagerDependencies;

    explicit GPSCorrectionManager(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr,
                                  const Dependencies& dependencies = {});
    ~GPSCorrectionManager() override;

    void shutdown();

    /// The vehicle output, whose links tests replace with RTCMMAVLink::setOutputProvider().
    RTCMMAVLink* rtcmMavlink() { return &_rtcmMavlink; }

    /// Applies the parts that changed; ignored after shutdown().
    void setConfiguration(const Configuration& configuration);
    /// Begins @a source, which delivers the stream @a instance, until the returned handle is destroyed. A category
    /// has one source at a time, so a replacement opens after the previous handle is gone.
    [[nodiscard]] std::unique_ptr<GPSCorrectionSourceHandle> openSource(GPSCorrectionSettings::CorrectionSource source,
                                                                        const QString& instance);

    /// As last published with stateChanged().
    State state() const { return _state; }

    /// The fresh stream selected for vehicles, as last published with stateChanged(); a HighestPriority source when
    /// none is.
    GPSCorrectionStream selectedStream() const { return _selected; }

    /// Rate of the corrections sent from the selected stream.
    quint64 selectedBytesPerSecond() const { return _selectedBytesPerSecond; }

    /// Bytes of the selected streams that vehicle links admitted, from every source since startup.
    quint64 vehicleBytesSubmitted() const { return _rtcmMavlink.totalBytesSubmitted(); }

    /// Why the enabled UDP input is not listening; empty while it listens or is disabled.
    QString udpInputError() const { return _udpInputError; }

    /// Why the enabled UDP output does not forward; empty while it forwards or is disabled.
    QString udpOutputError() const { return _udpOutputError; }

    /// Why the message filter lets no corrections through: it names the first entry that is not a message ID, or the
    /// messages that arrive while the filter takes none of them and no stream is selected. Empty otherwise.
    QString messageFilterError() const { return _messageFilterError; }

    bool logToFile() const { return _log.isOpen(); }

    /// Starts or ends the log: an RTCMLogFile in the Dependencies' log folder, named with the UTC time it starts at,
    /// of the frames as they go to vehicles and UDP forwarding. The log never affects the routing.
    void setLogToFile(bool log);

    QString logFile() const { return _log.file(); }

    /// The selected stream's frames per RTCM message ID, and how often each arrives. They start again when another
    /// stream is selected, and are empty while none is.
    RTCMMessageCountModel* messageCounts() { return &_messageCounts; }

    /// Where the selected stream's base station is, as its RTCM message 1005 or 1006 says, with the ellipsoid height
    /// as altitude. Invalid until the stream sends one, and once it is no longer the selected stream.
    QGeoCoordinate basePosition() const { return _basePosition; }

    Q_INVOKABLE static QString sourceName(int source);

signals:
    void stateChanged();
    void selectedBytesPerSecondChanged();
    void vehicleBytesSubmittedChanged();
    void udpInputChanged();
    void udpOutputChanged();
    void messageFilterChanged();
    void logChanged();
    void basePositionChanged();

private:
    /// See GPSCorrectionSelector::beginSource(); each source calls _endSource() before it is replaced or stops.
    void _beginSource(GPSCorrectionSettings::CorrectionSource source);
    void _endSource(GPSCorrectionSettings::CorrectionSource source);
    /// See GPSCorrectionSelector::submit(), which is offered the frames the message filter takes; a selected frame
    /// goes to vehicles and UDP forwarding.
    void _submit(GPSCorrectionSettings::CorrectionSource source, const QString& instance, const QByteArray& data,
                 qint64 receivedAtMs);
    /// Forwards @a data to the enabled UDP output and publishes whether it was sent; a repeated outcome is silent.
    void _forwardToUdpOutput(const QByteArray& data);
    /// Records what the frame @a data, received at @a receivedAtMs, tells about the selected stream @a stream.
    void _recordSelected(const GPSCorrectionStream& stream, const QByteArray& data, qint64 receivedAtMs);
    /// Forgets what the selected stream's frames told once it is no longer the selected one.
    void _forgetSelected();
    void _setBasePosition(const QGeoCoordinate& position);

    /// Each returns whether the input's state or the output's error changed.
    [[nodiscard]] bool _applyUdpInput();
    [[nodiscard]] bool _applyUdpOutput();
    /// Takes the message filter @a filter; the next refresh publishes its error.
    void _applyMessageFilter(const QString& filter);
    /// Publishes why the filter lets no corrections through, given whether a stream is selected at @a now.
    void _refreshMessageFilterError(bool streamSelected, qint64 now);
    bool _udpInputEnabled() const;
    void _startUdpInput();
    void _retryUdpInput();

    void _scheduleRefresh();
    void _refresh();

    /// messageFilterError() names at most this many of the messages the filter left out.
    static constexpr qsizetype MAX_LEFT_OUT_NAMED = 12;

    RuntimeScheduler* const _scheduler;
    ScheduledTask _refreshTask;
    /// Every second: retries the UDP input, completes the rate window and refreshes the state.
    ScheduledTask _tickTask;
    GPSCorrectionSelector _selector;
    DataRateTracker _selectedRate;
    RTCMMAVLink _rtcmMavlink;
    RTCMUdpInput _udpInput;
    std::unique_ptr<GPSCorrectionSourceHandle> _udpSource;
    UdpForwarder _udpOutput{this};
    const std::function<QString()> _logDirectory;
    RTCMLogFile _log;
    std::optional<Configuration> _configuration;
    State _state = State::Inactive;
    GPSCorrectionStream _selected;
    quint64 _selectedBytesPerSecond = 0;
    QString _udpInputError;
    QString _udpOutputError;
    /// The message IDs of a valid filter; empty takes every message.
    QSet<int> _messageFilter;
    /// Names the filter's entry that is not a message ID.
    QString _messageFilterInvalid;
    /// The messages a valid filter left out of the sources the routing selects from while no stream was selected, and
    /// when the latest arrived.
    QSet<int> _leftOutMessages;
    qint64 _leftOutAtMs = 0;
    QString _messageFilterError;
    RTCMMessageCountModel _messageCounts{this};
    QGeoCoordinate _basePosition;
    /// The stream _messageCounts and _basePosition describe. _selected follows it only at the next refresh.
    GPSCorrectionStream _recordedStream;
    bool _shutdown = false;
};
