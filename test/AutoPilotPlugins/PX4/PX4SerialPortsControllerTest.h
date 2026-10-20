#pragma once

#include "BaseClasses/VehicleTest.h"

class PX4SerialPortRow;
class PX4SerialPortsController;

/// PX4 Ports page controller against a PX4 MockLink, which streams only the serial port parameters PX4 would
/// have looked up and answers the rest by name.
class PX4SerialPortsControllerTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit PX4SerialPortsControllerTest(QObject* parent = nullptr)
        : VehicleTest(parent)
    {
        setWaitForParameters(true);
    }

private slots:
    void _pageShownForVehicleWithPortFunctions();
    void _rowsShowVehicleAssignments();
    void _unsentParamLoadingUntilReadEnds_data();
    void _unsentParamLoadingUntilReadEnds();
    void _assignWritesImmediatelyWithoutRebootNotice();
    void _revertRestoresValuesFromPageOpen();
    void _conflictReportedUntilResolved();
    void _sharedPortRunsSeveralFunctions();
    void _changedOptionCountedOnOwningPort_data();
    void _changedOptionCountedOnOwningPort();
    void _rcInputDriverStaysOnDefaultPort();
    void _rebootNoticeReturnsWhenPageCloses();

private:
    static PX4SerialPortRow* _row(const PX4SerialPortsController& controller, int portId);
    static QStringList _optionNames(const PX4SerialPortRow* row);
    /// Waits until no row is loading parameters, so none are in flight when the link closes
    static bool _waitForRowsLoaded(const PX4SerialPortsController& controller);
    bool _waitForMockValue(const QString& paramName, int value);
};
