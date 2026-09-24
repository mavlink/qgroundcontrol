#pragma once

#include <QtCore/QAbstractListModel>

#include "GPSCorrectionDiagnostics.h"

/// A bounded metadata-only history, refreshed in batches by the application facade.
class GPSCorrectionEventModel : public QAbstractListModel
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
        InactiveSource = static_cast<int>(GPSCorrectionReason::InactiveSource),
        SessionMismatch = static_cast<int>(GPSCorrectionReason::SessionMismatch),
        InvalidTimestamp = static_cast<int>(GPSCorrectionReason::InvalidTimestamp),
        Expired = static_cast<int>(GPSCorrectionReason::Expired),
        MessageFiltered = static_cast<int>(GPSCorrectionReason::MessageFiltered),
        NotSelected = static_cast<int>(GPSCorrectionReason::NotSelected),
        DestinationUnavailable = static_cast<int>(GPSCorrectionReason::DestinationUnavailable),
        QueueFull = static_cast<int>(GPSCorrectionReason::QueueFull),
        InvalidFrame = static_cast<int>(GPSCorrectionReason::InvalidFrame),
    };
    Q_ENUM(Reason)

    enum Role
    {
        EventSequenceRole = Qt::UserRole + 1,
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
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    void setEvents(const QList<GPSCorrectionEvent>& events);

private:
    QList<GPSCorrectionEvent> _events;
    QList<GPSCorrectionEvent> _pendingEvents;
    bool _updating = false;
    bool _pending = false;
};
