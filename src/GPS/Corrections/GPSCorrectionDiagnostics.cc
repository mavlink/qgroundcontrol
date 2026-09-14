#include "GPSCorrectionDiagnostics.h"

GPSCorrectionReason gpsCorrectionReason(GPSCorrectionOutcome outcome)
{
    switch (outcome) {
        case GPSCorrectionOutcome::Written:
            return GPSCorrectionReason::None;
        case GPSCorrectionOutcome::WriteFailed:
            return GPSCorrectionReason::WriteFailed;
        case GPSCorrectionOutcome::Expired:
            return GPSCorrectionReason::Expired;
        case GPSCorrectionOutcome::Cancelled:
            return GPSCorrectionReason::Cancelled;
        case GPSCorrectionOutcome::Cleared:
            return GPSCorrectionReason::SourceChanged;
        case GPSCorrectionOutcome::NotReady:
            return GPSCorrectionReason::DestinationUnavailable;
        case GPSCorrectionOutcome::InvalidData:
            return GPSCorrectionReason::InvalidFrame;
        case GPSCorrectionOutcome::Overflow:
            return GPSCorrectionReason::QueueFull;
    }
    return GPSCorrectionReason::InvalidDelivery;
}
