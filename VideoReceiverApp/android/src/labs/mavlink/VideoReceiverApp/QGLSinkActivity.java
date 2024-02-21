package labs.mavlink.VideoReceiverApp;

import org.qtproject.qt.android.bindings.QtActivity;

import android.util.Log;
import android.os.Bundle;

public class QGLSinkActivity extends QtActivity
{
    private static final String LOG_TAG = "QGLSinkActivity";

    public native void nativeInit();

    public QGLSinkActivity()
    {
        Log.i(LOG_TAG, "MainActivity");
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        nativeInit();
        Log.i(LOG_TAG, "Created");
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    public void onInit(int status) {
    }

    public void jniOnLoad() {
        nativeInit();
    }
}
