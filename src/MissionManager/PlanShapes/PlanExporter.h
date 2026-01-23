#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>

#include <memory>
#include <mutex>

Q_DECLARE_LOGGING_CATEGORY(PlanExporterLog)

class MissionController;
class MissionItem;
class QmlObjectListModel;
class Vehicle;

/// Abstract base class for plan exporters
class PlanExporter : public QObject
{
    Q_OBJECT

public:
    explicit PlanExporter(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~PlanExporter() = default;

    /// Export the plan to the specified file
    /// @param filename Output file path
    /// @param missionController Mission controller containing the plan data
    /// @param errorString Output parameter for error message on failure
    /// @return true on success
    virtual bool exportToFile(const QString& filename,
                              MissionController* missionController,
                              QString& errorString) = 0;

    /// Returns the file extension for this exporter (without dot)
    virtual QString fileExtension() const = 0;

    /// Returns a human-readable name for this format
    virtual QString formatName() const = 0;

    /// Returns the file filter string for file dialogs
    virtual QString fileFilter() const = 0;

    // ========================================================================
    // Static factory methods
    // ========================================================================

    /// Register an exporter for a file extension
    static void registerExporter(const QString& extension, PlanExporter* exporter);

    /// Get an exporter for the given file extension (returns nullptr if none registered)
    static PlanExporter* exporterForExtension(const QString& extension);

    /// Get an exporter for the given filename (based on extension)
    static PlanExporter* exporterForFile(const QString& filename);

    /// Returns list of all registered file extensions
    static QStringList registeredExtensions();

    /// Returns combined file filter string for all exporters
    static QStringList fileDialogFilters();

    /// Initialize all built-in exporters
    static void initializeExporters();

protected:
    /// Data prepared for export by prepareMissionData()
    struct ExportData {
        Vehicle* vehicle = nullptr;
        QmlObjectListModel* visualItems = nullptr;
        QList<MissionItem*> missionItems;
        std::unique_ptr<QObject> itemParent;  // Parent object for mission items lifetime
        bool valid = false;
    };

    /// Prepare mission data for export - validates inputs and converts mission items
    /// @param missionController The mission controller to export from
    /// @param errorString Output parameter for error message on failure
    /// @return ExportData struct with valid=true on success, or valid=false with errorString set
    ExportData prepareMissionData(MissionController* missionController, QString& errorString);

private:
    static QHash<QString, PlanExporter*> s_exporters;
    static bool s_initialized;
};

/// Macro to declare singleton instance method and static member for PlanExporter subclasses
/// Usage in header: DECLARE_PLAN_EXPORTER_SINGLETON(ClassName)
#define DECLARE_PLAN_EXPORTER_SINGLETON(ClassName) \
public: \
    static ClassName* instance(); \
private: \
    static ClassName* s_instance; \
    static std::once_flag s_onceFlag;

/// Macro to implement singleton instance method for PlanExporter subclasses
/// Usage in source: IMPLEMENT_PLAN_EXPORTER_SINGLETON(ClassName)
#define IMPLEMENT_PLAN_EXPORTER_SINGLETON(ClassName) \
    ClassName* ClassName::s_instance = nullptr; \
    std::once_flag ClassName::s_onceFlag; \
    ClassName* ClassName::instance() { \
        std::call_once(s_onceFlag, []() { \
            s_instance = new ClassName(); \
            PlanExporter::registerExporter(s_instance->fileExtension(), s_instance); \
        }); \
        return s_instance; \
    }
