#!/bin/sh

# Create a custom keychain
security create-keychain -p travis ios-build.keychain

# Make the custom keychain default, so xcodebuild will use it for signing
security default-keychain -s ios-build.keychain

# Unlock the keychain
security unlock-keychain -p travis ios-build.keychain

# Set keychain timeout to 1 hour for long builds
# see http://www.egeek.me/2013/02/23/jenkins-and-xcode-user-interaction-is-not-allowed/
security set-keychain-settings -t 3600 -l ~/Library/Keychains/ios-build.keychain

# Add certificates to keychain and allow codesign to access them
security import ./deploy/iosCerts/apple.cer -k ~/Library/Keychains/ios-build.keychain -T /usr/bin/codesign
security import ./deploy/iosCerts/dist.cer -k ~/Library/Keychains/ios-build.keychain -T /usr/bin/codesign
security import ./deploy/iosCerts/dist.p12 -k ~/Library/Keychains/ios-build.keychain -P $IOS_PASSWORD -T /usr/bin/codesign


# Put the provisioning profile in place
mkdir -p ~/Library/MobileDevice/Provisioning\ Profiles
cp "./deploy/iosCerts/QGroundControl_Distribution.mobileprovision" ~/Library/MobileDevice/Provisioning\ Profiles/