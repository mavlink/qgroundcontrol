FROM gcr.io/oss-fuzz-base/base-builder:v1

COPY cmake/CustomOptions.cmake $SRC/qgroundcontrol/cmake/CustomOptions.cmake
COPY test/Fuzz $SRC/qgroundcontrol/test/Fuzz

RUN set -eux; \
    mavlink_repository="$(sed -n 's/^set(QGC_MAVLINK_GIT_REPO "\([^"]*\)".*/\1/p' "$SRC/qgroundcontrol/cmake/CustomOptions.cmake")"; \
    mavlink_ref="$(sed -n 's/^set(QGC_MAVLINK_GIT_TAG "\([^"]*\)".*/\1/p' "$SRC/qgroundcontrol/cmake/CustomOptions.cmake")"; \
    test -n "$mavlink_repository"; \
    test -n "$mavlink_ref"; \
    git init "$SRC/mavlink"; \
    git -C "$SRC/mavlink" fetch --depth=1 "$mavlink_repository" "$mavlink_ref"; \
    git -C "$SRC/mavlink" checkout --detach FETCH_HEAD; \
    git -C "$SRC/mavlink" submodule update --init --depth=1 pymavlink

COPY .clusterfuzzlite/build.sh $SRC/build.sh
WORKDIR $SRC/qgroundcontrol
