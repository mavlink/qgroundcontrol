################################################################################
#
# (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
#
# QGroundControl is licensed according to the terms in the file
# COPYING.md in the root of the source code directory.
#
################################################################################

# These are the Post Link steps which are specific to installer builds

installer {
    DEFINES += QGC_INSTALL_RELEASE
    MacBuild {
        QMAKE_POST_LINK += && echo macdeployqt
        QMAKE_POST_LINK += && $$dirname(QMAKE_QMAKE)/macdeployqt $${TARGET}.app -appstore-compliant -verbose=1 -qmldir=$${SOURCE_DIR}/src

        # macdeployqt is missing some relocations once in a while. "Fix" it:
        QMAKE_POST_LINK += && cp -R /Library/Frameworks/GStreamer.framework $${TARGET}.app/Contents/Frameworks
        QMAKE_POST_LINK += && echo libexec
        QMAKE_POST_LINK += && ln -sf $${TARGET}.app/Contents/Frameworks $${TARGET}.app/Contents/Frameworks/GStreamer.framework/Versions/1.0/libexec/Frameworks
        QMAKE_POST_LINK += && install_name_tool -change /Library/Frameworks/GStreamer.framework/Versions/1.0/lib/GStreamer @executable_path/../Frameworks/GStreamer.framework/Versions/1.0/lib/GStreamer $${TARGET}.app/Contents/MacOS/QGroundControl
        QMAKE_POST_LINK += && rm -rf $${TARGET}.app/Contents/Frameworks/GStreamer.framework/Versions/1.0/{bin,etc,share,Headers,include,Commands}
        QMAKE_POST_LINK += && rm -rf $${TARGET}.app/Contents/Frameworks/GStreamer.framework/Versions/1.0/lib/{*.a,*.la,glib-2.0,gst-validate-launcher,pkgconfig}

        codesign {
            # Disabled for now since it's not working correctly yet
            #QMAKE_POST_LINK += && echo codesign
            #QMAKE_POST_LINK += && codesign --deep $${TARGET}.app -s WQREC9W69J
        }

        # Create package
        QMAKE_POST_LINK += && echo hdiutil
        QMAKE_POST_LINK += && mkdir -p package
        QMAKE_POST_LINK += && mkdir -p staging
        QMAKE_POST_LINK += && rsync -a --delete $${TARGET}.app staging
        QMAKE_POST_LINK += && hdiutil create /tmp/tmp.dmg -ov -volname "$${TARGET}-$${MAC_VERSION}" -fs HFS+ -srcfolder "staging"
        QMAKE_POST_LINK += && hdiutil convert /tmp/tmp.dmg -format UDBZ -o package/$${TARGET}.dmg
        QMAKE_POST_LINK += && rm /tmp/tmp.dmg
    }
    WindowsBuild {
        QMAKE_POST_LINK += $$escape_expand(\\n) $$quote("\"C:\\Program Files \(x86\)\\NSIS\\makensis.exe\"" $$(QGC_NSIS_INSTALLER_PARAMETERS) /DDRIVER_MSI="$$SOURCE_DIR\\deploy\\driver.msi" /DINSTALLER_ICON="\"$${QGC_INSTALLER_ICON}\"" /DHEADER_BITMAP="\"$${QGC_INSTALLER_HEADER_BITMAP}\"" /DAPPNAME="\"$${QGC_APP_NAME}\"" /DEXENAME="\"$${TARGET}\"" /DORGNAME="\"$${QGC_ORG_NAME}\"" /DDESTDIR=$${DESTDIR} /NOCD "\"/XOutFile $${DESTDIR}\\$${TARGET}-installer.exe\"" "$$SOURCE_DIR\\deploy\\qgroundcontrol_installer.nsi")
        OTHER_FILES += deploy/qgroundcontrol_installer.nsi
    }
    LinuxBuild {
        #-- TODO: This uses hardcoded paths. It should use $${DESTDIR}
        QMAKE_POST_LINK += && mkdir -p package
        QMAKE_POST_LINK += && tar -cj --exclude='package' -f package/QGroundControl.tar.bz2 staging --transform 's/$${DESTDIR}/qgroundcontrol/'
    }
    AndroidBuild {
        QMAKE_POST_LINK += && mkdir -p package
        QMAKE_POST_LINK += && make install INSTALL_ROOT=android-build/
        QMAKE_POST_LINK += && androiddeployqt --input android-libQGroundControl.so-deployment-settings.json --output android-build --deployment bundled --gradle --sign $${SOURCE_DIR}/android/android_release.keystore dagar --storepass $$(ANDROID_STOREPASS)
        contains(QT_ARCH, arm) {
            QGC_APK_BITNESS = "32"
        } else:contains(QT_ARCH, arm64) {
            QGC_APK_BITNESS = "64"
        } else {
            QGC_APK_BITNESS = ""
        }
        QMAKE_POST_LINK += && cp android-build/build/outputs/apk/android-build-release-signed.apk package/QGroundControl$${QGC_APK_BITNESS}.apk
    }
}
