# EarFix — Signing & Notarization

Distributable macOS builds are signed with a **Developer ID Application** certificate,
notarized by Apple, and stapled, so the AU/VST3 open on any Mac without Gatekeeper
warnings. Signing/notarization runs as a post-build step (`scripts/sign-and-notarize.sh`),
independent of Xcode's build-time signing.

## One-time setup

1. **Create the Developer ID Application certificate** (Account Holder only):
   Xcode → Settings → Accounts → team *Holger Schuler (3NC838977T)* →
   Manage Certificates → **+** → **Developer ID Application**.
   Verify:
   ```
   security find-identity -v -p codesigning | grep "Developer ID Application"
   ```

2. **Store notarization credentials** in a keychain profile (default name `earfix-notary`).
   Create an app-specific password at appleid.apple.com → Sign-In & Security →
   App-Specific Passwords, then:
   ```
   xcrun notarytool store-credentials "earfix-notary" \
       --apple-id "hey@holgerschueler.com" --team-id 3NC838977T \
       --password "xxxx-xxxx-xxxx-xxxx"
   ```

## Release build

```
scripts/sign-and-notarize.sh --build --version 1.4.0
```

Produces `release/EarFix-v1.4.0-macOS.zip` (signed, notarized, stapled AU + VST3).

## Notes

- Local development installs stay ad-hoc; only distribution builds use Developer ID.
- The script strips the `com.apple.FinderInfo` xattr that otherwise makes `codesign`
  fail (the long-standing "Plugin Copy Step" issue in BUILD_NOTES.md).
- AUv3 app-extension distribution (wrapped in a host app) is not covered here — the
  script signs the AU `.component` and VST3 only, matching prior releases.
