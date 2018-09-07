# QGroundControl Change Log

Note: This file only contains high level features or important fixes.

## 3.5

### 3.5.0 - Not yet released
* Added Airmap integration to QGC
* Add ESTIMATOR_STATUS values to new estimatorStatus Vehicle FactGroup. These are now available to display in instrument panel.

## 3.4

### 3.4.4 - Not yet released

### 3.4.3
* Fix bug where Resume Mission would not display correctly in some cases. Issue #6835.
* Fix Planned Home Position altitude when no terrain data available. Issue #6846.

### 3.4.2
* Fix bug where new mission items may end up with 0 altitude internally and sent to vehicle while UI shows correct altitude. Issue #6823.

### 3.4.1
* Fix crash when Survery with terrain follow is moved quickly
* Fix terrain follow climb/descent rate fields swapped in ui
