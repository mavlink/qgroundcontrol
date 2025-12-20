# Adding Custom Mode (qgc side)
## src/FirmwarePlugin/APM/ArduPlaneFirmwarePlugin.cc:

```
	under _setModeEnumToModeStringMapping({
	add
	{ APMPlaneMode::KAMIKAZE      , _kamikaze               }

	under static FlightModeList availableFlightModes = {
	add
	{ _kamikaze               , APMPlaneMode::KAMIKAZE      , true , true }
```
	
## src/FirmwarePlugin/APM/ArduPlaneFirmwarePlugin.h:
```
      under
      enum Mode : uint32_t {
        ...
ADD---> KAMIKAZE      = 27
```

```
    under
    protected:
        uint32_t _convertToCustomFlightModeEnum(uint32_t val) const override;

        const QString _manualFlightMode = tr("Manual");
        const QString _circleFlightMode = tr("Circle");
        ...
ADD---> const QString _kamikaze = tr("Kamikaze");
```


# Adding a Class under new folder(AAD)

ADDED folder: src/AAD

## KamikazeLocManager.cc:
```
    #include <QApplicationStatic>
    #include <QDebug>
    #include "KamikazeLocManager.h"

    Q_APPLICATION_STATIC(KamikazeLocManager, _kamikazeLocManager)

    KamikazeLocManager* KamikazeLocManager::instance()
    {
        return _kamikazeLocManager();
    }
```
## KamikazeLocManager.h:
```
    #pragma once

    #include <QtCore/QObject>
    #include <QGeoCoordinate>

    class KamikazeLocManager : public QObject
    {
        Q_OBJECT

        Q_PROPERTY(QGeoCoordinate coordinate READ coordinate NOTIFY coordinateChanged)

    public:
        static KamikazeLocManager* instance();

        Q_INVOKABLE void printCoordinate(QGeoCoordinate coord) {
            _Coordinate = coord;
            qDebug() << "Current Coordinate:" << _Coordinate;
            emit coordinateChanged();
        }

        QGeoCoordinate coordinate() const { return _Coordinate; }

    signals:
        void coordinateChanged();

    private:
        QGeoCoordinate _Coordinate;
    };
```

## CMakeLists.txt:
```
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            KamikazeLocManager.cc
            KamikazeLocManager.h
    )

    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
```

#### ❗️Important:
 ADDED
 ```
 add_subdirectory(src/AAD)
 ```
  at the end of CMakeLists.txt(not src/AAD/CMakeLists.txt, qgroundcontrol/CMakeLists.txt)


# Accessing Class

## src/QmlControls/QGroundControlQmlGlobal.cc:
```
#include "KamikazeLocManager.h"
```
...
```
    QGroundControlQmlGlobal::QGroundControlQmlGlobal(QObject *parent)
        : QObject(parent)
        ...
        , _multiVehicleManager(MultiVehicleManager::instance())
ADD---> , _kamikazeLocManager(KamikazeLocManager::instance())
```

## src/QmlControls/QGroundControlQmlGlobal.h:
```
// ADD class and MACRO

class ...
class KamikazeLocManager;

Q_MOC_INCLUDE("KamikazeLocManager.h")
```
```
ADDED-> Q_PROPERTY(KamikazeLocManager*  kamikazeLocManager      READ    kamikazeLocManager      CONSTANT)
        Q_PROPERTY(QString              appName                 READ    appName                 CONSTANT)
        Q_PROPERTY(LinkManager*         linkManager             READ    linkManager             CONSTANT)
        ...
```

```
        ...
        ADSBVehicleManager*     adsbVehicleManager  ()  { return _adsbVehicleManager; }
        QmlUnitsConversion*     unitsConversion     ()  { return &_unitsConversion; }
ADDED-> KamikazeLocManager*     kamikazeLocManager  ()  { return _kamikazeLocManager; }
```
```
    private:
ADDED-> KamikazeLocManager*     _kamikazeLocManager     = nullptr;
        QGCMapEngineManager*    _mapEngineManager       = nullptr;
        ADSBVehicleManager*     _adsbVehicleManager     = nullptr;
```

# Using class in FlightMap.qml
## src/FlightMap/FlightMap.qml:

### add property

```
...
property var    _kamikazeLocManager:        QGroundControl.kamikazeLocManager
```
### draw qr icon
```
    // QR icon at set location
    MapQuickItem {
        anchorPoint.x: kamikaze_icon.width / 2
        anchorPoint.y: kamikaze_icon.height / 2
        visible: true
        coordinate: _kamikazeLocManager.coordinate

        sourceItem: Image {
            id: kamikaze_icon
            source:  "/res/qr.png" //"/res/zoom-gps.svg"
            mipmap: true
            antialiasing: true
            fillMode: Image.PreserveAspectFit

            property real baseSize: ScreenTools.defaultFontPixelHeight / 4
            property real referenceZoom: 15

            // SCALE WITH ZOOM
            width:  baseSize * Math.pow(2, _map.zoomLevel - referenceZoom)
            height: width

            sourceSize.height: height
            transform: Rotation {
                origin.x:       kamikaze_icon.width  / 2
                origin.y:       kamikaze_icon.height / 2
                angle:          0
            }
        }
    }
```

### update kamikazeLocManager coord
```
    MultiPointTouchArea {
        ...

        onPressed: (touchPoints) => {
            lastMouseX = touchPoints[0].x
            lastMouseY = touchPoints[0].y

            let coord = _map.toCoordinate(Qt.point(touchPoints[0].x, touchPoints[0].y), false)
            if (_kamikazeLocManager) {
                _kamikazeLocManager.setCoordinate(coord)
            } else {
                console.warn("kamikazeLocManager not available yet")
            }
        }
```

# Adding image resource

## qgcresources.qrc:
```
	    <qresource prefix="/res">
	    ...
ADD->   <file alias="qr.png">resources/qr.png</file>
```

# Misc changes

## Alt+f4 not immediately closes with Active Connection

### src/UI/MainWindow.qml:

```
commented out connection check
	// if (!(_closeChecksToSkip & _skipActiveConnectionsCheckMask) && !checkForActiveConnections()) {
	//     return false
	// }
```


# Build

## first run cmake with build dir: qgroundcontrol/build
Example:
```
cd ~/qgroundcontrol
rm -rf build
mkdir build
cd build
CC=/usr/bin/gcc CXX=/usr/bin/g++ CCACHE_DISABLE=1 \
cmake -S .. -B . -GNinja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DQt6_DIR=~/Qt/6.10.0/gcc_64/lib/cmake/Qt6 \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```
## set alias
```
nano ~/.bashrc
```
add
```
# QGroundControl dev aliases
alias qgc-build='ninja -C ~/qgroundcontrol/build -j8 QGroundControl'
alias qgc-run='~/qgroundcontrol/build/RelWithDebInfo/QGroundControl'
alias qgc-br='ninja -C ~/qgroundcontrol/build -j8 QGroundControl && ~/qgroundcontrol/build/RelWithDebInfo/QGroundControl'

```
## note: -j option of ninja:
```
-j N   run N jobs in parallel (0 means infinity) [default=#CPUs]
```
### i have 12 cores and use -j8 to limit core usage and prevent freeze

