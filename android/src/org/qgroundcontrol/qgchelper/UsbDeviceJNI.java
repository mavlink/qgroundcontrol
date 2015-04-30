package org.qgroundcontrol.qgchelper;

/* Copyright 2013 Google Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Project home page: http://code.google.com/p/usb-serial-for-android/
 */
///////////////////////////////////////////////////////////////////////////////////////////
//  Written by: Mike Goza April 2014
//
//  These routines interface with the Android USB Host devices for serial port communication.
//  The code uses the usb-serial-for-android software library.  The UsbDeviceJNI class is the
//  interface to the C++ routines through jni calls.  Do not change the functions without also
//  changing the corresponding calls in the C++ routines or you will break the interface.
//
////////////////////////////////////////////////////////////////////////////////////////////

import java.util.HashMap;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.io.IOException;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.*;
import android.widget.Toast;
import android.util.Log;

import com.hoho.android.usbserial.driver.*;
import org.qtproject.qt5.android.bindings.QtActivity;
import org.qtproject.qt5.android.bindings.QtApplication;

public class UsbDeviceJNI extends QtActivity
{
    public static int BAD_PORT = 0;
    private static UsbDeviceJNI m_instance;
    private static UsbManager m_manager;    //  ANDROID USB HOST CLASS
    private static List<UsbSerialDriver> m_devices; //  LIST OF CURRENT DEVICES
    private static HashMap<Integer, UsbSerialDriver> m_openedDevices;   //  LIST OF OPENED DEVICES
    private static HashMap<Integer, UsbIoManager> m_ioManager;	//  THREADS FOR LISTENING FOR INCOMING DATA
    private static HashMap<Integer, Integer> m_userData;    //  CORRESPONDING USER DATA FOR OPENED DEVICES.  USED IN DISCONNECT CALLBACK
    //  USED TO DETECT WHEN A DEVICE HAS BEEN UNPLUGGED
    private BroadcastReceiver m_UsbReceiver = null;
    private final static ExecutorService m_Executor = Executors.newSingleThreadExecutor();
    private static final String TAG = "QGC_UsbDeviceJNI";

    private final static UsbIoManager.Listener m_Listener =
            new UsbIoManager.Listener()
            {
                @Override
                public void onRunError(Exception eA, int userDataA)
                {
                    Log.e(TAG, "onRunError Exception");
                    nativeDeviceException(userDataA, eA.getMessage());
                }

                @Override
                public void onNewData(final byte[] dataA, int userDataA)
                {
                    nativeDeviceNewData(userDataA, dataA);
                }
            };


    //  NATIVE C++ FUNCTION THAT WILL BE CALLED IF THE DEVICE IS UNPLUGGED
    private static native void nativeDeviceHasDisconnected(int userDataA);
    private static native void nativeDeviceException(int userDataA, String messageA);
    private static native void nativeDeviceNewData(int userDataA, byte[] dataA);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Constructor.  Only used once to create the initial instance for the static functions.
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////
    public UsbDeviceJNI()
    {
        m_instance = this;
        m_openedDevices = new HashMap<Integer, UsbSerialDriver>();
        m_userData = new HashMap<Integer, Integer>();
        m_ioManager = new HashMap<Integer, UsbIoManager>();
        Log.i(TAG, "Instance created");
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Find all current devices that match the device filter described in the androidmanifest.xml and the
    //  device_filter.xml
    //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    private static boolean getCurrentDevices()
    {
        if (m_instance == null)
            return false;

        if (m_manager == null)
            m_manager = (UsbManager)m_instance.getSystemService(Context.USB_SERVICE);

        if (m_devices != null)
            m_devices.clear();

        m_devices = UsbSerialProber.findAllDevices(m_manager);

        return true;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  List all available devices that are not already open.  It returns the serial port info
    //  in a : separated string array.  Each string entry consists of the following:
    //
    //  DeviceName:Company:ProductId:VendorId
    //
    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    public static String[] availableDevicesInfo()
    {
        //  GET THE LIST OF CURRENT DEVICES
        if (!getCurrentDevices())
        {
            Log.e(TAG, "UsbDeviceJNI instance not present");
            return null;
        }

        //  MAKE SURE WE HAVE ENTRIES
        if (m_devices.size() <= 0)
        {
            //Log.e(TAG, "No USB devices found");
            return null;
        }

        if (m_openedDevices == null)
        {
            Log.e(TAG, "m_openedDevices is null");
            return null;
        }

        int countL = 0;
        int iL;

        //  CHECK FOR ALREADY OPENED DEVICES AND DON"T INCLUDE THEM IN THE COUNT
        for (iL=0; iL<m_devices.size(); iL++)
        {
            if (m_openedDevices.get(m_devices.get(iL).getDevice().getDeviceId()) != null)
            {
                countL++;
                break;
            }
        }

        if (m_devices.size() - countL <= 0)
        {
            //Log.e(TAG, "No open USB devices found");
            return null;
        }

        String[] listL = new String[m_devices.size() - countL];
        UsbSerialDriver driverL;
        String tempL;

        //  GET THE DATA ON THE INDIVIDUAL DEVICES SKIPPING THE ONES THAT ARE ALREADY OPEN
        countL = 0;
        for (iL=0; iL<m_devices.size(); iL++)
        {
            driverL = m_devices.get(iL);
            if (m_openedDevices.get(driverL.getDevice().getDeviceId()) == null)
            {
                UsbDevice deviceL = driverL.getDevice();
                tempL = deviceL.getDeviceName() + ":";

                if (driverL instanceof FtdiSerialDriver)
                    tempL = tempL + "FTDI:";
                else if (driverL instanceof CdcAcmSerialDriver)
                    tempL = tempL + "Cdc Acm:";
                else if (driverL instanceof Cp2102SerialDriver)
                    tempL = tempL + "Cp2102:";
                else if (driverL instanceof ProlificSerialDriver)
                    tempL = tempL + "Prolific:";
                else
                    tempL = tempL + "Unknown:";

                tempL = tempL + Integer.toString(deviceL.getProductId()) + ":";
                tempL = tempL + Integer.toString(deviceL.getVendorId()) + ":";
                listL[countL] = tempL;
                countL++;
                //Log.i(TAG, "Found " + tempL);
            }
        }

        return listL;
    }



    /////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Open a device based on the name.
    //
    //  Args:   nameA - name of the device to open
    //          userDataA - C++ pointer to the QSerialPort that is trying to open the device.  This is
    //                      used by the detector to inform the C++ side if it is unplugged
    //
    //  Returns:    ID number of opened port.  This number is used to reference the correct port in subsequent
    //              calls like close(), read(), and write().
    //
    /////////////////////////////////////////////////////////////////////////////////////////////////
    public static int open(String nameA, int userDataA)
    {
        int idL = BAD_PORT;

        Log.i(TAG, "Getting device list");
        if (!getCurrentDevices())
            return BAD_PORT;

        //  CHECK THAT PORT IS NOT ALREADY OPENED
        if (m_openedDevices != null)
        {
            for (UsbSerialDriver driverL: m_openedDevices.values())
            {
                if (nameA.equals(driverL.getDevice().getDeviceName()))
                {
                    Log.e(TAG, "Device already opened");
                    return BAD_PORT;
                }
            }
        }
        else
            return BAD_PORT;

        if (m_devices == null)
            return BAD_PORT;

        //  OPEN THE DEVICE
        try
        {
            for (int iL=0; iL<m_devices.size(); iL++)
            {
                Log.i(TAG, "Checking device " + m_devices.get(iL).getDevice().getDeviceName() + " id: " + m_devices.get(iL).getDevice().getDeviceId());
                if (nameA.equals(m_devices.get(iL).getDevice().getDeviceName()))
                {
                    idL = m_devices.get(iL).getDevice().getDeviceId();
                    m_openedDevices.put(idL, m_devices.get(iL));
                    m_userData.put(idL, userDataA);

                    if (m_instance.m_UsbReceiver == null)
                    {
                        Log.i(TAG, "Creating new broadcast receiver");
                        m_instance.m_UsbReceiver= new BroadcastReceiver()
                        {
                            public void onReceive(Context contextA, Intent intentA)
                            {
                                String actionL = intentA.getAction();

                                if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(actionL))
                                {
                                    UsbDevice deviceL = (UsbDevice)intentA.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                                    if (deviceL != null)
                                    {
                                        if (m_userData.containsKey(deviceL.getDeviceId()))
                                        {
                                            nativeDeviceHasDisconnected(m_userData.get(deviceL.getDeviceId()));
                                        }
                                    }
                                }
                            }
                        };
                        //  TURN ON THE INTENT FILTER SO IT WILL DETECT AN UNPLUG SIGNAL
                        IntentFilter filterL = new IntentFilter();
                        filterL.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
                        m_instance.registerReceiver(m_instance.m_UsbReceiver, filterL);
                    }

                    m_openedDevices.get(idL).open();

                    //  START UP THE IO MANAGER TO READ/WRITE DATA TO THE DEVICE
                    UsbIoManager managerL = new UsbIoManager(m_openedDevices.get(idL), m_Listener, userDataA);
                    if (managerL == null)
                        Log.e(TAG, "UsbIoManager instance is null");
                    m_ioManager.put(idL, managerL);
                    m_Executor.submit(managerL);
                    Log.i(TAG, "Port open successfull");
                    return idL;
                }
            }

            return BAD_PORT;
        }
        catch(IOException exA)
        {
            if (idL != BAD_PORT)
            {
                m_openedDevices.remove(idL);
                m_userData.remove(idL);

                if(m_ioManager.get(idL) != null)
                    m_ioManager.get(idL).stop();

                m_ioManager.remove(idL);
            }
            Log.e(TAG, "Port open exception");
            return BAD_PORT;
        }
    }

    public static void startIoManager(int idA)
    {
        if (m_ioManager.get(idA) != null)
            return;

        UsbSerialDriver driverL = m_openedDevices.get(idA);

        if (driverL == null)
            return;

        UsbIoManager managerL = new UsbIoManager(driverL, m_Listener, m_userData.get(idA));
        m_ioManager.put(idA, managerL);
        m_Executor.submit(managerL);
    }

    public static void stopIoManager(int idA)
    {
        if(m_ioManager.get(idA) == null)
            return;

        m_ioManager.get(idA).stop();
        m_ioManager.remove(idA);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Sets the parameters on an open port.
    //
    //  Args:   idA - ID number from the open command
    //          baudRateA - Decimal value of the baud rate.  I.E. 9600, 57600, 115200, etc.
    //          dataBitsA - number of data bits.  Valid numbers are 5, 6, 7, 8
    //          stopBitsA - number of stop bits.  Valid numbers are 1, 2
    //          parityA - No Parity=0, Odd Parity=1, Even Parity=2
    //
    //  Returns:  T/F Success/Failure
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    public static boolean setParameters(int idA, int baudRateA, int dataBitsA, int stopBitsA, int parityA)
    {
        UsbSerialDriver driverL = m_openedDevices.get(idA);

        if (driverL == null)
            return false;

        try
        {
            driverL.setParameters(baudRateA, dataBitsA, stopBitsA, parityA);
            return true;
        }
        catch(IOException eA)
        {
            return false;
        }
    }



    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Close the device.
    //
    //  Args:  idA - ID number from the open command
    //
    //  Returns:  T/F Success/Failure
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////////
    public static boolean close(int idA)
    {
        UsbSerialDriver driverL = m_openedDevices.get(idA);

        if (driverL == null)
            return false;

        try
        {
            stopIoManager(idA);
            m_userData.remove(idA);
            m_openedDevices.remove(idA);
            driverL.close();

            return true;
        }
        catch(IOException eA)
        {
            return false;
        }
    }



    //////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Write data to the device.
    //
    //  Args:   idA - ID number from the open command
    //          sourceA - byte array of data to write
    //          timeoutMsecA - amount of time in milliseconds to wait for the write to occur
    //
    //  Returns:  number of bytes written
    //
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    public static int write(int idA, byte[] sourceA, int timeoutMSecA)
    {
        UsbSerialDriver driverL = m_openedDevices.get(idA);

        if (driverL == null)
            return 0;

        try
        {
            return driverL.write(sourceA, timeoutMSecA);
        }
        catch(IOException eA)
        {
            return 0;
        }
        /*
        UsbIoManager managerL = m_ioManager.get(idA);

        if(managerL != null)
        {
            managerL.writeAsync(sourceA);
            return sourceA.length;
        }
        else
            return 0;
        */
    }



    /////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Determine if a device name if valid.  Note, it does not look for devices that are already open
    //
    //  Args:  nameA - name of device to look for
    //
    //  Returns: T/F
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    public static boolean isDeviceNameValid(String nameA)
    {
        if (m_devices.size() <= 0)
            return false;

        for (int iL=0; iL<m_devices.size(); iL++)
        {
            if (m_devices.get(iL).getDevice().getDeviceName() == nameA)
                return true;
        }

        return false;
    }



    /////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Check if the device is open
    //
    //  Args:  nameA - name of device
    //
    //  Returns:  T/F
    //
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    public static boolean isDeviceNameOpen(String nameA)
    {
        if (m_openedDevices == null)
            return false;

        for (UsbSerialDriver driverL: m_openedDevices.values())
        {
            if (nameA.equals(driverL.getDevice().getDeviceName()))
                return true;
        }

        return false;
    }



    /////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Set the Data Terminal Ready flag on the device
    //
    //  Args:   idA - ID number from the open command
    //          onA - on=T, off=F
    //
    //  Returns:  T/F Success/Failure
    //
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    public static boolean setDataTerminalReady(int idA, boolean onA)
    {
        try
        {
            UsbSerialDriver driverL = m_openedDevices.get(idA);

            if (driverL == null)
                return false;

            driverL.setDTR(onA);
            return true;
        }
        catch(IOException eA)
        {
            return false;
        }
    }



    ////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Set the Request to Send flag
    //
    //  Args:   idA - ID number from the open command
    //          onA - on=T, off=F
    //
    //  Returns:  T/F Success/Failure
    //
    ////////////////////////////////////////////////////////////////////////////////////////////
    public static boolean setRequestToSend(int idA, boolean onA)
    {
        try
        {
            UsbSerialDriver driverL = m_openedDevices.get(idA);

            if (driverL == null)
                return false;

            driverL.setRTS(onA);
            return true;
        }
        catch(IOException eA)
        {
            return false;
        }
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Purge the hardware buffers based on the input and output flags
    //
    //  Args:   idA - ID number from the open command
    //          inputA - input buffer purge.  purge=T
    //          outputA - output buffer purge.  purge=T
    //
    //  Returns:  T/F Success/Failure
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    public static boolean purgeBuffers(int idA, boolean inputA, boolean outputA)
    {
        try
        {
            UsbSerialDriver driverL = m_openedDevices.get(idA);

            if (driverL == null)
                return false;

            return driverL.purgeHwBuffers(inputA, outputA);
        }
        catch(IOException eA)
        {
            return false;
        }
    }



    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Get the native device handle (file descriptor)
    //
    //  Args:   idA - ID number from the open command
    //
    //  Returns:  device handle
    //
    ///////////////////////////////////////////////////////////////////////////////////////////
    public static int getDeviceHandle(int idA)
    {
        UsbSerialDriver driverL = m_openedDevices.get(idA);

        if (driverL == null)
            return -1;

        UsbDeviceConnection connectL = driverL.getDeviceConnection();
        if (connectL == null)
            return -1;
        else
            return connectL.getFileDescriptor();
    }



    //////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Get the open usb serial driver for the given id
    //
    //  Args:  idA - ID number from the open command
    //
    //  Returns:  usb device driver
    //
    /////////////////////////////////////////////////////////////////////////////////////////////
    public static UsbSerialDriver getUsbSerialDriver(int idA)
    {
        return m_openedDevices.get(idA);
    }
}

