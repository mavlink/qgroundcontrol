installer {
    DEFINES += QGC_INSTALL_RELEASE

    AndroidBuild {
        message("Adding Custom Android APK ")
        # We don't sign Custom build yet
        QMAKE_POST_LINK += && mkdir -p $${DESTDIR}/package
        QMAKE_POST_LINK += && make install INSTALL_ROOT=$${DESTDIR}/android-build/
        QMAKE_POST_LINK += && androiddeployqt --input android-lib$${QGC_BINARY_NAME}.so-deployment-settings.json --output $${DESTDIR}/android-build --deployment bundled --gradle
        contains(QT_ARCH, arm) {
            QGC_APK_BITNESS = "32"
        } else:contains(QT_ARCH, arm64) {
            QGC_APK_BITNESS = "64"
        } else {
            QGC_APK_BITNESS = ""
        }
        QMAKE_POST_LINK += && (cp $${DESTDIR}/android-build/build/outputs/apk/android-build-release-signed.apk $${DESTDIR}/package/$${QGC_BINARY_NAME}$${QGC_APK_BITNESS}.apk 2>/dev/null
        QMAKE_POST_LINK += || cp $${DESTDIR}/android-build/build/outputs/apk/android-build-debug.apk $${DESTDIR}/package/$${QGC_BINARY_NAME}$${QGC_APK_BITNESS}.apk 2>/dev/null
        QMAKE_POST_LINK += || cp $${DESTDIR}/android-build/build/outputs/apk/debug/android-build-debug.apk $${DESTDIR}/package/$${QGC_BINARY_NAME}$${QGC_APK_BITNESS}.apk )
        QMAKE_POST_LINK += && echo "Done APK build"
    }
}
