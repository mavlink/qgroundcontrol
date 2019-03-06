# TODO: Merge changes upstream after testing AWS deployment
#
# Extends the original docker while prototyping and until we migrate our changes upstream
# TODO: Keep in sync with the one in custom/deploy/ci/linux/Dockerfile in prototyping phase
#
FROM mavlink/qgc-build-android:2019-02-03

RUN mkdir -p /home/jenkins/dev

# Install required dependencyes for APK buid
RUN sdkmanager --install 'build-tools;25.0.0'

ENV DEBIAN_FRONTEND teletype

VOLUME /home/jenkins/dev
