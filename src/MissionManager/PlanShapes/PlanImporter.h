#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtPositioning/QGeoCoordinate>

#include <mutex>

Q_DECLARE_LOGGING_CATEGORY(PlanImporterLog)

// Forward declaration
namespace GeoFormatRegistry { struct LoadResult; }

/// Result of importing a plan file
struct PlanImportResult {
    bool success = false;
    QString errorString;

    /// Imported waypoints (for simple imports)
    QList<QGeoCoordinate> waypoints;

    /// Waypoint names (optional, same order as waypoints)
    QStringList waypointNames;

    /// Flight path/track points (for tracks/routes)
    QList<QGeoCoordinate> trackPoints;

    /// Survey area polygons (for polygon imports)
    QList<QList<QGeoCoordinate>> polygons;

    /// Source format description
    QString formatDescription;

    /// Number of items imported
    int itemCount() const {
        return waypoints.count() + trackPoints.count() + polygons.count();
    }

    /// Check if any features were imported
    bool hasFeatures() const { return itemCount() > 0; }

    /// Check if waypoints were imported
    bool hasWaypoints() const { return !waypoints.isEmpty(); }

    /// Check if track points were imported
    bool hasTrackPoints() const { return !trackPoints.isEmpty(); }

    /// Check if polygons were imported
    bool hasPolygons() const { return !polygons.isEmpty(); }
};

/// Abstract base class for plan importers
class PlanImporter : public QObject
{
    Q_OBJECT

public:
    explicit PlanImporter(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~PlanImporter() = default;

    /// Import a plan from a file
    /// @param filename Input file path
    /// @return Import result with coordinates and any error info
    virtual PlanImportResult importFromFile(const QString& filename) = 0;

    /// Set vertex filter distance in meters
    /// @param meters Filter vertices closer than this distance (0 to disable)
    void setVertexFilterMeters(double meters) { _vertexFilterMeters = meters; }

    /// Get current vertex filter distance
    double vertexFilterMeters() const { return _vertexFilterMeters; }

    /// Returns the file extension for this importer (without dot)
    virtual QString fileExtension() const = 0;

    /// Returns a human-readable name for this format
    virtual QString formatName() const = 0;

    /// Returns the file filter string for file dialogs
    virtual QString fileFilter() const = 0;

    // ========================================================================
    // Static factory methods
    // ========================================================================

    /// Register an importer for a file extension
    static void registerImporter(const QString& extension, PlanImporter* importer);

protected:
    // ========================================================================
    // Helper methods for subclasses
    // ========================================================================

    /// Import using GeoFormatRegistry and convert result
    /// @param filename File to import
    /// @param formatName Human-readable format name for error/description messages
    /// @return Converted PlanImportResult
    PlanImportResult importWithRegistry(const QString& filename, const QString& formatName);

public:

    /// Get an importer for the given file extension (returns nullptr if none registered)
    static PlanImporter* importerForExtension(const QString& extension);

    /// Get an importer for the given filename (based on extension)
    static PlanImporter* importerForFile(const QString& filename);

    /// Returns list of all registered file extensions
    static QStringList registeredExtensions();

    /// Returns combined file filter string for all importers
    static QStringList fileDialogFilters();

    /// Initialize all built-in importers
    static void initializeImporters();

private:
    static QHash<QString, PlanImporter*> s_importers;

    double _vertexFilterMeters = 0.0;
};

/// Macro to declare singleton instance method and static member for PlanImporter subclasses
/// Usage in header: DECLARE_PLAN_IMPORTER_SINGLETON(ClassName)
#define DECLARE_PLAN_IMPORTER_SINGLETON(ClassName) \
public: \
    static ClassName* instance(); \
private: \
    static ClassName* s_instance; \
    static std::once_flag s_onceFlag;

/// Macro to implement singleton instance method for PlanImporter subclasses
/// Usage in source: IMPLEMENT_PLAN_IMPORTER_SINGLETON(ClassName)
#define IMPLEMENT_PLAN_IMPORTER_SINGLETON(ClassName) \
    ClassName* ClassName::s_instance = nullptr; \
    std::once_flag ClassName::s_onceFlag; \
    ClassName* ClassName::instance() { \
        std::call_once(s_onceFlag, []() { \
            s_instance = new ClassName(); \
            PlanImporter::registerImporter(s_instance->fileExtension(), s_instance); \
        }); \
        return s_instance; \
    }
