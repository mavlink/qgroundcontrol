#!/bin/bash

set -ex

setup_env() {
  # Travis sets CC/CXX to the system toolchain, so our .travis.yml
  # exports USE_{CC,CXX} for this script to use.
  if [ -n "$USE_CC" ]; then
      export CC=$USE_CC
  fi
  if [ -n "$USE_CXX" ]; then
      export CXX=$USE_CXX
  fi
  # Use -jN for faster builds. Travis build machines under Docker
  # have a lot of cores, but are memory-limited, so the kernel
  # will OOM if we try to use them all, so use at most 4.
  # See https://github.com/travis-ci/travis-ci/issues/1972
  export NCPUS=$(getconf _NPROCESSORS_ONLN)
  export JOBS=$(( $NCPUS < 4 ? $NCPUS : 4 ))
}

# We have to do this by hand rather than use the coverity addon because of
# matrix explosion: https://github.com/travis-ci/travis-ci/issues/1975
# We also do it by hand because when we're throttled, the addon will exit
# the build immediately and skip the main script!
coverity_scan() {
  if [ "${COVERITY_SCAN}" != "true" ] || \
     [ -n "${TRAVIS_TAG}" ] || \
     [ "${TRAVIS_PULL_REQUEST}" = "true" ]
  then
    echo "Skipping coverity scan."
    return
  fi

  export COVERITY_SCAN_PROJECT_NAME="${TRAVIS_REPO_SLUG}"
  export COVERITY_SCAN_NOTIFICATION_EMAIL="google-breakpad-dev@googlegroups.com"
  export COVERITY_SCAN_BUILD_COMMAND="make -j${JOBS}"
  export COVERITY_SCAN_BUILD_COMMAND_PREPEND="git clean -q -x -d -f; git checkout -f; ./configure"
  export COVERITY_SCAN_BRANCH_PATTERN="master"

  curl -s "https://scan.coverity.com/scripts/travisci_build_coverity_scan.sh" | bash || :
}

# Do an in-tree build and make sure tests pass.
build() {
  ./configure
  make -j${JOBS} check VERBOSE=1
  make distclean
}

# Do an out-of-tree build and make sure we can create a release tarball.
build_out_of_tree() {
  mkdir -p build/native
  pushd build/native >/dev/null
  ../../configure
  make -j${JOBS} distcheck VERBOSE=1
  popd >/dev/null
}

main() {
  setup_env
  build
  build_out_of_tree

  # Do scans last as they like to dirty the tree and some tests
  # expect a clean tree (like code style checks).
  coverity_scan
}

main "$@"
