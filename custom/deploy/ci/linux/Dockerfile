# TODO: Merge changes upstream after testing AWS deployment
#
# Extends the original docker while prototyping and until we migrate our changes upstream
# TODO: Keep in sync with the one in custom/deploy/ci/linux/Dockerfile in prototyping phase
#
FROM mavlink/qgc-build-linux:2019-02-03

ENV DEBIAN_FRONTEND noninteractive

# Required by AppImage
RUN apt-get update && apt-get upgrade -y && apt-get -y --no-install-recommends install \
        apt-utils \
        fuse \
        kmod \
        rsync \
        openssl \
        libssl-dev \
    && apt-get -y autoremove \
    && apt-get clean autoclean \
    && rm -rf /var/lib/apt/lists/{apt,dpkg,cache,log} /tmp/* /var/tmp/*

ENV DEBIAN_FRONTEND teletype
