/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#pragma once

#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>

class LinkInterface;
class Vehicle;
class QmlObjectListModel;
class QTimer;

Q_DECLARE_LOGGING_CATEGORY(MultiVehicleManagerLog)

class MultiVehicleManager : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_MOC_INCLUDE("LinkInterface.h")
    Q_MOC_INCLUDE("Vehicle.h")
    Q_PROPERTY(bool                 activeVehicleAvailable          READ _getActiveVehicleAvailable                                         NOTIFY activeVehicleAvailableChanged)
    Q_PROPERTY(bool                 parameterReadyVehicleAvailable  READ _getParameterReadyVehicleAvailable                                 NOTIFY parameterReadyVehicleAvailableChanged)
    Q_PROPERTY(Vehicle              *activeVehicle                  READ activeVehicle                      WRITE setActiveVehicle          NOTIFY activeVehicleChanged)
    Q_PROPERTY(QmlObjectListModel   *vehicles                       READ vehicles                                                           CONSTANT)
    Q_PROPERTY(QmlObjectListModel   *selectedVehicles               READ selectedVehicles                                                   CONSTANT)
    Q_PROPERTY(Vehicle              *offlineEditingVehicle          READ offlineEditingVehicle                                              CONSTANT)

public:
    explicit MultiVehicleManager(QObject *parent = nullptr);
    ~MultiVehicleManager();

    static MultiVehicleManager *instance();
    static void registerQmlTypes();

    void init();
    Q_INVOKABLE Vehicle *getVehicleById(int vehicleId) const;
    Q_INVOKABLE void      selectVehicle(int vehicleId);
    Q_INVOKABLE void    deselectVehicle(int vehicleId);
    Q_INVOKABLE void    deselectAllVehicles();
    QmlObjectListModel *vehicles() const { return _vehicles; }
    QmlObjectListModel *selectedVehicles() const { return _selectedVehicles; }
    Vehicle *offlineEditingVehicle() const { return _offlineEditingVehicle; }
    Vehicle *activeVehicle() const { return _activeVehicle; }
    void setActiveVehicle(Vehicle *vehicle);

signals:
    void vehicleAdded(Vehicle *vehicle);
    void vehicleRemoved(Vehicle *vehicle);
    void activeVehicleAvailableChanged(bool activeVehicleAvailable);
    void parameterReadyVehicleAvailableChanged(bool parameterReadyVehicleAvailable);
    void activeVehicleChanged(Vehicle *activeVehicle);

private slots:
    void _deleteVehiclePhase1(Vehicle *vehicle); /// This slot is connected to the Vehicle::allLinksDestroyed signal such that the Vehicle is deleted and all other right things happen when the Vehicle goes away.
    void _deleteVehiclePhase2(Vehicle *vehicle);
    void _setActiveVehiclePhase2(Vehicle *vehicle);
    void _vehicleParametersReadyChanged(bool parametersReady);
    void _sendGCSHeartbeat();
    void _vehicleHeartbeatInfo(LinkInterface *link, int vehicleId, int componentId, int vehicleFirmwareType, int vehicleType);
    void _requestProtocolVersion(unsigned version) const; /// This slot is connected to the Vehicle::requestProtocolVersion signal such that the vehicle manager tries to switch MAVLink to v2 if all vehicles support it

private:
    bool _vehicleExists(int vehicleId);
    bool _vehicleSelected(int vehicleId);
    void _setActiveVehicle(Vehicle *vehicle);
    bool _getActiveVehicleAvailable() const { return _activeVehicleAvailable; }
    void _setActiveVehicleAvailable(bool activeVehicleAvailable);
    bool _getParameterReadyVehicleAvailable() const { return _parameterReadyVehicleAvailable; }
    void _setParameterReadyVehicleAvailable(bool parametersReady);

    QTimer *_gcsHeartbeatTimer = nullptr;           ///< Timer to emit heartbeats
    Vehicle *_offlineEditingVehicle = nullptr;      ///< Disconnected vechicle used for offline editing
    QmlObjectListModel *_vehicles = nullptr;
    QmlObjectListModel *_selectedVehicles = nullptr;
    bool _activeVehicleAvailable = false;           ///< true: An active vehicle is available
    bool _parameterReadyVehicleAvailable = false;   ///< true: An active vehicle with ready parameters is available
    Vehicle *_activeVehicle = nullptr;              ///< Currently active vehicle from a ui perspective
    QList<int> _ignoreVehicleIds;                   ///< List of vehicle id for which we ignore further communication
    bool _initialized = false;

    static constexpr int kGCSHeartbeatRateMSecs = 1000;  ///< Heartbeat rate
};
