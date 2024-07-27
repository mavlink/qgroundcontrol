package org.mavlink.qgroundcontrol;

import android.util.Log;

import com.hoho.android.usbserial.util.SerialInputOutputManager;

public class QGCSerialListener implements SerialInputOutputManager.Listener
{
	private final static String TAG = QGCSerialListener.class.getSimpleName();

	private long classPtr;

	QGCSerialListener(long classPtr)
	{
        this.classPtr = classPtr;
    }

    @Override
    public void onNewData(byte[] data)
    {
        // Log.i(TAG, "Serial read:" + new String(data, StandardCharsets.UTF_8));
        QGCActivity.nativeDeviceNewData(classPtr, data);
    }

    @Override
    public void onRunError(Exception ex)
    {
        Log.e(TAG, "onRunError Exception", ex);
        QGCActivity.nativeDeviceException(classPtr, ex.getMessage());
    }
}
