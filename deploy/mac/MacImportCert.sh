#!/bin/sh
KEY_CHAIN=mac-travis-build.keychain
security create-keychain -p travis $KEY_CHAIN
security default-keychain -s $KEY_CHAIN
security unlock-keychain -p travis $KEY_CHAIN
security set-keychain-settings -t 3600 -u $KEY_CHAIN
security import deploy/MacCertificates.p12 -k $KEY_CHAIN -P $MAC_CERT_PASSWORD -T /usr/bin/codesign
security set-key-partition-list -S apple-tool:,apple: -s -k travis $KEY_CHAIN
security list-keychains -s $KEY_CHAIN