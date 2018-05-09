#ifndef QGCHILFLIGHTGEARCONFIGURATION_H
#define QGCHILFLIGHTGEARCONFIGURATION_H

#include <QWidget>

#include "QGCHilLink.h"
#include "QGCFlightGearLink.h"
#include "Vehicle.h"

#include "ui_QGCHilFlightGearConfiguration.h"

namespace Ui {
class QGCHilFlightGearConfiguration;
}

class QGCHilFlightGearConfiguration : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCHilFlightGearConfiguration(Vehicle* vehicle, QWidget *parent = 0);
    ~QGCHilFlightGearConfiguration();

protected:
    
private slots:
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_barometerOffsetLineEdit_textChanged(const QString& baroOffset);
    void _setDefaultOptions(void);
    void _showContextMenu(const QPoint& pt);

private:
    Vehicle*    _vehicle;
    Ui::QGCHilFlightGearConfiguration _ui;

    static const char*  _settingsGroup;                 /// Top level settings group
    const char*         _mavSettingsSubGroup;           /// We maintain a settings sub group per mav type

    static const char*  _mavSettingsSubGroupFixedWing;  /// Subgroup if mav type is MAV_TYPE_FIXED_WING
    static const char*  _mavSettingsSubGroupQuadRotor;  /// Subgroup is mav type is MAV_TYPE_QUADROTOR

    static const char*  _aircraftKey;                   /// Settings key for aircraft selection
    static const char*  _optionsKey;                    /// Settings key for FlightGear cmd line options
    static const char*  _barometerOffsetKey;            /// Settings key for barometer offset
    static const char*  _sensorHilKey;                  /// Settings key for Sensor Hil checkbox
    
    static const char* _defaultOptions;                 /// Default set of FlightGEar command line options
    
    QAction _resetOptionsAction;                        /// Context menu item to reset options to default
    
signals:
     void barometerOffsetChanged(float barometerOffsetkPa);
};

#endif // QGCHILFLIGHTGEARCONFIGURATION_H
