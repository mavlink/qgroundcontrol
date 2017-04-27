#!/bin/bash
set -ex

get_depot_tools() {
  cd
  git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
  PATH="$HOME/depot_tools:$PATH"
}

gclient_sync() {
  # Rename the source dir to match what gclient expects.
  srcdir=$(basename "$TRAVIS_BUILD_DIR")
  cd "${TRAVIS_BUILD_DIR}"/..
  mv "${srcdir}" src
  gclient config --unmanaged https://github.com/google/breakpad.git
  gclient sync
}

main() {
  get_depot_tools
  gclient_sync
}

main "$@"
