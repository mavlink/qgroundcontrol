# TODO: Merge changes upstream after testing AWS deployment
#
# Extends the original docker while prototyping and until we migrate our changes upstream
# TODO: Keep in sync with the one in custom/deploy/ci/linux/Dockerfile in prototyping phase
#
FROM mavlink/qgc-build-linux:2019-02-03

ENV DEBIAN_FRONTEND noninteractive

# Required by AppImage
RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y --no-install-recommends apt-utils
RUN apt-get install -y --no-install-recommends fuse
RUN apt-get install -y --no-install-recommends kmod
RUN apt-get install -y --no-install-recommends rsync

ENV DEBIAN_FRONTEND teletype
