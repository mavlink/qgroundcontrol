include($$PWD/libs/qtandroidserialport/src/qtandroidserialport.pri)

ANDROID_MIN_SDK_VERSION = 26
ANDROID_TARGET_SDK_VERSION = 33

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

 equals(QMAKE_HOST.os, Darwin) {
    # Latest Mac OSX has different sed than regular linux.
    SED_I = '$$QMAKE_STREAM_EDITOR -i \"\"'
} else {
    SED_I = '$$QMAKE_STREAM_EDITOR -i'
}

exists($$ANDROID_PACKAGE_CUSTOM_SOURCE_DIR) {
    message("Merging$$ $$ANDROID_PACKAGE_QGC_SOURCE_DIR and $$ANDROID_PACKAGE_CUSTOM_SOURCE_DIR to $$ANDROID_PACKAGE_SOURCE_DIR")

    android_source_dir_target.commands = $$android_source_dir_target.commands && \
            $$QMAKE_COPY_DIR $$ANDROID_PACKAGE_CUSTOM_SOURCE_DIR/* $$ANDROID_PACKAGE_SOURCE_DIR && \
            $$SED_I \"s/package=\\\"org.mavlink.qgroundcontrol\\\"/package=\\\"$$QGC_ANDROID_PACKAGE\\\"/\" $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml
}

# Insert package name into manifest file

android_source_dir_target.commands = $$android_source_dir_target.commands && \
        $$SED_I \"s/%%QGC_INSERT_PACKAGE_NAME%%/$$QGC_ANDROID_PACKAGE/\" $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml

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
    $$SED_I \"s/<!-- %%QGC_INSERT_ACTIVITY_INTENT_FILTER -->/$$QGC_INSERT_ACTIVITY_INTENT_FILTER/\" $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml

# Update manifest activity meta data as needed

contains(DEFINES, NO_SERIAL_LINK) {
    # No need to add anything to manifest
    android_source_dir_target.commands = $$android_source_dir_target.commands && \
        $$SED_I \"s/<!-- %%QGC_INSERT_ACTIVITY_META_DATA -->//\" $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml
} else {
    # Updates the manifest for usb device support
    android_source_dir_target.commands = $$android_source_dir_target.commands && \
        $$SED_I \"s/<!-- %%QGC_INSERT_ACTIVITY_META_DATA -->/<meta-data android:resource=\\\"@xml\\\/device_filter\\\" android:name=\\\"android.hardware.usb.action.USB_DEVICE_ATTACHED\\\"\\\/>\r\n<meta-data android:resource=\\\"@xml\\\/device_filter\\\" android:name=\\\"android.hardware.usb.action.USB_DEVICE_DETACHED\\\"\\\/>\r\n<meta-data android:resource=\\\"@xml\\\/device_filter\\\" android:name=\\\"android.hardware.usb.action.USB_ACCESSORY_ATTACHED\\\"\\\/>/\" $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml
}

# OTHER_FILES makes the specified files be visible in Qt Creator for editing

exists($$ANDROID_PACKAGE_CUSTOM_SOURCE_DIR/AndroidManifest.xml) {
    DISTFILES += \
        $$ANDROID_PACKAGE_CUSTOM_SOURCE_DIR/AndroidManifest.xml
} else {
    DISTFILES += \
        $$ANDROID_PACKAGE_QGC_SOURCE_DIR/AndroidManifest.xml
}

DISTFILES += \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/build.gradle \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/gradle/wrapper/gradle-wrapper.jar \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/gradle/wrapper/gradle-wrapper.properties \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/gradlew \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/gradlew.bat \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/res/values/libs.xml \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/res/xml/device_filter.xml \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/res/xml/network_security_config.xml \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/res/xml/qtprovider_paths.xml \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/com/hoho/android/usbserial/driver/CdcAcmSerialDriver.java \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/com/hoho/android/usbserial/driver/CommonUsbSerialDriver.java \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/com/hoho/android/usbserial/driver/Cp2102SerialDriver.java \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/com/hoho/android/usbserial/driver/FtdiSerialDriver.java \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/com/hoho/android/usbserial/driver/ProlificSerialDriver.java \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/com/hoho/android/usbserial/driver/UsbId.java \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/com/hoho/android/usbserial/driver/UsbSerialDriver.java \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/com/hoho/android/usbserial/driver/UsbSerialProber.java \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/com/hoho/android/usbserial/driver/UsbSerialRuntimeException.java \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/org/freedesktop/gstreamer/androidmedia/GstAhcCallback.java \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/org/freedesktop/gstreamer/androidmedia/GstAhsCallback.java \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/org/freedesktop/gstreamer/androidmedia/GstAmcOnFrameAvailableListener.java \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/org/mavlink/qgroundcontrol/QGCActivity.java \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/org/mavlink/qgroundcontrol/UsbIoManager.java

SOURCES += \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/AndroidInterface.cc
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/AndroidInit.cpp

HEADERS += \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src/AndroidInterface.h

INCLUDEPATH += \
    $$ANDROID_PACKAGE_QGC_SOURCE_DIR/src
