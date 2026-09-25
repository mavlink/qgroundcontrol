#pragma once

#include <QtCore/QRangeModel>

#include "GPSCorrectionDiagnostics.h"

template <>
struct QRangeModel::RowOptions<GPSCorrectionEvent>
{
    static constexpr auto rowCategory = QRangeModel::RowCategory::MultiRoleItem;
};

/// A bounded metadata-only history, refreshed in batches by the application facade.
class GPSCorrectionEventModel : public QRangeModel
{
    Q_OBJECT

public:
    enum Stage
    {
        Received = static_cast<int>(GPSCorrectionStage::Received),
        Validated = static_cast<int>(GPSCorrectionStage::Validated),
        Selected = static_cast<int>(GPSCorrectionStage::Selected),
        Queued = static_cast<int>(GPSCorrectionStage::Queued),
        Dropped = static_cast<int>(GPSCorrectionStage::Dropped),
    };
    Q_ENUM(Stage)

    enum Reason
    {
        None = static_cast<int>(GPSCorrectionReason::None),
        InvalidTimestamp = static_cast<int>(GPSCorrectionReason::InvalidTimestamp),
        Expired = static_cast<int>(GPSCorrectionReason::Expired),
        MessageFiltered = static_cast<int>(GPSCorrectionReason::MessageFiltered),
        NotSelected = static_cast<int>(GPSCorrectionReason::NotSelected),
        DestinationUnavailable = static_cast<int>(GPSCorrectionReason::DestinationUnavailable),
        InvalidFrame = static_cast<int>(GPSCorrectionReason::InvalidFrame),
    };
    Q_ENUM(Reason)

    /// Roles follow the GPSCorrectionEvent property order, as QRangeModel assigns them.
    enum Role
    {
        EventSequenceRole = Qt::UserRole,
        TimestampMsRole,
        SourceRole,
        SourceInstanceRole,
        SourceSessionRole,
        DestinationIdRole,
        DestinationSessionRole,
        StageRole,
        ReasonRole,
        BytesRole,
    };
    Q_ENUM(Role)

    explicit GPSCorrectionEventModel(QObject* parent = nullptr);
    ~GPSCorrectionEventModel() override;
    /// Removes events that left the front of the history and appends new ones, without resetting the model.
    void setEvents(const QList<GPSCorrectionEvent>& events);

private:
    QList<GPSCorrectionEvent> _events;
    QList<GPSCorrectionEvent> _pendingEvents;
    bool _updating = false;
    bool _pending = false;
};
