QT += androidextras

include($$PWD/libs/qtandroidserialport/src/qtandroidserialport.pri)

ANDROID_PACKAGE_SOURCE_DIR          = $$OUT_PWD/ANDROID_PACKAGE_SOURCE_DIR  # Tells Qt location of package files for build
ANDROID_PACKAGE_QGC_SOURCE_DIR      = $$PWD/android                         # Original location of QGC package files
ANDROID_PACKAGE_CUSTOM_SOURCE_DIR   = $$PWD/custom/android                  # Original location for custom build override package files

# We always move the package files to the ANDROID_PACKAGE_SOURCE_DIR build dir so we can modify the manifest as needed

android_source_dir_target.target = $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml
android_source_dir_target.commands = \
    $$QMAKE_MKDIR $$ANDROID_PACKAGE_SOURCE_DIR && \
    $$QMAKE_COPY_DIR $$ANDROID_PACKAGE_QGC_SOURCE_DIR/* $$ANDROID_PACKAGE_SOURCE_DIR
PRE_TARGETDEPS += $$android_source_dir_target.target
QMAKE_EXTRA_TARGETS += android_source_dir_target
exists($$ANDROID_PACKAGE_CUSTOM_SOURCE_DIR/AndroidManifest.xml) {
    android_source_dir_target.depends = $$ANDROID_PACKAGE_CUSTOM_SOURCE_DIR/AndroidManifest.xml
} else {
    android_source_dir_target.depends = $$ANDROID_PACKAGE_QGC_SOURCE_DIR/AndroidManifest.xml
}

# Custom builds can override android package file

exists($$ANDROID_PACKAGE_CUSTOM_SOURCE_DIR) {
    message("Merging$$ $$ANDROID_PACKAGE_QGC_SOURCE_DIR and $$ANDROID_PACKAGE_CUSTOM_SOURCE_DIR to $$ANDROID_PACKAGE_SOURCE_DIR")

    android_source_dir_target.commands = $$android_source_dir_target.commands && \
            $$QMAKE_COPY_DIR $$ANDROID_PACKAGE_CUSTOM_SOURCE_DIR/* $$ANDROID_PACKAGE_SOURCE_DIR && \
            $$QMAKE_STREAM_EDITOR -i \"s/package=\\\"org.mavlink.qgroundcontrol\\\"/package=\\\"$$QGC_ANDROID_PACKAGE\\\"/\" $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml
}

# Insert package name into manifest file

android_source_dir_target.commands = $$android_source_dir_target.commands && \
        $$QMAKE_STREAM_EDITOR -i \"s/%%QGC_INSERT_PACKAGE_NAME%%/$$QGC_ANDROID_PACKAGE/\" $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml

# Update manifest activity intent filter as needed

QGC_INSERT_ACTIVITY_INTENT_FILTER = ""
AndroidHomeApp {
    # QGC is the android home application
    QGC_INSERT_ACTIVITY_INTENT_FILTER = $$QGC_INSERT_ACTIVITY_INTENT_FILTER "\r\n<category android:name=\\\"android.intent.category.HOME\\\"\\\/>\r\n<category android:name=\\\"android.intent.category.DEFAULT\\\"\\\/>"
}
!contains(DEFINES, NO_SERIAL_LINK) {
    # Add usb device support
    QGC_INSERT_ACTIVITY_INTENT_FILTER = $$QGC_INSERT_ACTIVITY_INTENT_FILTER "\r\n<action android:name=\\\"android.hardware.usb.action.USB_DEVICE_ATTACHED\\\"\\\/>\r\n<action android:name=\\\"android.hardware.usb.action.USB_DEVICE_DETACHED\\\"\\\/>\r\n<action android:name=\\\"android.hardware.usb.action.USB_ACCESSORY_ATTACHED\\\"\\\/>"
}
contains(DEFINES, QGC_ENABLE_BLUETOOTH) {
    QGC_INSERT_ACTIVITY_INTENT_FILTER = $$QGC_INSERT_ACTIVITY_INTENT_FILTER "\r\n<action android:name=\\\"android.bluetooth.device.action.ACL_CONNECTED\\\"\\\/>\r\n<action android:name=\\\"android.bluetooth.device.action.ACL_DISCONNECTED\\\"\\\/>"
}
android_source_dir_target.commands = $$android_source_dir_target.commands && \
    $$QMAKE_STREAM_EDITOR -i \"s/<!-- %%QGC_INSERT_ACTIVITY_INTENT_FILTER -->/$$QGC_INSERT_ACTIVITY_INTENT_FILTER/\" $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml

# Update manifest activity meta data as needed

contains(DEFINES, NO_SERIAL_LINK) {
    # No need to add anything to manifest
    android_source_dir_target.commands = $$android_source_dir_target.commands && \
        $$QMAKE_STREAM_EDITOR -i \"s/<!-- %%QGC_INSERT_ACTIVITY_META_DATA -->//\" $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml
} else {
    # Updates the manifest for usb device support
    android_source_dir_target.commands = $$android_source_dir_target.commands && \
        $$QMAKE_STREAM_EDITOR -i \"s/<!-- %%QGC_INSERT_ACTIVITY_META_DATA -->/<meta-data android:resource=\\\"@xml\\\/device_filter\\\" android:name=\\\"android.hardware.usb.action.USB_DEVICE_ATTACHED\\\"\\\/>\r\n<meta-data android:resource=\\\"@xml\\\/device_filter\\\" android:name=\\\"android.hardware.usb.action.USB_DEVICE_DETACHED\\\"\\\/>\r\n<meta-data android:resource=\\\"@xml\\\/device_filter\\\" android:name=\\\"android.hardware.usb.action.USB_ACCESSORY_ATTACHED\\\"\\\/>/\" $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml
}

# OTHER_FILES makes the specified files be visible in Qt Creator for editing

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
