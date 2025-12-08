# EarFix Build Notes

## Known Issue: Code Signing Fails in Xcode Post-Build Script

The JUCE-generated post-build script fails with:
```
resource fork, Finder information, or similar detritus not allowed
file with invalid attached data: Disallowed xattr com.apple.FinderInfo found on ...
```

This happens because `ditto` preserves extended attributes, and `codesign` refuses to sign files with certain xattrs.

**We have not found a way to fix this in Xcode project settings.**

## Workaround: Manual Install After Build

After building in Xcode (the build will fail at the signing step, but the binary is compiled):

### VST3
```bash
rm -rf ~/Library/Audio/Plug-Ins/VST3/EarFix.vst3
cp -R "Builds/MacOSX/build/Release/EarFix.vst3" ~/Library/Audio/Plug-Ins/VST3/
xattr -cr ~/Library/Audio/Plug-Ins/VST3/EarFix.vst3
codesign --force --sign "-" ~/Library/Audio/Plug-Ins/VST3/EarFix.vst3
```

### AU (Audio Unit)
```bash
rm -rf ~/Library/Audio/Plug-Ins/Components/EarFix.component
cp -R "Builds/MacOSX/build/Release/EarFix.component" ~/Library/Audio/Plug-Ins/Components/
xattr -cr ~/Library/Audio/Plug-Ins/Components/EarFix.component
codesign --force --sign "-" ~/Library/Audio/Plug-Ins/Components/EarFix.component
```

## Quick One-Liner (from project root)

```bash
# VST3
rm -rf ~/Library/Audio/Plug-Ins/VST3/EarFix.vst3 && cp -R Builds/MacOSX/build/Release/EarFix.vst3 ~/Library/Audio/Plug-Ins/VST3/ && xattr -cr ~/Library/Audio/Plug-Ins/VST3/EarFix.vst3 && codesign --force --sign "-" ~/Library/Audio/Plug-Ins/VST3/EarFix.vst3

# AU
rm -rf ~/Library/Audio/Plug-Ins/Components/EarFix.component && cp -R Builds/MacOSX/build/Release/EarFix.component ~/Library/Audio/Plug-Ins/Components/ && xattr -cr ~/Library/Audio/Plug-Ins/Components/EarFix.component && codesign --force --sign "-" ~/Library/Audio/Plug-Ins/Components/EarFix.component
```

## After Installing

Restart your DAW to reload the updated plugins.
