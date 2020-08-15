include($$PWD/libs/qtandroidserialport/src/qtandroidserialport.pri)
message("Adding Serial Java Classes")
QT += androidextras

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
exists($$PWD/custom/android) {
    message("Merging $$PWD/custom/android/ -> $$PWD/android/")
    ANDROID_PACKAGE_SOURCE_DIR = $$OUT_PWD/ANDROID_PACKAGE_SOURCE_DIR

    javaforce.target = custom_android
    javaforce.commands = $$QMAKE_MKDIR $$ANDROID_PACKAGE_SOURCE_DIR && \
                         $$QMAKE_COPY_DIR $$PWD/android/* $$OUT_PWD/ANDROID_PACKAGE_SOURCE_DIR && \
                         $$QMAKE_COPY_DIR $$PWD/custom/android/* $$OUT_PWD/ANDROID_PACKAGE_SOURCE_DIR
    javaforce.depends = FORCE
    PRE_TARGETDEPS += $$javaforce.target
    QMAKE_EXTRA_TARGETS += javaforce
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
