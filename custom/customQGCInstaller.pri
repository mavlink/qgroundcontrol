AndroidBuild {
    # We don't sign our build yet
    QMAKE_POST_LINK += mkdir -p $${DESTDIR}/package
    QMAKE_POST_LINK += && make install INSTALL_ROOT=$${DESTDIR}/android-build/
    # We don't sign our build yet
    QMAKE_POST_LINK += && androiddeployqt --input android-lib$${QGC_BINARY_NAME}.so-deployment-settings.json --output $${DESTDIR}/android-build --deployment bundled --gradle
    # In case of signing with debugging key copy that one instead
    QMAKE_POST_LINK += && cp $${DESTDIR}/android-build/build/outputs/apk/android-build-release-signed.apk $${DESTDIR}/package/$${QGC_BINARY_NAME}.apk 2>/dev/null
    QMAKE_POST_LINK += || cp $${DESTDIR}/android-build/build/outputs/apk/android-build-debug.apk $${DESTDIR}/package/$${QGC_BINARY_NAME}.apk
    QMAKE_POST_LINK += && echo "Done APK build"
} else {
    # Include the default installer config
    include($$QGCROOT/QGCInstaller.pri)
}
