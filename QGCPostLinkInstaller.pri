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
        #QMAKE_POST_LINK += && rsync -a --delete $BUILT_PRODUCTS_DIR/$${TARGET}.app .
        VideoEnabled {
            # Install the gstreamer framework
            # This will:
            # Copy from the original distibution into DESTDIR/gstwork (if not already there)
            # Prune the framework, removing stuff we don't need
            # Relocate all dylibs so they can work under @executable_path/...
            # Copy the result into the app bundle
            # Make sure qgroundcontrol can find them
            QMAKE_POST_LINK += && $$SOURCE_DIR/tools/prepare_gstreamer_framework.sh $${OUT_PWD}/gstwork/ $${TARGET}.app $${TARGET}
        }

        QMAKE_POST_LINK += && echo macdeployqt
        QMAKE_POST_LINK += && $$dirname(QMAKE_QMAKE)/macdeployqt $${TARGET}.app -appstore-compliant -verbose=1 -qmldir=$${SOURCE_DIR}/src

        # macdeployqt is missing some relocations once in a while. "Fix" it:
        QMAKE_POST_LINK += && echo osxrelocator
        QMAKE_POST_LINK += && python $$SOURCE_DIR/tools/osxrelocator.py $${TARGET}.app/Contents @rpath @executable_path/../Frameworks -r > /dev/null 2>&1

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
        QMAKE_POST_LINK += $$escape_expand(\\n) $$quote("\"C:\\Program Files \(x86\)\\NSIS\\makensis.exe\"" $$(QGC_NSIS_INSTALLER_PARAMETERS) /DDRIVER_MSI="\"$${QGC_INSTALLER_DRIVER_MSI}\"" /DINSTALLER_ICON="\"$${QGC_INSTALLER_ICON}\"" /DHEADER_BITMAP="\"$${QGC_INSTALLER_HEADER_BITMAP}\"" /DAPPNAME="\"$${QGC_APP_NAME}\"" /DEXENAME="\"$${TARGET}\"" /DORGNAME="\"$${QGC_ORG_NAME}\"" /DDESTDIR=$${DESTDIR} /NOCD "\"/XOutFile $${DESTDIR}\\$${TARGET}-installer.exe\"" "\"$${QGC_INSTALLER_SCRIPT}\"")
        OTHER_FILES += $${QGC_INSTALLER_SCRIPT}
    }
    LinuxBuild {
        #-- TODO: This uses hardcoded paths. It should use $${DESTDIR}
        QMAKE_POST_LINK += && mkdir -p package
        QMAKE_POST_LINK += && tar -cj --exclude='package' -f package/QGroundControl.tar.bz2 staging --transform 's/$${DESTDIR}/qgroundcontrol/'
    }
    AndroidBuild {
        _ANDROID_KEYSTORE_PASSWORD = $$(ANDROID_KEYSTORE_PASSWORD)
        isEmpty(_ANDROID_KEYSTORE_PASSWORD) {
            message(Skipping androiddeployqt since keystore password is not available)
        } else {
            QMAKE_POST_LINK += && mkdir -p package
            QMAKE_POST_LINK += && make apk_install_target INSTALL_ROOT=android-build
            QMAKE_POST_LINK += && androiddeployqt --verbose --input android-QGroundControl-deployment-settings.json --output android-build --release --sign $${SOURCE_DIR}/android/android_release.keystore QGCAndroidKeyStore --storepass $$(ANDROID_KEYSTORE_PASSWORD)
            QMAKE_POST_LINK += && cp android-build/build/outputs/apk/release/android-build-release-signed.apk package/QGroundControl$${ANDROID_TRUE_BITNESS}.apk
        }
    }
}
