message("Adding Android Support for ST16")
QT += androidextras
ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android_typhoonh
OTHER_FILES += \
    $$ANDROID_PACKAGE_SOURCE_DIR/AndroidManifest.xml \
    $$ANDROID_PACKAGE_SOURCE_DIR/src/com/yuneec/datapilot/QGCActivity.java \
    $$ANDROID_PACKAGE_SOURCE_DIR/src/com/yuneec/datapilot/StartQGCAtBootReceiver.java

DISTFILES += \
    $$ANDROID_PACKAGE_SOURCE_DIR/gradle/wrapper/gradle-wrapper.jar \
    $$ANDROID_PACKAGE_SOURCE_DIR/gradlew \
    $$ANDROID_PACKAGE_SOURCE_DIR/res/values/libs.xml \
    $$ANDROID_PACKAGE_SOURCE_DIR/build.gradle \
    $$ANDROID_PACKAGE_SOURCE_DIR/gradle/wrapper/gradle-wrapper.properties \
    $$ANDROID_PACKAGE_SOURCE_DIR/gradlew.bat
