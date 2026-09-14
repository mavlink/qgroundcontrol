FROM gcr.io/oss-fuzz-base/base-builder:v1

COPY cmake/CustomOptions.cmake $SRC/qgroundcontrol/cmake/CustomOptions.cmake
COPY test/Fuzz $SRC/qgroundcontrol/test/Fuzz
COPY .clusterfuzzlite/read_mavlink_config.py $SRC/read_mavlink_config.py

RUN set -eux; \
    mavlink_repository="$(python3 "$SRC/read_mavlink_config.py" "$SRC/qgroundcontrol/cmake/CustomOptions.cmake" GIT_REPO)"; \
    mavlink_ref="$(python3 "$SRC/read_mavlink_config.py" "$SRC/qgroundcontrol/cmake/CustomOptions.cmake" GIT_TAG)"; \
    test -n "$mavlink_repository"; \
    test -n "$mavlink_ref"; \
    git init "$SRC/mavlink"; \
    git -C "$SRC/mavlink" fetch --depth=1 "$mavlink_repository" "$mavlink_ref"; \
    git -C "$SRC/mavlink" checkout --detach FETCH_HEAD; \
    git -C "$SRC/mavlink" submodule update --init --depth=1 pymavlink

COPY .clusterfuzzlite/build.py $SRC/build.py
COPY .clusterfuzzlite/build.sh $SRC/build.sh
WORKDIR $SRC/qgroundcontrol
