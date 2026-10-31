# Initial cache (cmake -C) of the APK this action boot-tests, which android.yml's emulator leg builds and does not
# publish: QGroundControl runs its simple boot test at every launch and logs the Main category, which reports the
# result, with the startup categories the desktop boot test logs. A release APK ignores arguments `am start` passes,
# and the workflow's configure arguments cannot hold the space between them.
set(QT_ANDROID_APPLICATION_ARGUMENTS
    "--simple-boot-test --logging:Main,API.QGCApplication,Video.VideoManager,Video.GStreamer"
    CACHE STRING "Arguments QGroundControl gets at every launch"
)
