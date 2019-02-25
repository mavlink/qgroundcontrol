# TODO: Merge changes upstream after testing AWS deployment
#
# Extends the original docker while prototyping and until we migrate our changes upstream
# TODO: Keep in sync with the one in custom/deploy/ci/linux/Dockerfile in prototyping phase
#
FROM mavlink/qgc-build-linux:2019-02-03

ENV DEBIAN_FRONTEND noninteractive

ENV QGC_CUSTOM_APP_NAME="AGS"
ENV QGC_CUSTOM_GENERIC_NAME="Auterion Ground Station"
ENV QGC_CUSTOM_BINARY_NAME="AuterionGS"
ENV QGC_CUSTOM_LINUX_START_SH="$(pwd)/custom/deploy/qgroundcontrol-start.sh"
ENV QGC_CUSTOM_APP_ICON="$(pwd)/custom/res/src/Auterion_Icon.png"
ENV QGC_CUSTOM_APP_ICON_NAME="Auterion_Icon"

# Required by AppImage
RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends apt-utils
RUN apt-get install -y --no-install-recommends fuse
RUN apt-get install -y --no-install-recommends kmod
RUN apt-get install -y --no-install-recommends rsync

ENV DEBIAN_FRONTEND teletype
