#pragma once

#include <QtCore/QLoggingCategory>
#include <QtGui/QVector3D>

#include "Joystick.h"

struct SDL_Joystick;
typedef struct SDL_Joystick SDL_Joystick;

struct SDL_Gamepad;
typedef struct SDL_Gamepad SDL_Gamepad;

struct SDL_Haptic;
typedef struct SDL_Haptic SDL_Haptic;

Q_DECLARE_LOGGING_CATEGORY(JoystickSDLLog)

class JoystickSDL : public Joystick
{
    Q_OBJECT

    friend class JoystickTest;
    friend class MockJoystick;

public:
    explicit JoystickSDL(const QString &name, const QList<int> &gamepadAxes, const QList<int> &nonGamepadAxes, int buttonCount, int hatCount, int instanceId, QObject *parent = nullptr);
    ~JoystickSDL() override;

    // Instance management
    int instanceId() const { return _instanceId; }
    void setInstanceId(int instanceId) { _instanceId = instanceId; }
    bool requiresCalibration() const override { return !isGamepad(); }

    // Haptic and LED
    bool hasRumble() const override;
    bool hasRumbleTriggers() const override;
    bool hasLED() const override;
    void rumble(quint16 lowFreq, quint16 highFreq, quint32 durationMs) override;
    void rumbleTriggers(quint16 left, quint16 right, quint32 durationMs) override;
    void setLED(quint8 red, quint8 green, quint8 blue) override;
    bool sendEffect(const QByteArray &data) override;

    // Device identity
    QString guid() const override;
    quint16 vendorId() const override;
    quint16 productId() const override;
    QString serial() const override;
    QString deviceType() const override;
    QString path() const override;
    bool isVirtual() const override;
    quint16 firmwareVersion() const override;
    QString connectionType() const override;

    // Player info
    int playerIndex() const override;
    void setPlayerIndex(int index) override;

    // Power and battery
    int batteryPercent() const override;
    QString powerState() const override;

    // Gamepad type
    bool isGamepad() const override;
    QString gamepadType() const override;

    // Control labels
    QString axisLabel(int axis) const override;
    QString buttonLabel(int button) const override;
    QString axisSFSymbol(int axis) const override;
    QString buttonSFSymbol(int button) const override;

    // Mapping management
    QString getMapping() const override;
    bool addMapping(const QString &mapping) override;
    QVariantMap getAxisBinding(int axis) const override;
    QVariantMap getButtonBinding(int button) const override;

    // Sensor support
    bool hasGyroscope() const override;
    bool hasAccelerometer() const override;
    bool setGyroscopeEnabled(bool enabled) override;
    bool setAccelerometerEnabled(bool enabled) override;
    QVector3D gyroscopeData() const override;
    QVector3D accelerometerData() const override;
    float gyroscopeDataRate() const override;
    float accelerometerDataRate() const override;

    // Touchpad support
    int touchpadCount() const override;
    int touchpadFingerCount(int touchpad) const override;
    QVariantMap getTouchpadFinger(int touchpad, int finger) const override;

    // Trackball support
    int ballCount() const override;
    QVariantMap getBall(int ball) const override;

    // Capability queries
    bool hasButton(int button) const override;
    bool hasAxis(int axis) const override;

    // Real gamepad type (actual hardware vs mapped type)
    QString realGamepadType() const override;

    // Type-specific labels
    QString buttonLabelForType(int button) const override;
    QString axisLabelForType(int axis) const override;

    // Haptic/Force Feedback support
    bool hasHaptic() const override;
    int hapticEffectsCount() const override;
    bool hapticRumbleSupported() const override;
    bool hapticRumbleInit() override;
    bool hapticRumblePlay(float strength, quint32 durationMs) override;
    void hapticRumbleStop() override;

    // Mapping utilities
    QString getMappingForGUID(const QString &guid) const override;

    // Virtual joystick control
    bool setVirtualAxis(int axis, int value) override;
    bool setVirtualButton(int button, bool down) override;
    bool setVirtualHat(int hat, quint8 value) override;
    bool setVirtualBall(int ball, int dx, int dy) override;
    bool setVirtualTouchpad(int touchpad, int finger, bool down, float x, float y, float pressure) override;
    bool sendVirtualSensorData(int sensorType, float x, float y, float z) override;

    // Properties/Capability detection
    bool hasMonoLED() const override;
    bool hasRGBLED() const override;
    bool hasPlayerLED() const override;

    // Connection state
    QString connectionState() const override;

    // Initial axis state (for drift detection)
    QVariantMap getAxisInitialState(int axis) const override;

    // Per-device custom mapping
    bool setMapping(const QString &mapping) override;

    // Static methods for virtual joystick creation/destruction (used by tests)
    static int createVirtualJoystick(const QString &name, int axisCount, int buttonCount, int hatCount);
    static bool destroyVirtualJoystick(int instanceId);

    static bool init();
    static bool reloadMappings();
    static void shutdown();
    static void pumpEvents();
    static QMap<QString, Joystick*> discover();

private:
    bool _open() final;
    void _close() final;
    bool _update() final;

    bool _getButton(int idx) const final;
    int _getAxisValue(int idx) const final;
    bool _getHat(int hat, int idx) const final;

    quint64 _getProperties() const;
    bool _checkVirtualJoystick(const char *methodName) const;
    bool _hasGamepadCapability(const char *propertyName) const;
    static QString _connectionStateToString(int state);
    static QString _gamepadTypeEnumToString(int type);
    static void _loadGamepadMappings();

    // Static pre-open device queries
    static QString _getNameForInstanceId(int instanceId);
    static QString _getPathForInstanceId(int instanceId);
    static QString _getGUIDForInstanceId(int instanceId);
    static int _getVendorForInstanceId(int instanceId);
    static int _getProductForInstanceId(int instanceId);
    static int _getProductVersionForInstanceId(int instanceId);
    static QString _getTypeForInstanceId(int instanceId);
    static QString _getRealTypeForInstanceId(int instanceId);
    static int _getPlayerIndexForInstanceId(int instanceId);

    // Static type/string conversions
    static QString _gamepadTypeToString(int type);
    static int _gamepadTypeFromString(const QString &str);
    static QString _gamepadAxisToString(int axis);
    static int _gamepadAxisFromString(const QString &str);
    static QString _gamepadButtonToString(int button);
    static int _gamepadButtonFromString(const QString &str);

    // Static thread safety
    static void _lockJoysticks();
    static void _unlockJoysticks();

    // Static event/polling control
    static void _setJoystickEventsEnabled(bool enabled);
    static bool _joystickEventsEnabled();
    static void _setGamepadEventsEnabled(bool enabled);
    static bool _gamepadEventsEnabled();
    static void _updateJoysticks();
    static void _updateGamepads();

    // Static utilities
    static int _getInstanceIdFromPlayerIndex(int playerIndex);
    static QVariantMap _getGUIDInfo(const QString &guid);
    static int _addMappingsFromFile(const QString &filePath);

    QList<int> _gamepadAxes;
    QList<int> _nonGamepadAxes;
    int _instanceId = -1;

    SDL_Joystick *_sdlJoystick = nullptr;
    SDL_Gamepad *_sdlGamepad = nullptr;
    SDL_Haptic *_sdlHaptic = nullptr;
};
