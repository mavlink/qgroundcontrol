#!/usr/bin/env bash

set -euo pipefail

if (( $# != 3 )); then
    echo "Usage: $0 <app-bundle> <minimum-ios-version> <iphoneos|iphonesimulator>" >&2
    exit 2
fi

bundle_path=$1
minimum_ios_version=$2
platform=$3

case "$platform" in
    iphoneos | iphonesimulator) ;;
    *)
        echo "Unsupported iOS platform: $platform" >&2
        exit 2
        ;;
esac

if [[ ! -d "$bundle_path" || ! -f "$bundle_path/Info.plist" ]]; then
    echo "Invalid iOS app bundle: $bundle_path" >&2
    exit 2
fi

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
asset_catalog="$script_dir/Images.xcassets"
launch_screen="$script_dir/QGCLaunchScreen.storyboard"
bundle_plist="$bundle_path/Info.plist"
partial_plist=$(mktemp "$bundle_path/.qgc-ios-assets.XXXXXX")
trap 'rm -f "$partial_plist"' EXIT

xcrun=${XCRUN:-xcrun}
plist_buddy=${PLIST_BUDDY:-/usr/libexec/PlistBuddy}

"$xcrun" actool \
    --app-icon AppIcon \
    --output-partial-info-plist "$partial_plist" \
    --platform "$platform" \
    --target-device iphone \
    --target-device ipad \
    --minimum-deployment-target "$minimum_ios_version" \
    --compile "$bundle_path" \
    "$asset_catalog"

"$xcrun" ibtool \
    --errors \
    --warnings \
    --notices \
    --target-device iphone \
    --target-device ipad \
    --minimum-deployment-target "$minimum_ios_version" \
    --compile "$bundle_path/QGCLaunchScreen.storyboardc" \
    "$launch_screen"

"$plist_buddy" -c "Merge \"$partial_plist\"" "$bundle_plist"

rm -f "$bundle_path/QGCLaunchScreen.storyboard"

test -f "$bundle_path/Assets.car"
test -d "$bundle_path/QGCLaunchScreen.storyboardc"
