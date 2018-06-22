# -------------------------------------------------
# QGroundControl - Micro Air Vehicle Groundstation
# Please see our website at <http://qgroundcontrol.org>
# Maintainer:
# Lorenz Meier <lm@inf.ethz.ch>
# (c) 2009-2014 QGroundControl Developers
# License terms set in COPYING.md
# -------------------------------------------------

installer {
    DEFINES += QGC_INSTALL_RELEASE
    MacBuild {
        VideoEnabled {
            # Install the gstreamer framework
            # This will:
            # Copy from the original distibution into DESTDIR/gstwork (if not already there)
            # Prune the framework, removing stuff we don't need
            # Relocate all dylibs so they can work under @executable_path/...
            # Copy the result into the app bundle
            # Make sure qgroundcontrol can find them
            message("Preparing GStreamer Framework")
            QMAKE_POST_LINK += && $$BASEDIR/tools/prepare_gstreamer_framework.sh $${OUT_PWD}/gstwork/ $${DESTDIR}/$${TARGET}.app $${TARGET}
        } else {
            message("Skipping GStreamer Framework")
        }

        # We cd to release directory so we can run macdeployqt without a path to the
        # qgroundcontrol.app file. If you specify a path to the .app file the symbolic
        # links to plugins will not be created correctly.
        QMAKE_POST_LINK += && cd $${DESTDIR} && $$dirname(QMAKE_QMAKE)/macdeployqt $${TARGET}.app -appstore-compliant -verbose=2 -qmldir=$${BASEDIR}/src

        # macdeployqt does not deploy texttospeech plugin
        QMAKE_POST_LINK += && mkdir $${TARGET}.app/Contents/PlugIns/texttospeech
        QMAKE_POST_LINK += && cp $$[QT_INSTALL_PLUGINS]/texttospeech/libqtexttospeech_speechosx.dylib $${TARGET}.app/Contents/PlugIns/texttospeech

        # macdeployqt is missing some relocations once in a while. "Fix" it:
        QMAKE_POST_LINK += && python $$BASEDIR/tools/osxrelocator.py $${TARGET}.app/Contents @rpath @executable_path/../Frameworks -r > /dev/null 2>&1

        # Create package
        QMAKE_POST_LINK += && hdiutil create /tmp/tmp.dmg -ov -volname "$${TARGET}-$${MAC_VERSION}" -fs HFS+ -srcfolder "$${DESTDIR}/"
        QMAKE_POST_LINK += && mkdir -p $${DESTDIR}/package
        QMAKE_POST_LINK += && hdiutil convert /tmp/tmp.dmg -format UDBZ -o $${DESTDIR}/package/$${TARGET}.dmg
        QMAKE_POST_LINK += && rm /tmp/tmp.dmg
    }
    WindowsBuild {
        QMAKE_POST_LINK += $$escape_expand(\\n) cd $$BASEDIR_WIN && $$quote("\"C:\\Program Files \(x86\)\\NSIS\\makensis.exe\"" /DINSTALLER_ICON="\"$${QGC_INSTALLER_ICON}\"" /DHEADER_BITMAP="\"$${QGC_INSTALLER_HEADER_BITMAP}\"" /DAPPNAME="\"$${QGC_APP_NAME}\"" /DEXENAME="\"$${TARGET}\"" /DORGNAME="\"$${QGC_ORG_NAME}\"" /DDESTDIR=$${DESTDIR} /NOCD "\"/XOutFile $${DESTDIR_WIN}\\$${TARGET}-installer.exe\"" "$$BASEDIR_WIN\\deploy\\qgroundcontrol_installer.nsi")
        OTHER_FILES += deploy/qgroundcontrol_installer.nsi
    }
    LinuxBuild {
        #-- TODO: This uses hardcoded paths. It should use $${DESTDIR}
        QMAKE_POST_LINK += && mkdir -p release/package
        QMAKE_POST_LINK += && tar -cjf release/package/QGroundControl.tar.bz2 release --exclude='package' --transform 's/release/qgroundcontrol/'
    }
    AndroidBuild {
        #-- TODO: This uses hardcoded paths. It should use $${DESTDIR}
        QMAKE_POST_LINK += && mkdir -p $${DESTDIR}/package
        QMAKE_POST_LINK += && make install INSTALL_ROOT=$${DESTDIR}/android-build/
        QMAKE_POST_LINK += && androiddeployqt --input android-libQGroundControl.so-deployment-settings.json --output $${DESTDIR}/android-build --deployment bundled --gradle --sign $${BASEDIR}/android/android_release.keystore dagar --storepass $$(ANDROID_STOREPASS)
        QMAKE_POST_LINK += && cp $${DESTDIR}/android-build/build/outputs/apk/android-build-release-signed.apk $${DESTDIR}/package/QGroundControl.apk
    }
}
