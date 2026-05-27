# JIACDIGCS Swarm Architecture

## Overview

JIACDIGCS introduces professional swarm management capabilities for multi-UAV operations, transforming QGroundControl into a comprehensive swarm command and control platform.

## Swarm System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         JIACDIGCS UI Layer                       │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌────────┐ │
│  │  FlyView    │  │  PlanView   │  │   Swarm     │  │Settings│ │
│  └─────────────┘  └─────────────┘  │  Interface  │  └────────┘ │
│                                   └─────────────┘              │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      QML Component Layer                         │
│  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐   │
│  │  Swarm     │ │  Swarm     │ │  Swarm     │ │  Swarm     │   │
│  │  Control   │ │  Telemetry │ │  Alert     │ │  Formation │   │
│  │  Panel     │ │  Widget    │ │  System    │ │  Selector  │   │
│  └────────────┘ └────────────┘ └────────────┘ └────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    C++ Core Logic Layer                          │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                   SwarmManager                             │ │
│  │  - Vehicle Management                                      │ │
│  │  - Formation Coordination                                   │ │
│  │  - Synchronized Commands                                   │ │
│  │  - Health Monitoring                                       │ │
│  │  - Collision Detection                                     │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│              ┌───────────────┴───────────────┐                 │
│              ▼                               ▼                 │
│  ┌─────────────────────┐     ┌─────────────────────┐          │
│  │  MultiVehicleManager │     │    Vehicle Class   │          │
│  │  (Existing QGC)      │◄────│    (Extended)      │          │
│  └─────────────────────┘     └─────────────────────┘          │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     MAVLink Protocol Layer                       │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐          │
│  │ HEARTBEAT│ │  COMMAND │ │  MISSION │ │  HIL_GPS │          │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘          │
└─────────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. SwarmManager (src/Swarm/SwarmManager.h/cc)

The central orchestrator for all swarm operations.

**Key Features:**
- Singleton pattern for global access
- Vehicle registration and management
- Formation calculation algorithms
- Synchronized command execution
- Health monitoring and alerts
- Collision detection

**Public Interface:**
```cpp
// Vehicle management
void addVehicle(Vehicle* vehicle);
void removeVehicle(Vehicle* vehicle);
Vehicle* getVehicleById(int vehicleId);

// Formation control
void setCurrentFormation(SwarmFormation formation);
void applyFormationOffsets();
void lockFormation();

// Synchronized commands
void synchronizedTakeoff(double altitude);
void synchronizedLand();
void synchronizedRTL();
void emergencyStopAll();

// Health monitoring
QVariantMap getSwarmHealthStatus() const;
bool checkCollisionRisk() const;
```

### 2. QML Swarm Components

| Component | Location | Purpose |
|-----------|----------|---------|
| `SwarmInterface.qml` | src/Swarm/QmlControls/ | Main dashboard container |
| `SwarmControlPanel.qml` | src/Swarm/QmlControls/ | Command toolbar |
| `SwarmDashboard.qml` | src/Swarm/QmlControls/ | Fleet overview card |
| `SwarmVehicleStatus.qml` | src/Swarm/QmlControls/ | Individual vehicle display |
| `SwarmVehicleList.qml` | src/Swarm/QmlControls/ | Scrollable vehicle list |
| `SwarmFormationSelector.qml` | src/Swarm/QmlControls/ | Formation type picker |
| `SwarmTelemetryWidget.qml` | src/Swarm/QmlControls/ | Telemetry charts |
| `SwarmAlertSystem.qml` | src/Swarm/QmlControls/ | Alert management |
| `SwarmHealthIndicator.qml` | src/Swarm/QmlControls/ | Health status display |
| `SwarmMiniMap.qml` | src/Swarm/QmlControls/ | Mini map visualization |
| `SwarmMapVisualization.qml` | src/Swarm/QmlControls/ | Full map canvas |

### 3. Formation Types

```cpp
enum class SwarmFormation {
    None,       // No formation - free flight
    Line,       // Horizontal line
    VFormation, // V-shaped formation
    Grid,       // Grid/rectangular formation
    Circle,     // Circular formation
    Custom      // User-defined custom formation
};
```

### 4. Swarm Member Status

```cpp
enum class SwarmMemberStatus {
    Disconnected,  // No connection
    Connecting,    // Establishing connection
    Ready,         // Connected and armed
    InMission,     // Flying mission
    ReturningHome, // RTL in progress
    Emergency,     // Emergency stop active
    Landed         // On ground
};
```

## Data Flow

### Vehicle Registration
```
MultiVehicleManager → Vehicle added → SwarmManager::addVehicle()
                                          │
                                          ▼
                                    Update vehicle list
                                          │
                                          ▼
                                    Emit swarmMembersChanged()
                                          │
                                          ▼
                              QML UI updates vehicle list
```

### Command Execution
```
User clicks "Takeoff" → SwarmControlPanel → SwarmManager::synchronizedTakeoff()
                                          │
                                          ▼
                              Iterate through all vehicles
                                          │
                                          ▼
                              vehicle->vehicleTakeoff(altitude)
                                          │
                                          ▼
                              Emit synchronizedCommandCompleted()
```

### Health Monitoring
```
SwarmManager::_checkSwarmHealth() → Gather telemetry data
                                    │
                                    ▼
                            Calculate average battery
                                    │
                                    ▼
                            Check collision risks
                                    │
                                    ▼
                            Update health status
                                    │
                                    ▼
                            Emit to UI components
```

## Integration Points

### MultiVehicleManager Integration
- SwarmManager listens to `vehicleAdded`/`vehicleRemoved` signals
- Automatically registers new vehicles to swarm
- Shares vehicle selection with MultiVehicleManager

### FlyView Integration
- MainWindow.showSwarmInterface() added
- SwarmInterface added as third view alongside FlyView/PlanView
- View switching functions updated to handle swarm view

### MissionManager Integration
- `syncWaypoints()` distributes waypoints to all vehicles
- `distributeWaypoints()` allows custom distribution
- Formation offsets applied to mission waypoints

### Settings Integration
- Swarm settings panel in AppSettings
- Formation preferences persisted
- Swarm mode enable/disable toggle

## MAVLink Integration

### Swarm Coordination Messages
```cpp
// Custom swarm coordination (extend as needed)
MAVLINK_MSG_ID_SWARM_COORDINATION (250) // Custom ID
MAVLINK_MSG_ID_FORMATION_UPDATE (251)   // Formation positions
MAVLINK_MSG_ID_SWARM_STATUS (252)       // Swarm health
```

### Message Broadcasting
```cpp
void SwarmManager::_sendSwarmCoordinationMessage(Vehicle* vehicle, 
                                                  int messageId, 
                                                  const QVariantMap &params)
{
    // MAVLink message construction
    // Vehicle->sendMavCommand() or vehicle->sendMessage()
}
```

## Performance Optimization

### Telemetry Handling
- Timer-based updates (100ms default)
- Batch updates when possible
- Async processing for heavy calculations

### UI Responsiveness
- QML Canvas for map rendering (not QML elements)
- Signal-based updates (no polling from QML)
- Lazy loading for vehicle lists

### Threading Model
```
Main Thread     → UI updates, user interactions
Timer Thread    → Swarm health checks
Worker Thread   → Formation calculations
Network Thread  → MAVLink communication (existing)
```

## Backward Compatibility

### Single Vehicle Mode
When `SwarmManager.swarmEnabled == false`:
- All swarm features hidden
- Existing single-vehicle workflow unchanged
- MultiVehicleManager continues to work as before

### Vehicle Selection
- MultiVehicleManager.selectedVehicles() shared
- SwarmManager.selectVehicle() uses MultiVehicleManager
- UI can use either interface for vehicle selection

## Testing Considerations

### Unit Tests
- SwarmManager unit tests
- Formation calculation tests
- Command synchronization tests

### Integration Tests
- Multi-vehicle simulation
- Formation switching
- Emergency stop scenarios

### UI Tests
- SwarmInterface rendering
- Alert system display
- Telemetry chart updates

## Future Improvements

1. **Inter-vehicle communication**
   - Direct vehicle-to-vehicle MAVLink
   - Mesh networking support

2. **Advanced formations**
   - Dynamic re-formation
   - Obstacle avoidance integration
   - 3D formations for multi-altitude operations

3. **Mission synchronization**
   - Phase-based missions
   - Conditional waypoints per vehicle
   - Mission timing synchronization

4. **Telemetry improvements**
   - Real-time formation visualization
   - 3D swarm map
   - Predictive path planning

5. **Fleet management**
   - Subgroups with independent missions
   - Fleet-wide parameter changes
   - Centralized log collection