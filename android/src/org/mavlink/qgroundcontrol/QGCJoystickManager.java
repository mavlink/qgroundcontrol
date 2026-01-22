package org.mavlink.qgroundcontrol;

import android.content.Context;
import android.hardware.input.InputManager;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Build;
import android.util.Log;
import android.util.SparseArray;
import android.view.InputDevice;

public class QGCJoystickManager implements InputManager.InputDeviceListener {
    private static final String TAG = QGCJoystickManager.class.getSimpleName();

    private static QGCJoystickManager m_instance = null;
    private InputManager m_inputManager;
    private final SparseArray<DeviceSensorListener> m_sensorListeners = new SparseArray<>();

    public static void initialize(Context context) {
        if (m_instance != null) {
            return;
        }

        m_instance = new QGCJoystickManager();
        m_instance.m_inputManager = (InputManager) context.getSystemService(Context.INPUT_SERVICE);

        if (m_instance.m_inputManager != null) {
            m_instance.m_inputManager.registerInputDeviceListener(m_instance, null);
            Log.d(TAG, "InputDeviceListener registered.");
        } else {
            Log.w(TAG, "Failed to get InputManager");
        }
    }

    public static void cleanup() {
        if (m_instance != null) {
            m_instance.disableAllSensors();
            if (m_instance.m_inputManager != null) {
                m_instance.m_inputManager.unregisterInputDeviceListener(m_instance);
                Log.d(TAG, "InputDeviceListener unregistered.");
            }
        }
        m_instance = null;
    }

    private static boolean isGameController(InputDevice device) {
        int sources = device.getSources();
        return ((sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD) ||
               ((sources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK);
    }

    @Override
    public void onInputDeviceAdded(int deviceId) {
        InputDevice device = InputDevice.getDevice(deviceId);
        if (device != null && isGameController(device)) {
            Log.d(TAG, "Gamepad added: " + device.getName());
            nativeUpdateAvailableJoysticks();
        }
    }

    @Override
    public void onInputDeviceRemoved(int deviceId) {
        Log.d(TAG, "Input device removed: " + deviceId);
        disableSensors(deviceId);
        nativeUpdateAvailableJoysticks();
    }

    @Override
    public void onInputDeviceChanged(int deviceId) {
        InputDevice device = InputDevice.getDevice(deviceId);
        if (device != null && isGameController(device)) {
            Log.d(TAG, "Gamepad changed: " + device.getName());
            nativeUpdateAvailableJoysticks();
        }
    }

    public static boolean enableGyroscope(int deviceId, boolean enable) {
        if (m_instance == null) {
            return false;
        }
        return m_instance.setSensorEnabled(deviceId, Sensor.TYPE_GYROSCOPE, enable);
    }

    public static boolean enableAccelerometer(int deviceId, boolean enable) {
        if (m_instance == null) {
            return false;
        }
        return m_instance.setSensorEnabled(deviceId, Sensor.TYPE_ACCELEROMETER, enable);
    }

    private boolean setSensorEnabled(int deviceId, int sensorType, boolean enable) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
            Log.w(TAG, "Controller sensors require API 31+");
            return false;
        }

        InputDevice device = InputDevice.getDevice(deviceId);
        if (device == null) {
            Log.w(TAG, "Device not found: " + deviceId);
            return false;
        }

        SensorManager sensorManager = device.getSensorManager();
        if (sensorManager == null) {
            Log.w(TAG, "No SensorManager for device: " + deviceId);
            return false;
        }

        DeviceSensorListener listener = m_sensorListeners.get(deviceId);

        if (enable) {
            Sensor sensor = sensorManager.getDefaultSensor(sensorType);
            if (sensor == null) {
                Log.w(TAG, "Sensor type " + sensorType + " not available on device " + deviceId);
                return false;
            }

            if (listener == null) {
                listener = new DeviceSensorListener(deviceId);
                m_sensorListeners.put(deviceId, listener);
            }

            boolean registered = sensorManager.registerListener(
                listener, sensor, SensorManager.SENSOR_DELAY_GAME);

            if (registered) {
                listener.setSensorEnabled(sensorType, true);
                Log.d(TAG, "Registered sensor " + sensorType + " for device " + deviceId);
            }
            return registered;
        } else {
            if (listener != null) {
                Sensor sensor = sensorManager.getDefaultSensor(sensorType);
                if (sensor != null) {
                    listener.setSensorEnabled(sensorType, false);
                    if (!listener.hasAnySensorEnabled()) {
                        sensorManager.unregisterListener(listener);
                        m_sensorListeners.remove(deviceId);
                        Log.d(TAG, "Unregistered all sensors for device " + deviceId);
                    }
                }
            }
            return true;
        }
    }

    private void disableSensors(int deviceId) {
        DeviceSensorListener listener = m_sensorListeners.get(deviceId);
        if (listener != null) {
            InputDevice device = InputDevice.getDevice(deviceId);
            if (device != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                SensorManager sensorManager = device.getSensorManager();
                if (sensorManager != null) {
                    sensorManager.unregisterListener(listener);
                }
            }
            m_sensorListeners.remove(deviceId);
            Log.d(TAG, "Disabled all sensors for device " + deviceId);
        }
    }

    private void disableAllSensors() {
        for (int i = 0; i < m_sensorListeners.size(); i++) {
            int deviceId = m_sensorListeners.keyAt(i);
            disableSensors(deviceId);
        }
        m_sensorListeners.clear();
    }

    private static class DeviceSensorListener implements SensorEventListener {
        private final int m_deviceId;
        private boolean m_gyroEnabled = false;
        private boolean m_accelEnabled = false;

        DeviceSensorListener(int deviceId) {
            m_deviceId = deviceId;
        }

        void setSensorEnabled(int sensorType, boolean enabled) {
            if (sensorType == Sensor.TYPE_GYROSCOPE) {
                m_gyroEnabled = enabled;
            } else if (sensorType == Sensor.TYPE_ACCELEROMETER) {
                m_accelEnabled = enabled;
            }
        }

        boolean hasAnySensorEnabled() {
            return m_gyroEnabled || m_accelEnabled;
        }

        @Override
        public void onSensorChanged(SensorEvent event) {
            if (event.values.length >= 3) {
                nativeSensorData(m_deviceId, event.sensor.getType(),
                    event.values[0], event.values[1], event.values[2]);
            }
        }

        @Override
        public void onAccuracyChanged(Sensor sensor, int accuracy) {
        }
    }

    private static native void nativeUpdateAvailableJoysticks();
    private static native void nativeSensorData(int deviceId, int sensorType, float x, float y, float z);
}
