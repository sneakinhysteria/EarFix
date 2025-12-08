# Installation Guide

## Download

Download the latest release from the [Releases page](https://github.com/BrighterRealities/EarFix/releases).

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

## Gatekeeper / Security Notice

On first launch, macOS may block the plugin because it's not notarized. To allow it:

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

### Reaper
1. Install the VST3 version
2. Open Reaper
3. Go to **Options** > **Preferences** > **Plug-ins** > **VST**
4. Click **"Re-scan"**
5. Find EarFix in the FX browser

### Pro Tools (AAX)
1. Install the AAX version to `/Library/Application Support/Avid/Audio/Plug-Ins/`
2. Restart Pro Tools
3. Find EarFix in the plugin insert menu

---

## Uninstallation

To remove EarFix, simply delete the plugin files:

```bash
rm -rf ~/Library/Audio/Plug-Ins/Components/EarFix.component
rm -rf ~/Library/Audio/Plug-Ins/VST3/EarFix.vst3
```

---

## Troubleshooting

### Plugin doesn't appear in DAW
- Ensure the plugin is in the correct folder
- Rescan plugins in your DAW preferences
- Check that macOS security isn't blocking it (see Gatekeeper section)
- Try restarting your DAW

### Plugin validation fails
Run the AU validation tool to check for issues:
```bash
auval -v aufx Earx Brtr
```

### Audio crackles or dropouts
- Increase your DAW's audio buffer size
- Ensure your CPU isn't overloaded
- Try disabling other plugins temporarily

### Need help?
Open an issue on [GitHub](https://github.com/BrighterRealities/EarFix/issues).
