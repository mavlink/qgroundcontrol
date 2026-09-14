#include "ExclusionZoneController.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtPositioning/QGeoCoordinate>

#include "AppSettings.h"
#include "GeoFenceManager.h"
#include "QGCFencePolygon.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "ShapeFileHelper.h"
#include "StagedExclusionZone.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(ExclusionZoneLog, "Custom.ExclusionZone")

ExclusionZoneController::ExclusionZoneController(QObject* parent)
    : QObject(parent), _stagedZones(new QmlObjectListModel(this))
{}

ExclusionZoneController::~ExclusionZoneController() {}

void ExclusionZoneController::setTargetVehicle(Vehicle* vehicle)
{
    if (_targetVehicle != vehicle) {
        _targetVehicle = vehicle;
        emit targetVehicleChanged(_targetVehicle);
    }
}

int ExclusionZoneController::approvedCount() const
{
    int count = 0;
    for (int i = 0; i < _stagedZones->count(); i++) {
        if (qobject_cast<StagedExclusionZone*>(_stagedZones->get(i))->approved()) {
            count++;
        }
    }
    return count;
}

bool ExclusionZoneController::importFromFile(const QString& file)
{
    QList<QList<QGeoCoordinate>> polygons;
    QString errorString;
    if (!ShapeFileHelper::loadPolygonsFromFile(file, polygons, errorString)) {
        qCWarning(ExclusionZoneLog) << "loadPolygonsFromFile failed:" << errorString;
        return false;
    }
    if (polygons.isEmpty()) {
        qCWarning(ExclusionZoneLog) << "No polygons found in file:" << file;
        return false;
    }

    for (const QList<QGeoCoordinate>& vertices : polygons) {
        auto* polygon = new QGCFencePolygon(false /* inclusion */, this);
        polygon->appendVertices(vertices);
        auto* zone = new StagedExclusionZone(polygon, this);
        connect(zone, &StagedExclusionZone::approvedChanged, this, &ExclusionZoneController::approvedCountChanged);
        _stagedZones->append(zone);
    }

    emit approvedCountChanged();
    return true;
}

void ExclusionZoneController::setApproved(int index, bool approved)
{
    if (auto* zone = qobject_cast<StagedExclusionZone*>(_stagedZones->get(index))) {
        zone->setApproved(approved);
    }
}

bool ExclusionZoneController::pushApproved()
{
    if (!_targetVehicle) {
        emit pushFinished(false, tr("No target vehicle selected."));
        return false;
    }

    QList<StagedExclusionZone*> approvedZones;
    for (int i = 0; i < _stagedZones->count(); i++) {
        auto* zone = qobject_cast<StagedExclusionZone*>(_stagedZones->get(i));
        if (zone && zone->approved()) {
            approvedZones.append(zone);
        }
    }
    if (approvedZones.isEmpty()) {
        emit pushFinished(false, tr("No approved zones to push."));
        return false;
    }

    GeoFenceManager* fenceMgr = _targetVehicle->geoFenceManager();
    if (!fenceMgr) {
        emit pushFinished(false, tr("Target vehicle has no geofence support."));
        return false;
    }
    if (fenceMgr->inProgress()) {
        emit pushFinished(false, tr("A geofence sync is already in progress on this vehicle."));
        return false;
    }

    // 1. Build the audit JSON using the same per-item save method GeoFenceController::save()
    // itself delegates to for each polygon - same schema, no live GeoFenceController required.
    QJsonArray polygonArray;
    for (StagedExclusionZone* zone : approvedZones) {
        QJsonObject polygonJson;
        zone->polygon()->saveToJson(polygonJson);
        polygonArray.append(polygonJson);
    }
    QJsonObject fenceJson;
    fenceJson[QStringLiteral("version")] = 2;
    fenceJson[QStringLiteral("polygons")] = polygonArray;
    fenceJson[QStringLiteral("circles")] = QJsonArray();

    // 2. Write the audit-trail file.
    const QString dirPath =
        SettingsManager::instance()->appSettings()->missionSavePath() + QStringLiteral("/ExclusionZoneApprovals");
    QDir().mkpath(dirPath);
    const QString filePath =
        dirPath + QStringLiteral("/exclusion-zone-approval-%1.json")
                      .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss")));
    QFile auditFile(filePath);
    if (!auditFile.open(QIODevice::WriteOnly)) {
        qCWarning(ExclusionZoneLog) << "Failed to open audit file for write:" << filePath;
        emit pushFinished(false, tr("Failed to write audit file: %1").arg(filePath));
        return false;
    }
    auditFile.write(QJsonDocument(fenceJson).toJson());
    auditFile.close();
    qCDebug(ExclusionZoneLog) << "Wrote exclusion-zone audit file:" << filePath;

    // 3. Round-trip: re-parse the just-written JSON into fresh polygons, proving the audit file
    // is faithfully reconstructable rather than trusting the in-memory staged objects.
    auto* sendPolygons = new QmlObjectListModel(this);
    auto* sendCircles = new QmlObjectListModel(this);
    QString loadError;
    for (const QJsonValue& value : polygonArray) {
        auto* polygon = new QGCFencePolygon(false, sendPolygons);
        if (!polygon->loadFromJson(value.toObject(), true, loadError)) {
            qCWarning(ExclusionZoneLog) << "Audit round-trip failed:" << loadError;
            sendPolygons->deleteLater();
            sendCircles->deleteLater();
            emit pushFinished(false, tr("Audit round-trip failed: %1").arg(loadError));
            return false;
        }
        sendPolygons->append(polygon);
    }

    // 4. Send directly through the vehicle's own permanent GeoFenceManager - not through a
    // throwaway PlanMasterController/GeoFenceController. sendToVehicle() copies every
    // polygon/circle by value synchronously before the async MAVLink exchange starts, so
    // sendPolygons/sendCircles don't need to outlive this call.
    disconnect(_fenceErrorConnection);
    disconnect(_fenceSendCompleteConnection);
    _fenceErrorConnection =
        connect(fenceMgr, &GeoFenceManager::error, this, [this](int, const QString& msg) { _lastFenceError = msg; });
    _fenceSendCompleteConnection =
        connect(fenceMgr, &GeoFenceManager::sendComplete, this, [this, approvedZones](bool error) {
            if (!error) {
                for (StagedExclusionZone* zone : approvedZones) {
                    _stagedZones->removeOne(zone);
                    zone->deleteLater();
                }
                emit approvedCountChanged();
            }
            emit pushFinished(!error, error ? _lastFenceError : QString());
        });

    fenceMgr->sendToVehicle(QGeoCoordinate() /* no breach return for exclusion-only push */, *sendPolygons,
                            *sendCircles);

    sendPolygons->deleteLater();
    sendCircles->deleteLater();

    return true;
}
