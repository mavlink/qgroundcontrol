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
        QMAKE_POST_LINK += && mkdir -p $${DESTDIR}/package
        QMAKE_POST_LINK += && cd $${DESTDIR} && $$dirname(QMAKE_QMAKE)/macdeployqt qgroundcontrol.app -appstore-compliant -verbose=2 -qmldir=$${BASEDIR}/src
        QMAKE_POST_LINK += && cd $${OUT_PWD}
        QMAKE_POST_LINK += && hdiutil create -layout SPUD -srcfolder $${DESTDIR}/qgroundcontrol.app -volname QGroundControl $${DESTDIR}/package/qgroundcontrol.dmg
    }
    WindowsBuild {
        # The pdb moving command are commented out for now since we are including the .pdb in the installer. This makes it much
        # easier to debug user crashes.
        #QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY $${DESTDIR_WIN}\\qgroundcontrol.pdb
        #QMAKE_POST_LINK += $$escape_expand(\\n) del $${DESTDIR_WIN}\\qgroundcontrol.pdb
        QMAKE_POST_LINK += $$escape_expand(\\n) cd $$BASEDIR_WIN && $$quote("\"C:\\Program Files \(x86\)\\NSIS\\makensis.exe\"" /NOCD "\"/XOutFile $${DESTDIR_WIN}\\qgroundcontrol-installer.exe\"" "$$BASEDIR_WIN\\deploy\\qgroundcontrol_installer.nsi")
        #QMAKE_POST_LINK += $$escape_expand(\\n) $$QMAKE_COPY qgroundcontrol.pdb $${DESTDIR_WIN}
        #QMAKE_POST_LINK += $$escape_expand(\\n) del qgroundcontrol.pdb
        OTHER_FILES += deploy/qgroundcontrol_installer.nsi
    }
    LinuxBuild {
        #-- TODO: This uses hardcoded paths. It should use $${DESTDIR}
        QMAKE_POST_LINK += && mkdir -p release/package
        QMAKE_POST_LINK += && tar -cjf release/package/qgroundcontrol.tar.bz2 release --exclude='package' --transform 's/release/qgroundcontrol/'
    }
    AndroidBuild {
        #-- TODO: This uses hardcoded paths. It should use $${DESTDIR}
        QMAKE_POST_LINK += && mkdir -p $${DESTDIR}/package
        QMAKE_POST_LINK += && make install INSTALL_ROOT=$${DESTDIR}/android-build/
        QMAKE_POST_LINK += && androiddeployqt --input android-libqgroundcontrol.so-deployment-settings.json --output $${DESTDIR}/android-build --deployment bundled --gradle --sign $${BASEDIR}/android/android_release.keystore dagar --storepass $$(ANDROID_STOREPASS)
        QMAKE_POST_LINK += && cp $${DESTDIR}/android-build/build/outputs/apk/android-build-release-signed.apk $${DESTDIR}/package/qgroundcontrol.apk
    }
}
