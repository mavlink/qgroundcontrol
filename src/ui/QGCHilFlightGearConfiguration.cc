#include "QGCHilFlightGearConfiguration.h"
#include "MainWindow.h"
#include "UAS.h"

#include <QMenu>

// Various settings groups and keys
const char* QGCHilFlightGearConfiguration::_settingsGroup =                 "QGC_HILCONFIG_FLIGHTGEAR";
const char* QGCHilFlightGearConfiguration::_mavSettingsSubGroupFixedWing =  "FIXED_WING";
const char* QGCHilFlightGearConfiguration::_mavSettingsSubGroupQuadRotor =  "QUADROTOR";
const char* QGCHilFlightGearConfiguration::_aircraftKey =                   "AIRCRAFT";
const char* QGCHilFlightGearConfiguration::_optionsKey =                    "OPTIONS";
const char* QGCHilFlightGearConfiguration::_barometerOffsetKey =            "BARO_OFFSET";
const char* QGCHilFlightGearConfiguration::_sensorHilKey =                  "SENSOR_HIL";

// Default set of optional command line parameters. If FlightGear won't run HIL without it it should go into
// the QGCFlightGearLink code instead.
const char* QGCHilFlightGearConfiguration::_defaultOptions = "--roll=0 --pitch=0 --vc=0 --heading=300 --timeofday=noon --disable-hud-3d --disable-fullscreen --geometry=400x300 --disable-anti-alias-hud --wind=0@0 --turbulence=0.0 --disable-sound --disable-random-objects --disable-ai-traffic --shading-flat --fog-disable --disable-specular-highlight --disable-panel --disable-clouds --fdm=jsb --units-meters --enable-terrasync";

QGCHilFlightGearConfiguration::QGCHilFlightGearConfiguration(Vehicle* vehicle, QWidget *parent)
    : QWidget(parent)
    , _vehicle(vehicle)
    , _mavSettingsSubGroup(NULL)
    , _resetOptionsAction(tr("Reset to default options"), this)

{
    _ui.setupUi(this);
    
    QStringList items;
    if (_vehicle->vehicleType() == MAV_TYPE_FIXED_WING)
    {
        /*items << "EasyStar";*/
        items << "Rascal110-JSBSim";
        /*items << "c172p";
        items << "YardStik";
        items << "Malolo1";*/
        _mavSettingsSubGroup = _mavSettingsSubGroupFixedWing;
    }
    /*else if (_vehicle->vehicleType() == MAV_TYPE_QUADROTOR)
    {
        items << "arducopter";
        _mavSettingsSubGroup = _mavSettingsSubGroupQuadRotor;
    }*/
    else
    {
        // FIXME: Should disable all input, won't work. Show error message in the status label thing.
        items << "<aircraft>";
    }
    _ui.aircraftComboBox->addItems(items);
    
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_mavSettingsSubGroup);
    QString aircraft = settings.value(_aircraftKey).toString();
    QString options = settings.value(_optionsKey).toString();
    QString baroOffset = settings.value(_barometerOffsetKey).toString();
    bool sensorHil = settings.value(_sensorHilKey, QVariant(true)).toBool();
    settings.endGroup();
    settings.endGroup();

    if (!aircraft.isEmpty()) {
        int index = _ui.aircraftComboBox->findText(aircraft);
        if (index != -1) {
            _ui.aircraftComboBox->setCurrentIndex(index);
        }
    }
    if (options.isEmpty()) {
        options = _defaultOptions;
    }
    _ui.optionsPlainTextEdit->setPlainText(options);
    _ui.barometerOffsetLineEdit->setText(baroOffset);
    _ui.sensorHilCheckBox->setChecked(sensorHil);

    // Provide an option on the context menu to reset the option back to default
    _ui.optionsPlainTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    bool success = connect(&_resetOptionsAction, &QAction::triggered, this, &QGCHilFlightGearConfiguration::_setDefaultOptions);
    Q_ASSERT(success);
    success = connect(_ui.optionsPlainTextEdit, &QPlainTextEdit::customContextMenuRequested,
                      this, &QGCHilFlightGearConfiguration::_showContextMenu);
    Q_ASSERT(success);
    Q_UNUSED(success);  // Silence release build unused variable warning
}

QGCHilFlightGearConfiguration::~QGCHilFlightGearConfiguration()
{
    QString aircraft = _ui.aircraftComboBox->currentText();
    QString options = _ui.optionsPlainTextEdit->toPlainText();
    QString baroOffset = _ui.barometerOffsetLineEdit->text();
    bool sensorHil = _ui.sensorHilCheckBox->isChecked();
    
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_mavSettingsSubGroup);

    if (aircraft.isEmpty()) {
        settings.remove(_aircraftKey);
    } else {
        settings.setValue(_aircraftKey, aircraft);
    }
    
    if (options.isEmpty() || options == _defaultOptions) {
        settings.remove(_optionsKey);
    } else {
        settings.setValue(_optionsKey, options);
    }
    
    // Double QVariant is to convert from string to float. It will change to 0.0 if invalid.
    settings.setValue(_barometerOffsetKey, QVariant(QVariant(baroOffset).toFloat()));
    
    settings.setValue(_sensorHilKey, QVariant(sensorHil));
    
    settings.endGroup();
    settings.endGroup();
}

void QGCHilFlightGearConfiguration::on_startButton_clicked()
{
    //XXX check validity of inputs
    QString options = _ui.optionsPlainTextEdit->toPlainText();
    options.append(" --aircraft=" + _ui.aircraftComboBox->currentText());
    _vehicle->uas()->enableHilFlightGear(true, options, _ui.sensorHilCheckBox->isChecked(), this);
}

void QGCHilFlightGearConfiguration::on_stopButton_clicked()
{
    _vehicle->uas()->stopHil();
}

void QGCHilFlightGearConfiguration::on_barometerOffsetLineEdit_textChanged(const QString& baroOffset)
{
    emit barometerOffsetChanged(baroOffset.toFloat());
}

void QGCHilFlightGearConfiguration::_showContextMenu(const QPoint &pt)
{
    QMenu* menu = _ui.optionsPlainTextEdit->createStandardContextMenu();
    menu->addAction(&_resetOptionsAction);
    menu->exec(_ui.optionsPlainTextEdit->mapToGlobal(pt));
    delete menu;
}

void QGCHilFlightGearConfiguration::_setDefaultOptions(void)
{
    _ui.optionsPlainTextEdit->setPlainText(_defaultOptions);
}

