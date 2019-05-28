# TODO: Merge changes upstream after testing AWS deployment
#
# Extends the original docker while prototyping and until we migrate our changes upstream
# TODO: Keep in sync with the one in custom/deploy/ci/linux/Dockerfile in prototyping phase
#
FROM mavlink/qgc-build-android:2019-02-03

# Install required dependencyes for APK buid
RUN sdkmanager --install 'build-tools;25.0.0'

# Install required tools
RUN apt-get update && apt-get upgrade -y && apt-get -y --no-install-recommends install \
        bzip2 \
    && apt-get -y autoremove \
    && apt-get clean autoclean \
    && rm -rf /var/lib/apt/lists/{apt,dpkg,cache,log} /tmp/* /var/tmp/*


# TODO: cache the download to a mounted persistent storage
ENV CI_ANDROID_GSTREAMER_LOCATION /tmp/gstreamer-1.0-android-universal-1.14.4
RUN cd /tmp && curl -LOJs -C - https://s3-us-west-2.amazonaws.com/qgroundcontrol/dependencies/gstreamer-1.0-android-universal-1.14.4.tar.bz2 && tar -jxf gstreamer-1.0-android-universal-1.14.4.tar.bz2 -C /tmp

ENV DEBIAN_FRONTEND teletype
