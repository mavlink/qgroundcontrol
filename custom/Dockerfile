#
# Build Environment for DataPilot (Android)
#
# Author: Dario Roethlisberger <dario.roethlisberger@gmail.com>
#
# From qgroundcontrol/custom 
#
# Build docker: docker build -t datapilot .
# Run command: docker run -it -v `dirname $(pwd)`:/home/docker1000/qgroundcontrol:rw datapilot
#

FROM ubuntu:16.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update \
	&& apt-get -y --quiet --no-install-recommends install \
        git \
        default-jdk \
        wget \
        build-essential \
	&& apt-get -y autoremove \
	&& apt-get clean autoclean \
	&& rm -rf /var/lib/apt/lists/{apt,dpkg,cache,log} /tmp/* /var/tmp/*

RUN adduser --disabled-password --gecos '' --uid 1000 docker1000
USER docker1000

RUN cd /home/docker1000 \
	&& wget --no-check-certificate http://www.grubba.com/dpbuild.tgz \
    && tar -xvzf dpbuild.tgz

ENTRYPOINT ln -f -s ~/dpbuild/gstreamer-1.0-android-x86-1.5.2 . && ~/qgroundcontrol/custom/build.sh build
WORKDIR "/home/docker1000/qgroundcontrol"
