#pragma once

#include <optional>
#include <utility>

#include "GPSConfigurationReport.h"
#include "GPSConnectionError.h"

/// A failed receiver attempt retains the operation evidence that determines its retry policy.
struct GPSReceiverFailure
{
    enum class Stage
    {
        Open,
        Configure,
        Receive
    };
    enum class Cause
    {
        Unsupported,
        Cancelled,
        TimedOut,
        Transport,
        UncertainWrite,
        Rejected
    };

    quint64 generation = 0;
    Stage stage = Stage::Open;
    Cause cause = Cause::Transport;
    GPSConnectionError error = GPSConnectionError::OpenFailed;
    GPSRetryDisposition retry = GPSRetryDisposition::Retry;
    QString detail;
    std::optional<GPSOpenResult> transportOpen;
    std::optional<GPSConfigurationResult> configuration;
    std::optional<GPSReadResult> transportRead;

    static GPSReceiverFailure from(quint64 generation, GPSConnectionError error, const QString& detail,
                                   std::optional<GPSOpenResult> opened = {},
                                   std::optional<GPSConfigurationResult> configured = {},
                                   std::optional<GPSReadResult> read = {})
    {
        GPSReceiverFailure failure;
        failure.generation = generation;
        failure.error = error;
        failure.detail = detail;
        failure.transportOpen = std::move(opened);
        failure.configuration = std::move(configured);
        failure.transportRead = std::move(read);
        if (error == GPSConnectionError::ConfigFailed) {
            failure.stage = Stage::Configure;
            failure.cause = Cause::Rejected;
            if (failure.configuration) {
                switch (failure.configuration->status) {
                    case GPSConfigurationStatus::Unsupported:
                        failure.cause = Cause::Unsupported;
                        break;
                    case GPSConfigurationStatus::Cancelled:
                        failure.cause = Cause::Cancelled;
                        break;
                    case GPSConfigurationStatus::TransportError:
                        failure.cause = Cause::Transport;
                        break;
                    case GPSConfigurationStatus::Failed:
                    case GPSConfigurationStatus::NotConfigured:
                    case GPSConfigurationStatus::Ready:
                        break;
                }
                if ((failure.cause == Cause::Rejected || failure.cause == Cause::Transport) &&
                    failure.configuration->transportRead &&
                    failure.configuration->transportRead->status == GPSReadStatus::TimedOut)
                    failure.cause = Cause::TimedOut;
                if ((failure.cause == Cause::Rejected || failure.cause == Cause::Transport ||
                     failure.cause == Cause::TimedOut) &&
                    failure.configuration->transportWrite) {
                    const auto& write = *failure.configuration->transportWrite;
                    if (write.status == GPSWriteStatus::Cancelled)
                        failure.cause = Cause::Cancelled;
                    else if (write.uncertainBytes > 0)
                        failure.cause = Cause::UncertainWrite;
                    else if (write.status == GPSWriteStatus::TimedOut)
                        failure.cause = Cause::TimedOut;
                    else if (write.status == GPSWriteStatus::Unsupported)
                        failure.cause = Cause::Unsupported;
                }
            }
        } else if (error == GPSConnectionError::DeviceError) {
            failure.stage = Stage::Receive;
            if (failure.transportRead && failure.transportRead->status == GPSReadStatus::Cancelled)
                failure.cause = Cause::Cancelled;
            else if (failure.transportRead && failure.transportRead->status == GPSReadStatus::TimedOut)
                failure.cause = Cause::TimedOut;
        } else if (failure.transportOpen) {
            switch (failure.transportOpen->status) {
                case GPSOpenStatus::Unsupported:
                    failure.cause = Cause::Unsupported;
                    break;
                case GPSOpenStatus::Cancelled:
                    failure.cause = Cause::Cancelled;
                    break;
                case GPSOpenStatus::TimedOut:
                    failure.cause = Cause::TimedOut;
                    break;
                case GPSOpenStatus::Opened:
                case GPSOpenStatus::Error:
                    break;
            }
        }
        failure.retry = failure.cause == Cause::Unsupported ? GPSRetryDisposition::AwaitChange
                        : failure.cause == Cause::Cancelled ? GPSRetryDisposition::Cancel
                                                            : GPSRetryDisposition::Retry;
        return failure;
    }
};
Q_DECLARE_METATYPE(GPSReceiverFailure)
