# TODO: Merge changes upstream after testing AWS deployment
#
# Extends the original docker while prototyping and until we migrate our changes upstream
# TODO: Keep in sync with the one in custom/deploy/ci/linux/Dockerfile in prototyping phase
#
FROM mavlink/qgc-build-android:2019-02-03

# Install required dependencyes for APK buid
RUN sdkmanager --install 'build-tools;25.0.0'

# Install required tools
RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends bzip2

# TODO: Remove temporary dev folder
RUN mkdir -p /home/jenkins/dev

# TODO: cache the download to a mounted persistent storage
ENV CI_ANDROID_GSTREAMER_LOCATION /tmp/gstreamer-1.0-android-universal-1.14.4
RUN cd /tmp && curl -LOJs -C - https://s3-us-west-2.amazonaws.com/qgroundcontrol/dependencies/gstreamer-1.0-android-universal-1.14.4.tar.bz2 && tar -jxf gstreamer-1.0-android-universal-1.14.4.tar.bz2 -C /tmp

ENV DEBIAN_FRONTEND teletype

VOLUME /home/jenkins/dev
