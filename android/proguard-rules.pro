# Qt framework classes - must be preserved for JNI
-keep class org.qtproject.qt.** { *; }
-keep class org.kde.necessitas.** { *; }
-keep class ru.dublgis.** { *; }
-keep class com.falsinsoft.qtandroidtools.** { *; }

# QGC classes with native methods
-keepclasseswithmembers class org.mavlink.qgroundcontrol.QGCActivity {
    native <methods>;
}
-keepclasseswithmembers class org.mavlink.qgroundcontrol.serial.QGCUsbSerialManager {
    native <methods>;
}
-keepclasseswithmembers class org.mavlink.qgroundcontrol.QGCNativeLogSink {
    native <methods>;
}
# Static methods are resolved from C++ by method name/signature.
-keepclassmembers class org.mavlink.qgroundcontrol.QGCActivity {
    public static *;
}
-keepclassmembers class org.mavlink.qgroundcontrol.serial.QGCUsbSerialManager {
    public static *;
}
-keep class org.mavlink.qgroundcontrol.QGCLogger { *; }
-keep class org.mavlink.qgroundcontrol.QGCNativeLogSink { *; }
-keep class org.mavlink.qgroundcontrol.QGCSDLManager { *; }
-keep class org.mavlink.qgroundcontrol.serial.** { *; }

# SDL - native method stubs required for JNI registration
-keep class org.libsdl.app.** { *; }

# GStreamer - native callbacks
-keep class org.freedesktop.gstreamer.** { *; }

# usb-serial-for-android
-keep class com.hoho.android.usbserial.** { *; }

# AndroidX FileProvider
-keep class androidx.core.content.FileProvider { *; }

# androidx.tracing — transitive via androidx.core, which references androidx.tracing.Trace at
# startup. R8 strips it from the minified release APK while leaving the reference live, crashing
# the app with NoClassDefFoundError: Landroidx/tracing/Trace;. Only affects the Release variant.
-keep class androidx.tracing.** { *; }
-dontwarn androidx.tracing.**

# Kotlin runtime — required for on-device androidx.test instrumentation.
# The androidx.test 1.6.x runner is Kotlin-compiled and resolves kotlin.jvm.internal.* (Intrinsics
# null-checks, Metadata, Unit, ...) from the *installed* app's classloader at runtime. The debug
# androidTest APK omits kotlin-stdlib (AGP dedups it against the app variant, which pulls it in
# transitively via androidx.core), so on device these classes survive only if R8 keeps them in the
# release app APK. Without this the runner dies with
#   NoClassDefFoundError: kotlin.jvm.internal.Intrinsics
# in AndroidJUnitRunner.newApplication, before any test executes (PR #14536 emulator CI).
-keep class kotlin.** { *; }
-dontwarn kotlin.**

# Preserve native method names
-keepclasseswithmembernames class * {
    native <methods>;
}

# Preserve enums (used by serialization)
-keepclassmembers enum * {
    public static **[] values();
    public static ** valueOf(java.lang.String);
}
