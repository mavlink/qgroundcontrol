#pragma once

#include <functional>

#include "GPSPositionService.h"

class SimulatedPosition;

/// The application supplies configuration and source creation; the manager requests location permission and binds
/// the platform or simulated source.
class PositionManager : public GPSPositionService
{
    Q_OBJECT
    Q_MOC_INCLUDE("SimulatedPosition.h")

public:
    /// Creates a position source parented to @a parent, or returns null.
    using SourceFactory = std::function<QGeoPositionInfoSource*(QObject* parent)>;

    struct Configuration
    {
        SourceMode sourceMode = SourceMode::Automatic;
        bool operator==(const Configuration&) const = default;
    };

    explicit PositionManager(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~PositionManager() override;

    /// Ignored after shutdown().
    void setConfiguration(const Configuration& configuration);

    /// Creates the platform source once location permission is granted; without a factory, or when it returns null,
    /// Qt's default source is used.
    void setPlatformSourceFactory(SourceFactory factory);

    /// Makes init() publish a SimulatedPosition instead of requesting location permission for the platform source.
    void setSimulated(bool simulated);

    void init();
    /// Releases the position sources and ignores later configuration; init() does not restart them.
    void shutdown();

signals:
    /// init() created @a source, which the application may steer with SimulatedPosition::setReferencePosition().
    void simulatedPositionCreated(SimulatedPosition* source);

private:
    void _setupPositionSources();
    void _handlePermissionStatus(Qt::PermissionStatus permissionStatus);
    void _checkPermission();
    SourceFactory _platformSourceFactory;
    bool _simulated = false;
    bool _shutdown = false;
    bool _destroying = false;
};
