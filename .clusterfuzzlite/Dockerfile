FROM gcr.io/oss-fuzz-base/base-builder:v1

# The base image's own Python serves OSS-Fuzz's tooling. QGC's scripts run on the Python named in
# tools/.python-version: uv installs it under /opt and finds the pin from any directory below $SRC.
COPY --from=ghcr.io/astral-sh/uv:0.12.8 /uv /usr/local/bin/uv
COPY tools/.python-version $SRC/.python-version
ENV UV_PYTHON_INSTALL_DIR=/opt/qgc-python
RUN uv python install --no-bin "$(cat "$SRC/.python-version")"

COPY cmake/CustomOptions.cmake $SRC/qgroundcontrol/cmake/CustomOptions.cmake
COPY test/Fuzz $SRC/qgroundcontrol/test/Fuzz
COPY .clusterfuzzlite/read_mavlink_config.py $SRC/read_mavlink_config.py

RUN set -eux; \
    mavlink_repository="$(uv run --no-project python "$SRC/read_mavlink_config.py" "$SRC/qgroundcontrol/cmake/CustomOptions.cmake" GIT_REPO)"; \
    mavlink_ref="$(uv run --no-project python "$SRC/read_mavlink_config.py" "$SRC/qgroundcontrol/cmake/CustomOptions.cmake" GIT_TAG)"; \
    test -n "$mavlink_repository"; \
    test -n "$mavlink_ref"; \
    git init "$SRC/mavlink"; \
    git -C "$SRC/mavlink" fetch --depth=1 "$mavlink_repository" "$mavlink_ref"; \
    git -C "$SRC/mavlink" checkout --detach FETCH_HEAD; \
    git -C "$SRC/mavlink" submodule update --init --depth=1 pymavlink

COPY .clusterfuzzlite/build.py $SRC/build.py
COPY .clusterfuzzlite/build.sh $SRC/build.sh
WORKDIR $SRC/qgroundcontrol
