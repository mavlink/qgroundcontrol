# Qt framework classes - must be preserved for JNI
-keep class org.qtproject.qt.** { *; }
-keep class org.kde.necessitas.** { *; }
-keep class ru.dublgis.** { *; }
-keep class com.falsinsoft.qtandroidtools.** { *; }

# QGC classes with native methods
-keepclasseswithmembers class org.jiacdigcs.swarm.QGCActivity {
    native <methods>;
}
-keepclasseswithmembers class org.jiacdigcs.swarm.QGCUsbSerialManager {
    native <methods>;
}
# Static methods are resolved from C++ by method name/signature.
-keepclassmembers class org.jiacdigcs.swarm.QGCActivity {
    public static *;
}
-keepclassmembers class org.jiacdigcs.swarm.QGCUsbSerialManager {
    public static *;
}
-keep class org.jiacdigcs.swarm.QGCUsbId { *; }
-keep class org.jiacdigcs.swarm.QGCUsbSerialProber { *; }
-keep class org.jiacdigcs.swarm.QGCLogger { *; }
-keep class org.jiacdigcs.swarm.QGCFtdiSerialDriver { *; }
-keep class org.jiacdigcs.swarm.QGCFtdiSerialDriver$QGCFtdiSerialPort { *; }
-keep class org.jiacdigcs.swarm.QGCFtdiDriver { *; }
-keep class org.jiacdigcs.swarm.QGCSDLManager { *; }

# SDL - native method stubs required for JNI registration
-keep class org.libsdl.app.** { *; }

# GStreamer - native callbacks
-keep class org.freedesktop.gstreamer.** { *; }

# usb-serial-for-android
-keep class com.hoho.android.usbserial.** { *; }

# AndroidX FileProvider
-keep class androidx.core.content.FileProvider { *; }

# Preserve native method names
-keepclasseswithmembernames class * {
    native <methods>;
}

# Preserve enums (used by serialization)
-keepclassmembers enum * {
    public static **[] values();
    public static ** valueOf(java.lang.String);
}
