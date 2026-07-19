#!/bin/bash
#
# EarFix — Developer ID signing + notarization + stapling for distribution.
#
# Signs the built AU (.component) and VST3 (.vst3) with a Developer ID Application
# certificate + hardened runtime, notarizes them with Apple, staples the ticket,
# and produces a distributable zip. Independent of Xcode's build-time signing, so
# it survives Projucer regenerating the .xcodeproj.
#
# PREREQUISITES (one-time, only you can do these — they need your Apple login):
#
#   1) Create a "Developer ID Application" certificate (Account Holder only):
#        Xcode > Settings > Accounts > select your team (Holger Schuler, 3NC838977T)
#        > Manage Certificates > "+" > "Developer ID Application"
#      Verify:  security find-identity -v -p codesigning | grep "Developer ID Application"
#
#   2) Store notarization credentials in a keychain profile named below:
#        # App-specific password (appleid.apple.com > Sign-In & Security):
#        xcrun notarytool store-credentials "$NOTARY_PROFILE" \
#            --apple-id "hey@holgerschueler.com" --team-id 3NC838977T \
#            --password "xxxx-xxxx-xxxx-xxxx"
#      (or use --key / --key-id / --issuer for an App Store Connect API key)
#
# USAGE:
#   scripts/sign-and-notarize.sh [--build] [--version X.Y.Z]
#     --build          run the Release xcodebuild first (AU + VST3)
#     --version X.Y.Z  label the output zip (default: reads CHANGELOG or "dev")
#
set -euo pipefail

# ---- Config (override via environment) ----
SIGN_ID="${SIGN_ID:-Developer ID Application: Holger Schueler (3NC838977T)}"
NOTARY_PROFILE="${NOTARY_PROFILE:-earfix-notary}"
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
XCODEPROJ="$PROJECT_DIR/Builds/MacOSX/EarFix.xcodeproj"
BUILD_DIR="$PROJECT_DIR/Builds/MacOSX/build/Release"

DO_BUILD=0
VERSION=""
while [ $# -gt 0 ]; do
    case "$1" in
        --build) DO_BUILD=1 ;;
        --version) VERSION="$2"; shift ;;
        *) echo "Unknown arg: $1"; exit 2 ;;
    esac
    shift
done
[ -z "$VERSION" ] && VERSION="dev"

STAGE="$PROJECT_DIR/release/EarFix-v${VERSION}-macOS"

echo "==> EarFix release signing"
echo "    identity : $SIGN_ID"
echo "    profile  : $NOTARY_PROFILE"
echo "    version  : $VERSION"

# ---- Preflight ----
if ! security find-identity -v -p codesigning | grep -q "Developer ID Application"; then
    echo "ERROR: no 'Developer ID Application' certificate in the keychain."
    echo "       Create it first (see PREREQUISITES at the top of this script)."
    exit 1
fi

# ---- Optional build ----
# NOTE: xcodebuild returns nonzero because the JUCE "Plugin Copy Step" fails on the
# com.apple.FinderInfo xattr — but compilation has already produced the products by
# then. So we build best-effort and verify the products exist afterwards instead of
# trusting the exit code.
if [ "$DO_BUILD" -eq 1 ]; then
    echo "==> Building Release (AU + VST3)…"
    xcodebuild -project "$XCODEPROJ" -scheme "EarFix - AU"   -configuration Release build >/tmp/earfix-build-au.log 2>&1 \
        || echo "   (AU xcodebuild exited nonzero — expected copy-step xattr issue; checking product)"
    xcodebuild -project "$XCODEPROJ" -scheme "EarFix - VST3" -configuration Release build >/tmp/earfix-build-vst3.log 2>&1 \
        || echo "   (VST3 xcodebuild exited nonzero — expected; checking product)"
fi

# ---- Stage the products in a NON-iCloud temp dir ----
# The project lives under ~/Desktop, which is iCloud-synced; iCloud keeps re-adding
# the com.apple.FinderInfo xattr that codesign rejects. Signing/notarizing in a plain
# temp dir avoids that race; only the finished artifacts are copied back to release/.
WORK="$(mktemp -d /private/tmp/earfix-release.XXXXXX)"
trap 'rm -rf "$WORK"' EXIT
WSTAGE="$WORK/EarFix-v${VERSION}-macOS"
mkdir -p "$WSTAGE"
COMPONENT="$WSTAGE/EarFix.component"
VST3="$WSTAGE/EarFix.vst3"

[ -d "$BUILD_DIR/EarFix.component" ] && ditto "$BUILD_DIR/EarFix.component" "$COMPONENT"
[ -d "$BUILD_DIR/EarFix.vst3" ]      && ditto "$BUILD_DIR/EarFix.vst3" "$VST3"
[ -d "$COMPONENT" ] || { echo "ERROR: no built EarFix.component in $BUILD_DIR (run with --build)"; exit 1; }

# ---- Sign (deep, hardened runtime, secure timestamp) ----
sign_bundle () {
    local target="$1"
    [ -d "$target" ] || return 0
    echo "==> Signing $(basename "$target")"
    xattr -cr "$target"                         # strip the FinderInfo detritus codesign rejects
    # Hardened runtime + secure timestamp, no entitlements (the host process governs
    # a loaded plugin; audio plugins need none for notarization).
    codesign --force --deep --options runtime --timestamp \
             --sign "$SIGN_ID" "$target"
    codesign --verify --deep --strict --verbose=2 "$target"
}
sign_bundle "$COMPONENT"
sign_bundle "$VST3"

# ---- Notarize (zip -> submit -> wait) ----
ZIP_TMP="$WORK/EarFix-v${VERSION}-macOS.zip"
echo "==> Zipping for notarization"
( cd "$WSTAGE/.." && ditto -c -k --keepParent "$(basename "$WSTAGE")" "$ZIP_TMP" )

echo "==> Submitting to Apple notary (this can take a few minutes)…"
xcrun notarytool submit "$ZIP_TMP" --keychain-profile "$NOTARY_PROFILE" --wait

# ---- Staple the tickets, then re-zip the stapled bundles ----
echo "==> Stapling"
[ -d "$COMPONENT" ] && xcrun stapler staple "$COMPONENT"
[ -d "$VST3" ]      && xcrun stapler staple "$VST3"

echo "==> Re-zipping stapled bundles"
rm -f "$ZIP_TMP"
( cd "$WSTAGE/.." && ditto -c -k --keepParent "$(basename "$WSTAGE")" "$ZIP_TMP" )

# ---- Final verification ----
echo "==> Verification"
[ -d "$COMPONENT" ] && { xcrun stapler validate "$COMPONENT"; spctl -a -vvv -t install "$COMPONENT" 2>&1 || true; }
[ -d "$VST3" ]      && xcrun stapler validate "$VST3"

# ---- Copy finished artifacts back to release/ ----
ZIP="$PROJECT_DIR/release/EarFix-v${VERSION}-macOS.zip"
echo "==> Copying artifacts to release/"
rm -rf "$STAGE" "$ZIP"
ditto "$WSTAGE" "$STAGE"          # stapled bundles (may pick up harmless iCloud xattrs)
cp "$ZIP_TMP" "$ZIP"             # the distributable zip (built clean in temp)

echo "==> Done. Distributable: $ZIP"
