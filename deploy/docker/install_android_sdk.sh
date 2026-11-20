#!/bin/bash
# Install the Android SDK packages that .github/build-config.json pins under ANDROID_SDK_ROOT
# (default /opt/android-sdk): the command-line tools, platform-tools, the platform, build-tools
# and the NDK. Then append the SDK, NDK, build-tools and JDK locations, and the minimum SDK level the
# Android presets read, to the profile script named by $1. Needs the JDK, wget, unzip, and
# read_config.py under /tmp/tools/setup.
set -euo pipefail

profile="$1"
sdk="${ANDROID_SDK_ROOT:-/opt/android-sdk}"
# shellcheck source=/dev/null
source <(python3 /tmp/tools/setup/read_config.py --export bash)

archive="$(mktemp --suffix .zip)"
wget -q "https://dl.google.com/android/repository/commandlinetools-linux-${ANDROID_CMDLINE_TOOLS}_latest.zip" -O "${archive}"
echo "${ANDROID_CMDLINE_TOOLS_SHA256}  ${archive}" | sha256sum -c -
mkdir -p "${sdk}/cmdline-tools"
unzip -q "${archive}" -d "${sdk}/cmdline-tools"
mv "${sdk}/cmdline-tools/cmdline-tools" "${sdk}/cmdline-tools/latest"
rm "${archive}"

sdkmanager="${sdk}/cmdline-tools/latest/bin/sdkmanager"
# sdkmanager closes the license prompt's pipe, so yes's SIGPIPE is not a failure.
(set +o pipefail; yes | "${sdkmanager}" --sdk_root="${sdk}" --licenses >/dev/null)
"${sdkmanager}" --sdk_root="${sdk}" \
    "platform-tools" \
    "platforms;android-${ANDROID_PLATFORM}" \
    "build-tools;${ANDROID_BUILD_TOOLS}" \
    "ndk;${ANDROID_NDK_FULL_VERSION}"

java_home="$(dirname "$(dirname "$(readlink -f "$(command -v javac)")")")"
cat >> "${profile}" <<EOF
export ANDROID_SDK_ROOT=${sdk}
export ANDROID_HOME=${sdk}
export ANDROID_NDK_ROOT=${sdk}/ndk/${ANDROID_NDK_FULL_VERSION}
export ANDROID_BUILD_TOOLS_DIR=${sdk}/build-tools/${ANDROID_BUILD_TOOLS}
export ANDROID_MIN_SDK=${ANDROID_MIN_SDK}
export JAVA_HOME=${java_home}
export PATH=${sdk}/cmdline-tools/latest/bin:${sdk}/platform-tools:\$PATH
EOF
