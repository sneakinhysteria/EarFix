# Installation Guide

## Download

Download the latest release from the [Releases page](https://github.com/sneakinhysteria/EarFix/releases).

The download contains:
- `EarFix.component` - Audio Unit plugin (macOS)
- `EarFix.vst3` - VST3 plugin (macOS/Windows)

---

## macOS Installation

### Audio Unit (AU) - Recommended for Logic Pro, GarageBand, MainStage

1. Open Finder
2. Press `Cmd + Shift + G` and enter: `~/Library/Audio/Plug-Ins/Components/`
3. Copy `EarFix.component` into this folder
4. Restart your DAW

**Alternative (Terminal):**
```bash
cp -R EarFix.component ~/Library/Audio/Plug-Ins/Components/
```

### VST3 - For Ableton Live, Cubase, Reaper, etc.

1. Open Finder
2. Press `Cmd + Shift + G` and enter: `~/Library/Audio/Plug-Ins/VST3/`
3. Copy `EarFix.vst3` into this folder
4. Restart your DAW

**Alternative (Terminal):**
```bash
mkdir -p ~/Library/Audio/Plug-Ins/VST3/
cp -R EarFix.vst3 ~/Library/Audio/Plug-Ins/VST3/
```

### System-wide Installation (All Users)

To install for all users on the system, use these paths instead:
- AU: `/Library/Audio/Plug-Ins/Components/`
- VST3: `/Library/Audio/Plug-Ins/VST3/`

Note: System-wide installation requires administrator privileges.

---

## Windows Installation

### System Requirements

- Windows 10 (version 1607 or later) or Windows 11
- 64-bit (x64) only — no 32-bit or ARM64 build; your DAW must be running as a 64-bit process
- [Microsoft Visual C++ Redistributable (x64)](https://aka.ms/vc14/vc_redist.x64.exe) — if EarFix doesn't appear after a rescan, install this first. Most DAWs already ship it, but a fresh Windows install may not have it.

Only VST3 is available on Windows (no AU or AAX).

### VST3

1. Open File Explorer
2. Navigate to `C:\Program Files\Common Files\VST3\`
3. Copy `EarFix.vst3` into this folder (this is a folder, not a single file — copy the whole thing)
4. Restart your DAW and rescan plugins

**Alternative (PowerShell):**
```powershell
Copy-Item -Recurse EarFix.vst3 "C:\Program Files\Common Files\VST3\"
```

---

## Gatekeeper / Security Notice (macOS only)

EarFix releases are signed with a Developer ID and notarized by Apple, so this shouldn't normally trigger — but if macOS still blocks the plugin:

### Method 1: System Preferences
1. Open **System Preferences** > **Security & Privacy** > **General**
2. Click **"Allow Anyway"** next to the EarFix message
3. Rescan plugins in your DAW

### Method 2: Terminal (Remove Quarantine)
```bash
xattr -cr ~/Library/Audio/Plug-Ins/Components/EarFix.component
xattr -cr ~/Library/Audio/Plug-Ins/VST3/EarFix.vst3
```

### Method 3: Right-Click Open
1. In Finder, navigate to the plugin location
2. Right-click (or Control-click) on the plugin
3. Select "Open" from the context menu
4. Click "Open" in the dialog

---

## DAW-Specific Instructions

### Logic Pro
1. Install the AU version
2. Open Logic Pro
3. Go to **Logic Pro** > **Preferences** > **Plug-in Manager**
4. Click **"Reset & Rescan Selection"** or restart Logic
5. Find EarFix under **Audio FX** > **BrighterRealities** > **EarFix**

### Ableton Live
1. Install the VST3 version
2. Open Ableton Live
3. Go to **Preferences** > **Plug-ins**
4. Enable **"Use VST3 Plug-in System Folder"**
5. Click **"Rescan"**
6. Find EarFix in the **Plug-ins** browser under **Audio Effects**

### GarageBand
1. Install the AU version
2. Open GarageBand
3. Insert on a track via **Smart Controls** > **Plug-ins** > **Audio Units** > **BrighterRealities**

### Reaper (macOS & Windows)
1. Install the VST3 version
2. Open Reaper
3. Go to **Options** > **Preferences** > **Plug-ins** > **VST**
4. Click **"Re-scan"**
5. Find EarFix in the FX browser

### Cubase / Nuendo (Windows)
1. Install the VST3 version
2. Open Cubase/Nuendo
3. Go to **Studio** > **VST Plug-in Manager**
4. Click **"Update"** or restart Cubase
5. Find EarFix under **Audio** > **BrighterRealities**

### FL Studio (Windows)
1. Install the VST3 version
2. Open FL Studio
3. Go to **Options** > **Manage Plugins**
4. Click **"Find plugins"** (or it will auto-detect on next launch)
5. Find EarFix in the plugin picker

### Studio One (Windows)
1. Install the VST3 version
2. Open Studio One
3. Go to **Studio One** > **Options** > **Locations** > **VST Plug-ins**
4. Click **"Reset & Rescan"**
5. Find EarFix under **Effects** > **BrighterRealities**

Pro Tools (AAX) is not supported — see the [README](README.md#supported-formats) for why.

---

## Uninstallation

To remove EarFix, simply delete the plugin files.

**macOS:**
```bash
rm -rf ~/Library/Audio/Plug-Ins/Components/EarFix.component
rm -rf ~/Library/Audio/Plug-Ins/VST3/EarFix.vst3
```

**Windows (PowerShell):**
```powershell
Remove-Item -Recurse "C:\Program Files\Common Files\VST3\EarFix.vst3"
```

---

## Troubleshooting

### Plugin doesn't appear in DAW
- Ensure the plugin is in the correct folder (see above — the paths differ between macOS and Windows)
- Rescan plugins in your DAW preferences
- **macOS**: check that Gatekeeper isn't blocking it (see Gatekeeper section)
- **Windows**: make sure your DAW is running as a 64-bit process, and that the [VC++ Redistributable](https://aka.ms/vc14/vc_redist.x64.exe) is installed
- Try restarting your DAW

### Plugin validation fails (macOS)
Run the AU validation tool to check for issues:
```bash
auval -v aufx Earx Brtr
```
(This is a macOS-only tool; there's no equivalent for the Windows VST3 build.)

### Audio crackles or dropouts
- Increase your DAW's audio buffer size
- Ensure your CPU isn't overloaded
- Try disabling other plugins temporarily

### Need help?
Open an issue on [GitHub](https://github.com/sneakinhysteria/EarFix/issues).
