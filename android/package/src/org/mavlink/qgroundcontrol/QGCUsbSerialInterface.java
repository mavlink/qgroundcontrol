package org.mavlink.qgroundcontrol;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.util.Log;

import com.hoho.android.usbserial.driver.*;
import com.hoho.android.usbserial.util.*;

public class UsbSerialInterface implements SerialInputOutputManager.Listener
{
    private static final String TAG = getSimpleName();
    private static final int BAD_DEVICE_ID = 0;
    private static final String ACTION_USB_PERMISSION = getPackageName() + "action.USB_PERMISSION";
    private enum UsbPermission { Unknown, Requested, Granted, Denied }
    private UsbManager _usbManager;
    private List<UsbSerialDriver> _usbDrivers;
    private SerialInputOutputManager _usbIoManager;
    private UsbSerialProber _usbProber;
    private ProbeTable _usbProbeTable;
    private UsbPermission _usbPermission = UsbPermission.Unknown;
    private int _baudrate;
    private int _databits;
    private int _stopbits;
    private int _parity;

    public UsbSerialInterface(Context context)
    {
    	BroadcastReceiver receiver = new BroadcastReceiver()
        {
            public void onReceive(Context context, Intent intent)
            {
                String action = intent.getAction();
                Log.i(TAG, "USB BroadcastReceiver action " + action);

                if(action.equals(ACTION_USB_PERMISSION))
                {
                    _handleUsbPermissions(context, intent);
                }
                else if(action.equals(UsbManager.ACTION_USB_DEVICE_DETACHED))
                {
                    _handleUsbDetached(context, intent);
                }
                else if (action.equals(UsbManager.ACTION_USB_DEVICE_ATTACHED))
                {
                    _handleUsbAttached(context, intent);
                }

                /* nativeUpdateAvailableJoysticks(); */
            }
        };

    	IntentFilter usbFilter = new IntentFilter();
        usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
        usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
        usbFilter.addAction(ACTION_USB_PERMISSION);
        context.registerReceiver(receiver, usbFilter);
        // if(android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.TIRAMISU) {
        //     _instance.registerReceiver(_instance._usbReceiver, filter, RECEIVER_EXPORTED);
        // } else {
        //     _instance.registerReceiver(_instance._usbReceiver, filter);
        // }

    	_usbDrivers = new ArrayList<UsbSerialDriver>();

        _usbProbeTable = getDefaultProbeTable();
        _addUsbIdsToProbeTable();
        _usbProber = new UsbSerialProber(_usbProbeTable);

        _usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
    }

    @Override
    public void onRunError(Exception ex)
    {
        Log.e(TAG, "onRunError Exception: " + ex.getMessage());
    }

    @Override
    public void onNewData(final byte[] data)
    {
        Log.i(TAG, "onNewData");
        nativeDeviceNewData(deviceId, data);
    }

    private void _addUsbIdsToProbeTable()
    {
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_PX4, QGCUsbId.DEVICE_PX4FMU, CdcAcmSerialDriver.class);

        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ATMEL, QGCUsbId.DEVICE_ATMEL_LUFA_CDC_DEMO_APP, CdcAcmSerialDriver.class);

        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_UNO, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_MEGA_2560, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_SERIAL_ADAPTER, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_MEGA_ADK, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_MEGA_2560_R3, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_UNO_R3, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_MEGA_ADK_R3, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_SERIAL_ADAPTER_R3, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUINO, QGCUsbId.DEVICE_ARDUINO_LEONARDO, CdcAcmSerialDriver.class);

        _usbProbeTable.addProduct(QGCUsbId.VENDOR_VAN_OOIJEN_TECH, QGCUsbId.DEVICE_VAN_OOIJEN_TECH_TEENSYDUINO_SERIAL, CdcAcmSerialDriver.class);

        _usbProbeTable.addProduct(QGCUsbId.VENDOR_LEAFLABS, QGCUsbId.DEVICE_LEAFLABS_MAPLE, CdcAcmSerialDriver.class);

        _usbProbeTable.addProduct(QGCUsbId.VENDOR_UBLOX, QGCUsbId.DEVICE_UBLOX_5, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_UBLOX, QGCUsbId.DEVICE_UBLOX_6, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_UBLOX, QGCUsbId.DEVICE_UBLOX_7, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_UBLOX, QGCUsbId.DEVICE_UBLOX_8, CdcAcmSerialDriver.class);

        _usbProbeTable.addProduct(QGCUsbId.VENDOR_OPENPILOT, QGCUsbId.DEVICE_REVOLUTION, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_OPENPILOT, QGCUsbId.DEVICE_OPLINK, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_OPENPILOT, QGCUsbId.DEVICE_SPARKY2, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_OPENPILOT, QGCUsbId.DEVICE_CC3D, CdcAcmSerialDriver.class);

        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUPILOT_CHIBIOS1, QGCUsbId.DEVICE_ARDUPILOT_CHIBIOS, CdcAcmSerialDriver.class);
        _usbProbeTable.addProduct(QGCUsbId.VENDOR_ARDUPILOT_CHIBIOS2, QGCUsbId.DEVICE_ARDUPILOT_CHIBIOS, CdcAcmSerialDriver.class);

        _usbProbeTable.addProduct(QGCUsbId.VENDOR_DRAGONLINK, QGCUsbId.DEVICE_DRAGONLINK, CdcAcmSerialDriver.class);
    }

    private void _handleUsbPermissions(Context context, Intent intent)
    {
        final UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if (device != null)
        {
            if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false))
            {
                _usbPermission = UsbPermission.Granted;
                nativeLogDebug("Permission granted to " + device.getDeviceName());
            }
            else
            {
                _usbPermission = UsbPermission.Denied;
                nativeLogDebug("Permission denied for " + device.getDeviceName());
            }
        }
        else
        {
            _usbPermission = UsbPermission.Unknown;
        }
    }

    private void _handleUsbAttached(Context context, Intent intent)
    {
        final UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if (device != null)
        {
            UsbSerialDriver driver = _usbProber.probeDevice(device);
            UsbDeviceConnection usbConnection = _usbManager.openDevice(device);
            if(usbConnection == null)
            {
                if(usbPermission == UsbPermission.Unknown && !_usbManager.hasPermission(device))
                {
                    final int flags = Build.VERSION.SDK_INT >= Build.VERSION_CODES.M ? PendingIntent.FLAG_IMMUTABLE : 0;
                    Intent intent = new Intent(INTENT_ACTION_GRANT_USB);
                    intent.setPackage(context.getPackageName());
                    PendingIntent usbPermissionIntent = PendingIntent.getBroadcast(context, 0, intent, flags);
                    _usbManager.requestPermission(device, usbPermissionIntent);
                    _usbPermission = UsbPermission.Requested;
                }
                else
                {
                    if (!_usbManager.hasPermission(driver.getDevice()))
                    {
                        nativeLogDebug("connection failed: permission denied");
                    }
                    else
                    {
                        nativeLogDebug("connection failed: open failed");
                    }
                }
            }
            else
            {
                try
                {
                    usbSerialPort.open(usbConnection);
                }
                catch (IOException ex)
                {
                    nativeLogWarning("_handleUsbAttached exception: " + ex.getMessage());
                }

                try
                {
                    usbSerialPort.setParameters(115200, 8, 1, UsbSerialPort.PARITY_NONE);
                }
                catch (UnsupportedOperationException ex)
                {
                    nativeLogWarning("unsupported setParameters: " + ex.getMessage());
                }

                if(usbSerialPort.isOpen())
                {
                    _usbIoManager = new SerialInputOutputManager(usbSerialPort, context);
                }
            }
        }
    }

    private void _handleUsbDetached(Context context, Intent intent)
    {
        final UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if(device != null)
        {
            if(_usbIoManager != null)
            {
                _usbIoManager.setListener(null);
                _usbIoManager.stop();
            }

            try
            {
                usbSerialPort.close();
            }
            catch (IOException ex)
            {
                nativeLogWarning("getDataSetReady exception: " + ex.getMessage());
            }
            usbSerialPort = null;
        }
    }

    private UsbSerialDriver _findDriver(int deviceId)
    {
        UsbSerialDriver result = null;

        for (UsbSerialDriver driver: _usbDrivers)
        {
            if (driver.getDevice().getDeviceId() == deviceId)
            {
                result = driver;
                break;
            }
        }

        return result;
    }

    private UsbSerialDriver _findDriver(String deviceName)
    {
        UsbSerialDriver result = null;

        for (UsbSerialDriver driver: _usbDrivers)
        {
            if (driver.getDevice().getDeviceName().equals(deviceName))
            {
                result = driver;
                break;
            }
        }

        return result;
    }

    private UsbSerialDriver _findDriver(UsbDevice usbDevice)
    {
        return _usbProbeTable.findDriver(usbDevice);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // USB Public Interface
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    public String[] availableDevicesInfo()
    {
        List<String> deviceInfoList = new ArrayList<String>();

        for (UsbSerialDriver driver: _usbProber.findAllDrivers(_usbManager))
        {
            final UsbDevice device = driver.getDevice();
            String deviceInfo = device.getDeviceName() + ":";

            if (driver instanceof CdcAcmSerialDriver)
            {
                deviceInfo += "CdcAcm:";
            }
            else if (driver instanceof Ch34xSerialDriver)
            {
                deviceInfo += "Ch34x:";
            }
            else if (driver instanceof ChromeCcdSerialDriver)
            {
                deviceInfo += "ChromeCcd:";
            }
            else if (driver instanceof Cp21xxSerialDriver)
            {
                deviceInfo += "Cp21xx:";
            }
            else if (driver instanceof FtdiSerialDriver)
            {
                deviceInfo += "FTDI:";
            }
            else if (driver instanceof GsmModemSerialDriver)
            {
                deviceInfo += "GsmModem:";
            }
            else if (driver instanceof ProlificSerialDriver)
            {
                deviceInfo += "Prolific:";
            }
            else
            {
                deviceInfo += "Unknown:";
            }

            deviceInfo += Integer.toString(device.getProductId()) + ":";
            deviceInfo += Integer.toString(device.getVendorId()) + ":";

            deviceInfoList.add(deviceInfo);
        }

        String[] rgDeviceInfo = new String[deviceInfoList.size()];
        for (int i = 0; i < deviceInfoList.size(); i++)
        {
            rgDeviceInfo[i] = deviceInfoList.get(i);
        }

        return rgDeviceInfo;
    }

    public int open(String deviceName)
    {
        int deviceId = BAD_DEVICE_ID;

        UsbSerialDriver driver = _findDriver(deviceName);

        if(driver != null)
        {
            UsbDevice device = driver.getDevice();
            if(device != null)
            {
                try
                {
                    driver.open(_usbManager.openDevice(device));
                }
                catch(IOException ex)
                {
                    nativeLogWarning("open exception: " + ex.getMessage());
                }

                if(driver.isOpen())
                {
                    deviceId = device.getDeviceId();
                    if(_usbIoManager != null)
			        {
			            _usbIoManager.start();
			        }
                }
            }
        }

        return deviceId;
    }

    public void close(int deviceId)
    {
        UsbSerialDriver driver = _findDriver(deviceId);

        if(driver != null)
        {
            try
            {
                driver.close();
            }
            catch(IOException ex)
            {
                nativeLogWarning("close exception: " + ex.getMessage());
            }

            if(!driver.isOpen())
            {
                if(_usbIoManager != null)
		        {
		            _usbIoManager.stop();
		        }
            }
        }
    }

    public boolean isDeviceOpen(String name)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriver(name);

        if(driver != null)
        {
            result = driver.isOpen();
        }

        return result;
    }

    public boolean isDeviceOpen(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriver(deviceId);

        if(driver != null)
        {
            result = driver.isOpen();
        }

        return result;
    }

    public int read(int deviceId, byte[] data, int timeoutMs)
    {
        int result = 0;

        UsbSerialDriver driver = _findDriver(deviceId);

        if(driver != null)
        {
            try
            {
                result = driver.read(data, timeoutMs);
            }
            catch(IOException ex)
            {
                nativeLogWarning("read exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public void write(int deviceId, byte[] data, int timeoutMs)
    {
        UsbSerialDriver driver = _findDriver(deviceId);

        if(driver != null)
        {
            try
            {
                driver.write(data, timeoutMs);
            }
            catch(IOException ex)
            {
                nativeLogWarning("write exception: " + ex.getMessage());
            }
        }
    }

    public void writeAsync(int deviceId, byte[] data, int timeoutMs)
    {
        if((_usbIoManager != null) && (timeoutMs > 0))
        {
            _usbIoManager.setWriteTimeout(timeoutMs);
            _usbIoManager.writeAsync(data);
        }
    }

    public void setParameters(int deviceId, int baudRate, int dataBits, int stopBits, int parity)
    {
        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                driver.setParameters(baudRate, dataBits, stopBits, parity);
            }
            catch(IOException ex)
            {
                nativeLogWarning("setParameters exception: " + ex.getMessage());
            }
        }
    }

    public boolean getCarrierDetect(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getCD();
            }
            catch(IOException ex)
            {
                nativeLogWarning("getCarrierDetect exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public boolean getClearToSend(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getCTS();
            }
            catch(IOException ex)
            {
                nativeLogWarning("getClearToSend exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public boolean getDataSetReady(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getDSR();
            }
            catch(IOException ex)
            {
                nativeLogWarning("getDataSetReady exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public boolean getDataTerminalReady(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getDTR();
            }
            catch(IOException ex)
            {
                nativeLogWarning("getDataTerminalReady exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public void setDataTerminalReady(int deviceId, boolean on)
    {
        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                driver.setDTR(on);
            }
            catch(IOException ex)
            {
                nativeLogWarning("setDataTerminalReady exception: " + ex.getMessage());
            }
        }
    }

    public boolean getRingIndicator(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getDTR();
            }
            catch(IOException ex)
            {
                nativeLogWarning("getRingIndicator exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public boolean getRequestToSend(int deviceId)
    {
        boolean result = false;

        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                result = driver.getRTS();
            }
            catch(IOException ex)
            {
                nativeLogWarning("getRequestToSend exception: " + ex.getMessage());
            }
        }

        return result;
    }

    public void setRequestToSend(int deviceId, boolean on)
    {
        UsbSerialDriver driver = _findDriver(deviceId);

        if (driver != null)
        {
            try
            {
                driver.setRTS(on);
            }
            catch(IOException ex)
            {
                nativeLogWarning("setRequestToSend exception: " + ex.getMessage());
            }
        }
    }

    public void flush(int deviceId, boolean output, boolean input)
    {
        UsbSerialDriver driver = _findDriver(deviceId);

        if(driver != null)
        {
            try
            {
                driver.purgeHwBuffers(output, input);
            }
            catch(IOException ex)
            {
                nativeLogWarning("flush exception: " + ex.getMessage());
            }
        }
    }

    public void setBreak(int deviceId, boolean on)
    {
        UsbSerialDriver driver = _findDriver(deviceId);

        if(driver != null)
        {
            try
            {
                driver.setBreak(on);
            }
            catch(IOException ex)
            {
                nativeLogWarning("setBreak exception: " + ex.getMessage());
            }
        }
    }
}