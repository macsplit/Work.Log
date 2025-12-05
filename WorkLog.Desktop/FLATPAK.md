# Building Work Log as a Flatpak

This document explains how to build and test Work Log Desktop as a Flatpak package.

## Prerequisites

Install flatpak-builder:

```bash
# Ubuntu/Debian
sudo apt install flatpak-builder

# Fedora
sudo dnf install flatpak-builder

# Arch Linux
sudo pacman -S flatpak-builder
```

Add Flathub repository (if not already added):

```bash
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

Install the KDE runtime and SDK:

```bash
flatpak install flathub org.kde.Platform//5.15-23.08 org.kde.Sdk//5.15-23.08
```

## Building

Build the Flatpak:

```bash
flatpak-builder --force-clean build-dir work.worklog.desktop.json
```

## Testing

Install and run the built Flatpak:

```bash
flatpak-builder --user --install --force-clean build-dir work.worklog.desktop.json
flatpak run work.worklog.desktop
```

## Build Configuration

The Flatpak build **disables cloud sync** via `-DENABLE_SYNC=OFF`. This means:

- No network access required or requested
- SyncManager code is not compiled
- Sync UI is not shown in the application
- The app is completely standalone and offline-capable

This is intentional to align with Flatpak/Flathub philosophy of minimal permissions and no mandatory external dependencies.

## Sandbox Permissions

The app requests the following permissions:

- `--share=ipc` - Required for Qt applications
- `--socket=fallback-x11` - X11 display access (fallback if Wayland unavailable)
- `--socket=wayland` - Wayland display access (preferred)
- `--device=dri` - GPU acceleration for rendering
- `--filesystem=xdg-data/WorkLog:create` - Access to app data directory (~/.local/share/WorkLog)
- `--filesystem=xdg-config/WorkLog:create` - Access to app config directory (~/.config/WorkLog)

**Note:** Network access is NOT requested. The Flatpak version is fully offline.

## Data Location

Inside the Flatpak sandbox, the SQLite database is stored at:
- `~/.var/app/work.worklog.desktop/data/WorkLog/worklog.db`

## Submitting to Flathub

To submit this app to Flathub:

1. Fork the Flathub repository: https://github.com/flathub/flathub
2. Create a new repository named `work.worklog.desktop`
3. Add `work.worklog.desktop.json` and `flathub.json`
4. Create a pull request to the Flathub repository
5. Address reviewer feedback

See: https://docs.flathub.org/docs/for-app-authors/submission/

## Source Repository vs Flatpak Build

The source repository contains optional sync functionality (via AWS DynamoDB) for users who build from source. However, the Flatpak build **explicitly disables** this feature via CMake configuration (`-DENABLE_SYNC=OFF`).

This approach allows:
- Users building from source can enable sync if they want it
- Flatpak users get a clean, offline-only experience
- No external dependencies or network access required for Flatpak
- Clear separation between optional features and core functionality

To build with sync enabled (non-Flatpak builds):
```bash
cmake -DENABLE_SYNC=ON ..
```

To build without sync (Flatpak default):
```bash
cmake -DENABLE_SYNC=OFF ..
```
