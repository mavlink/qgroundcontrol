#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/private/qandroidextras_p.h>
#include <QtGui/QVector3D>

#include "Joystick.h"

Q_DECLARE_LOGGING_CATEGORY(JoystickAndroidLog)

class JoystickAndroid : public Joystick, public QtAndroidPrivate::GenericMotionEventListener, public QtAndroidPrivate::KeyEventListener
{
public:
    explicit JoystickAndroid(const QString &name, int axisCount, int buttonCount, int id, QObject *parent = nullptr);
    ~JoystickAndroid() override;

    // Static initialization
    static bool init();
    static void shutdown() {}
    static void setNativeMethods();
    static QMap<QString, Joystick*> discover();

    // Device identity
    QString guid() const override { return _descriptor; }
    quint16 vendorId() const override { return _vendorId; }
    quint16 productId() const override { return _productId; }
    bool isVirtual() const override { return _isVirtual; }
    QString serial() const override;
    QString deviceType() const override;
    QString connectionType() const override;

    // Player info
    int playerIndex() const override;

    // Gamepad type
    bool isGamepad() const override { return _isGamepad; }
    QString gamepadType() const override;
    bool requiresCalibration() const override { return !_isGamepad; }

    // Capability queries
    bool hasButton(int button) const override;
    bool hasAxis(int axis) const override;

    // Rumble/Vibration
    bool hasRumble() const override { return _hasVibrator; }
    void rumble(quint16 lowFreq, quint16 highFreq, quint32 durationMs) override;

    // Power and battery (API 31+)
    int batteryPercent() const override;
    QString powerState() const override;

    // LED support (API 31+)
    bool hasLED() const override;
    bool hasRGBLED() const override;
    bool hasPlayerLED() const override;
    void setLED(quint8 red, quint8 green, quint8 blue) override;

    // Sensor support (API 31+)
    bool hasGyroscope() const override;
    bool hasAccelerometer() const override;
    bool setGyroscopeEnabled(bool enabled) override;
    bool setAccelerometerEnabled(bool enabled) override;
    QVector3D gyroscopeData() const override;
    QVector3D accelerometerData() const override;
    float gyroscopeDataRate() const override;
    float accelerometerDataRate() const override;

    // Control labels
    QString axisLabel(int axis) const override;
    QString buttonLabel(int button) const override;

    // Axis calibration
    QVariantMap getAxisInitialState(int axis) const override;

private:
    // Base class overrides
    bool _open() final { return true; }
    void _close() final {}
    bool _update() final { return true; }
    bool _getButton(int i) const final { return (i >= 0 && i < _btnValue.size()) ? _btnValue.at(i) : false; }
    int _getAxisValue(int i) const final { return (i >= 0 && i < _axisValue.size()) ? _axisValue.at(i) : 0; }
    bool _getHat(int hat, int i) const final;

    // JNI helper methods
    QJniObject _getInputDevice() const;
    QJniObject _getSensorManager() const;
    QJniObject _getLightsManager() const;
    QJniObject _getSensor(int type) const;
    QJniObject _getLightsList() const;
    QJniObject _getBatteryState() const;

    // Event handlers
    bool _handleKeyEvent(jobject event);
    bool _handleGenericMotionEvent(jobject event);

    // Internal helpers
    int _getAndroidHatAxis(int axisHatCode) const;
    void _cacheDeviceInfo();
    float _getSensorDataRate(int sensorType) const;
    bool _setSensorEnabled(int sensorType, const char *jniMethodName, bool enabled, bool &enabledFlag);

    // Device state
    int _deviceId = 0;
    QString _descriptor;
    quint16 _vendorId = 0;
    quint16 _productId = 0;
    bool _isVirtual = false;
    bool _isGamepad = false;
    bool _hasVibrator = false;
    int _sources = 0;
    int _deviceType = 0;

    // Sensor state (API 31+)
    QVector3D _gyroData;
    QVector3D _accelData;
    bool _gyroEnabled = false;
    bool _accelEnabled = false;

    // LED session (API 31+)
    QJniObject _lightsSession;

    // Input state
    QList<int> _btnCode;
    QList<int> _axisCode;
    QList<bool> _btnValue;
    QList<int> _axisValue;

    // Static constants
    static constexpr int _androidBtnListCount = 31;
    static int _actionDown;
    static int _actionUp;
    static int _axisHatX;
    static int _axisHatY;

    // Static state
    static QList<int> _androidBtnList;
    static QMutex _mutex;
    static QMap<int, JoystickAndroid*> _instances;

    // Static callbacks
    static void _updateSensorData(int deviceId, int sensorType, float x, float y, float z);
};
