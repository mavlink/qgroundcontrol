#pragma once

#include <QtCore/QAbstractListModel>
#include <QtQmlIntegration/QtQmlIntegration>

// Full include (not forward decl) required because streamForRole is
// Q_INVOKABLE and returns VideoStream* — Qt 6.10 meta-type registration
// rejects pointer types without either a complete definition here or
// Q_DECLARE_OPAQUE_POINTER(T*).
#include "VideoStream.h"

/// List model exposing active video streams to QML.
///
/// Each row represents a VideoStream (stable for its lifetime) with roles
/// for name, URI, thermal flag, frame delivery, and active status. This eliminates
/// the dangling-pointer bug from the old VideoReceiver*-based model — receivers
/// are created and destroyed underneath the stream during receiver rebuilds,
/// but the model row is always valid.
class VideoStreamModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum Role
    {
        NameRole = Qt::UserRole + 1,
        UriRole,
        IsThermalRole,
        IsActiveRole,
        FrameDeliveryRole,
        LastErrorRole,  ///< Per-stream error string (empty on success).
        StreamRole,     ///< The VideoStream * itself, for QML direct-binding.
    };
    Q_ENUM(Role)

    explicit VideoStreamModel(QObject* parent = nullptr);

    /// Called by VideoManager when the UVC active state changes so that
    /// activeStreamForRole(Primary) returns the correct stream.
    /// Emits activeStreamChanged(Primary) to trigger QML re-binding.
    void setUvcActive(bool active);

    // ── QAbstractListModel interface ──────────────────────────────────
    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    // ── Mutation API (called by VideoManager) ─────────────────────────
    void addStream(VideoStream* stream);
    void removeStream(VideoStream* stream);

    /// Find a stream by name (returns nullptr if not found). Q_INVOKABLE so
    /// QML can resolve Dynamic streams by name — multiple streams share
    /// Role::Dynamic, so `streamForRole(Dynamic)` is ambiguous.
    Q_INVOKABLE VideoStream* stream(const QString& name) const;

    /// Find a stream by role. Preferred over stream(QString) — avoids
    /// string-typed role lookups and gives QML a direct handle to bind to
    /// `VideoStream.frameDelivery` / `VideoStream.lastError` etc.
    /// Exposed via Q_INVOKABLE so QML can bind a VideoSinkBinder to the
    /// stream object and let C++ own sink registration teardown.
    Q_INVOKABLE VideoStream* streamForRole(int role) const;

    /// Returns the stream currently playing the given logical role.
    /// For VideoStream::Role::Primary, returns the UVC stream when the active
    /// video source is UVC, otherwise returns the Primary stream.
    /// For other roles, delegates to streamForRole().
    Q_INVOKABLE VideoStream* activeStreamForRole(int role) const;

    /// All streams in insertion order.
    [[nodiscard]] const QList<VideoStream*>& streams() const { return _streams; }

    /// Count of active (decoding) streams.
    Q_INVOKABLE int activeCount() const;

signals:
    /// Emitted when the effective stream for a given role changes (e.g. when
    /// the video source switches between UVC and network). QML consumers
    /// should re-query activeStreamForRole() and re-register their sink.
    void activeStreamChanged(int role);

private:
    void _onStreamChanged(VideoStream* stream, const QList<int>& roles);
    int _indexOf(VideoStream* stream) const;

    QList<VideoStream*> _streams;
    bool _uvcActive = false;
};
