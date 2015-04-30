package org.qgroundcontrol.qgchelper;

/* Copyright 2011 Google Inc.
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


import com.hoho.android.usbserial.driver.*;

import java.io.IOException;
import java.nio.ByteBuffer;
import android.util.Log;

/**
* Utility class which services a {@link UsbSerialDriver} in its {@link #run()}
* method.
*
* Original author mike wakerly (opensource@hoho.com)
* Modified by Mike Goza
*/
public class UsbIoManager implements Runnable {
    private static final int READ_WAIT_MILLIS = 100;
    private static final int BUFSIZ = 4096;
    private static final String TAG = "QGC_UsbIoManager";

    private final UsbSerialDriver mDriver;
    private int mUserData;
    private final ByteBuffer mReadBuffer = ByteBuffer.allocate(BUFSIZ);
    private final ByteBuffer mWriteBuffer = ByteBuffer.allocate(BUFSIZ);

    private enum State
    {
        STOPPED,
        RUNNING,
        STOPPING
    }

    // Synchronized by 'this'
    private State mState = State.STOPPED;

    // Synchronized by 'this'
    private Listener mListener;

    public interface Listener
    {
        /**
         * Called when new incoming data is available.
         */
        public void onNewData(byte[] data, int userData);

        /**
         * Called when {@link SerialInputOutputManager#run()} aborts due to an
         * error.
         */
        public void onRunError(Exception e, int userData);
    }



    /**
     * Creates a new instance with no listener.
     */
    public UsbIoManager(UsbSerialDriver driver)
    {
        this(driver, null, 0);
        Log.i(TAG, "Instance created");
    }



    /**
     * Creates a new instance with the provided listener.
     */
    public UsbIoManager(UsbSerialDriver driver, Listener listener, int userData)
    {
        mDriver = driver;
        mListener = listener;
        mUserData = userData;
    }



    public synchronized void setListener(Listener listener)
    {
        mListener = listener;
    }



    public synchronized Listener getListener()
    {
        return mListener;
    }



    public void writeAsync(byte[] data)
    {
        synchronized (mWriteBuffer)
        {
            mWriteBuffer.put(data);
        }
    }



    public synchronized void stop()
    {
        if (getState() == State.RUNNING)
        {
            mState = State.STOPPING;
            mUserData = 0;
        }
    }



    private synchronized State getState()
    {
        return mState;
    }



    /**
     * Continuously services the read and write buffers until {@link #stop()} is
     * called, or until a driver exception is raised.
     */
    @Override
    public void run()
    {
        synchronized (this)
        {
            if (mState != State.STOPPED)
                throw new IllegalStateException("Already running.");

            mState = State.RUNNING;
        }

        try
        {
            while (true)
            {
                if (mState != State.RUNNING)
                    break;

                step();
            }
        }
        catch (Exception e)
        {
            final Listener listener = getListener();
            if (listener != null)
                listener.onRunError(e, mUserData);
        }
        finally
        {
            synchronized (this)
            {
                mState = State.STOPPED;
            }
        }
    }



    private void step() throws IOException
    {
        // Handle incoming data.
        int len = mDriver.read(mReadBuffer.array(), READ_WAIT_MILLIS);

        if (len > 0)
        {
            final Listener listener = getListener();
            if (listener != null)
            {
                final byte[] data = new byte[len];
                mReadBuffer.get(data, 0, len);
                listener.onNewData(data, mUserData);
            }
            mReadBuffer.clear();
        }
/*
        // Handle outgoing data.
        byte[] outBuff = null;
        synchronized (mWriteBuffer)
        {
            if (mWriteBuffer.position() > 0)
            {
                len = mWriteBuffer.position();
                outBuff = new byte[len];
                mWriteBuffer.rewind();
                mWriteBuffer.get(outBuff, 0, len);
                mWriteBuffer.clear();
            }
        }
        if (outBuff != null)
            mDriver.write(outBuff, READ_WAIT_MILLIS);
*/
    }
}

