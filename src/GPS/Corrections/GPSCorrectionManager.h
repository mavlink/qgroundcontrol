#pragma once

#include <memory>
#include <optional>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "DataRateTracker.h"
#include "GPSCorrectionSelector.h"
#include "RTCMMAVLink.h"
#include "RTCMUdpInput.h"
#include "ScheduledTask.h"
#include "UdpForwarder.h"

class GPSCorrectionManager;
class RuntimeScheduler;

/// A begun correction source and the stream it delivers; destroying it ends the source.
class GPSCorrectionSourceHandle
{
public:
    ~GPSCorrectionSourceHandle();
    Q_DISABLE_COPY_MOVE(GPSCorrectionSourceHandle)

    /// Offers one valid RTCM frame of the source's stream, received at @a receivedAtMs on the scheduler's clock. Fresh
    /// frames of the selected stream go to vehicles and UDP forwarding. A non-empty @a instance names the frame's
    /// stream instead, for a source that names streams per frame.
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
/// stream goes to vehicles over MAVLink and to UDP forwarding. Receipt times are on the scheduler's clock; producers
/// on worker threads stamp with the steady clock, which the application's QtRuntimeScheduler uses.
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
        /// Listens for RTCM on a UDP port as the Udp source. A port that cannot be bound is retried every second.
        UdpInputConfiguration udpInput;
        /// Forwards the selected stream to a UDP peer.
        UdpOutputConfiguration udpOutput;
        bool operator==(const Configuration&) const = default;
    };

    explicit GPSCorrectionManager(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
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

    Q_INVOKABLE static QString sourceName(int source);

signals:
    void stateChanged();
    void selectedBytesPerSecondChanged();
    void vehicleBytesSubmittedChanged();
    void udpInputChanged();
    void udpOutputChanged();

private:
    /// See GPSCorrectionSelector::beginSource(); each source calls _endSource() before it is replaced or stops.
    void _beginSource(GPSCorrectionSettings::CorrectionSource source);
    void _endSource(GPSCorrectionSettings::CorrectionSource source);
    /// See GPSCorrectionSelector::submit(); a selected frame goes to vehicles and UDP forwarding.
    void _submit(GPSCorrectionSettings::CorrectionSource source, const QString& instance, const QByteArray& data,
                 qint64 receivedAtMs);

    /// Each returns whether the input's state or the output's error changed.
    [[nodiscard]] bool _applyUdpInput();
    [[nodiscard]] bool _applyUdpOutput();
    bool _udpInputEnabled() const;
    void _startUdpInput();
    void _retryUdpInput();

    void _scheduleRefresh();
    void _refresh();

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
    std::optional<Configuration> _configuration;
    State _state = State::Inactive;
    GPSCorrectionStream _selected;
    quint64 _selectedBytesPerSecond = 0;
    QString _udpInputError;
    QString _udpOutputError;
    bool _shutdown = false;
};
