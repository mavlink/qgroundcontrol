include($$PWD/libs/qtandroidserialport/src/qtandroidserialport.pri)
message("Adding Serial Java Classes")
QT += androidextras

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

android_source_dir_target.commands = echo Updating Android Manifest
android_source_dir_target.depends = FORCE

exists($$PWD/custom/android) {
    message("Merging $$PWD/custom/android/ -> $$PWD/android/")

    ANDROID_PACKAGE_SOURCE_DIR = $$OUT_PWD/ANDROID_PACKAGE_SOURCE_DIR
    android_source_dir_target.target = android_source_dir
    PRE_TARGETDEPS += $$android_source_dir_target.target
    QMAKE_EXTRA_TARGETS += android_source_dir_target

    android_source_dir_target.commands = $$android_source_dir_target.commands && \
            $$QMAKE_MKDIR $$ANDROID_PACKAGE_SOURCE_DIR && \
            $$QMAKE_COPY_DIR $$PWD/android/* $$OUT_PWD/ANDROID_PACKAGE_SOURCE_DIR && \
            $$QMAKE_COPY_DIR $$PWD/custom/android/* $$OUT_PWD/ANDROID_PACKAGE_SOURCE_DIR && \
            $$QMAKE_STREAM_EDITOR -i \"s/package=\\\"org.mavlink.qgroundcontrol\\\"/package=\\\"$$QGC_ANDROID_PACKAGE\\\"/\" $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml
}

NoSerialBuild {
    # No need to add anything to manifest
    android_source_dir_target.commands = $$android_source_dir_target.commands && \
        $$QMAKE_STREAM_EDITOR -i \"s/<!-- %%QGC_INSERT_ACTIVITY_INTENT_FILTER -->//\" $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml
    android_source_dir_target.commands = $$android_source_dir_target.commands && \
        $$QMAKE_STREAM_EDITOR -i \"s/<!-- %%QGC_INSERT_ACTIVITY_META_DATA -->//\" $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml
} else {
    # Updates the manifest for usb device support
    android_source_dir_target.commands = $$android_source_dir_target.commands && \
        $$QMAKE_STREAM_EDITOR -i \"s/<!-- %%QGC_INSERT_ACTIVITY_INTENT_FILTER -->/<action android:name=\\\"android.hardware.usb.action.USB_DEVICE_ATTACHED\\\"\\\/>\r\n<action android:name=\\\"android.hardware.usb.action.USB_DEVICE_DETACHED\\\"\\\/>\r\n<action android:name=\\\"android.hardware.usb.action.USB_ACCESSORY_ATTACHED\\\"\\\/>/\" $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml
    android_source_dir_target.commands = $$android_source_dir_target.commands && \
        $$QMAKE_STREAM_EDITOR -i \"s/<!-- %%QGC_INSERT_ACTIVITY_META_DATA -->/<meta-data android:resource=\\\"@xml\\\/device_filter\\\" android:name=\\\"android.hardware.usb.action.USB_DEVICE_ATTACHED\\\"\\\/>\r\n<meta-data android:resource=\\\"@xml\\\/device_filter\\\" android:name=\\\"android.hardware.usb.action.USB_DEVICE_DETACHED\\\"\\\/>\r\n<meta-data android:resource=\\\"@xml\\\/device_filter\\\" android:name=\\\"android.hardware.usb.action.USB_ACCESSORY_ATTACHED\\\"\\\/>/\" $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml
}

exists($$PWD/custom/android/AndroidManifest.xml) {
    OTHER_FILES += \
    $$PWD/custom/android/AndroidManifest.xml
} else {
    OTHER_FILES += \
    $$PWD/android/AndroidManifest.xml
}

OTHER_FILES += \
    $$PWD/android/res/xml/device_filter.xml \
    $$PWD/android/src/com/hoho/android/usbserial/driver/CdcAcmSerialDriver.java \
    $$PWD/android/src/com/hoho/android/usbserial/driver/CommonUsbSerialDriver.java \
    $$PWD/android/src/com/hoho/android/usbserial/driver/Cp2102SerialDriver.java \
    $$PWD/android/src/com/hoho/android/usbserial/driver/FtdiSerialDriver.java \
    $$PWD/android/src/com/hoho/android/usbserial/driver/ProlificSerialDriver.java \
    $$PWD/android/src/com/hoho/android/usbserial/driver/UsbId.java \
    $$PWD/android/src/com/hoho/android/usbserial/driver/UsbSerialDriver.java \
    $$PWD/android/src/com/hoho/android/usbserial/driver/UsbSerialProber.java \
    $$PWD/android/src/com/hoho/android/usbserial/driver/UsbSerialRuntimeException.java \
    $$PWD/android/src/org/mavlink/qgroundcontrol/QGCActivity.java \
    $$PWD/android/src/org/mavlink/qgroundcontrol/UsbIoManager.java \
    $$PWD/android/src/org/mavlink/qgroundcontrol/TaiSync.java \
    $$PWD/android/src/org/freedesktop/gstreamer/androidmedia/GstAhcCallback.java \
    $$PWD/android/src/org/freedesktop/gstreamer/androidmedia/GstAhsCallback.java \
    $$PWD/android/src/org/freedesktop/gstreamer/androidmedia/GstAmcOnFrameAvailableListener.java


DISTFILES += \
    $$PWD/android/gradle/wrapper/gradle-wrapper.jar \
    $$PWD/android/gradlew \
    $$PWD/android/res/values/libs.xml \
    $$PWD/android/build.gradle \
    $$PWD/android/gradle/wrapper/gradle-wrapper.properties \
    $$PWD/android/gradlew.bat

SOURCES += \
    $$PWD/android/src/AndroidInterface.cc

HEADERS += \
    $$PWD/android/src/AndroidInterface.h

INCLUDEPATH += \
    $$PWD/android/src
