# Testing Work Log Desktop Flatpak

## Quick Start

Once you have flatpak-builder and the KDE runtime installed, testing is simple:

```bash
cd WorkLog.Desktop
./flatpak-test-build.sh
```

Then install and run:

```bash
flatpak-builder --user --install --force-clean flatpak-build-dir work.worklog.desktop.json
flatpak run work.worklog.desktop
```

## Manual Testing Steps

### 1. Basic Functionality
- [ ] App launches without errors
- [ ] Can create a new work session
- [ ] Sessions are saved to database
- [ ] Can edit existing sessions
- [ ] Can delete sessions
- [ ] Hierarchy navigation works (Year > Month > Week > Day)
- [ ] Calendar date picker functions correctly
- [ ] Tag creation and assignment works
- [ ] Statistics display correctly (day/week/month totals)

### 2. Flatpak-Specific Tests
- [ ] Sync menu action is NOT visible (should be hidden when sync disabled)
- [ ] No network requests made (verify with `journalctl -f` while using app)
- [ ] Data persists between app restarts
- [ ] Database file created at: `~/.var/app/work.worklog.desktop/data/WorkLog/worklog.db`
- [ ] Config files at: `~/.var/app/work.worklog.desktop/config/WorkLog/`

### 3. Display Server Tests
- [ ] **Wayland**: App works on Wayland sessions
- [ ] **X11**: App works on X11 sessions
- [ ] **Mixed**: Can switch between sessions without issues

### 4. Desktop Environment Tests
Test on different DEs to ensure portability:
- [ ] KDE Plasma
- [ ] GNOME
- [ ] XFCE
- [ ] Other (specify): ___________

### 5. Icon and Metadata
- [ ] App icon displays correctly in launcher
- [ ] Desktop file metadata is correct
- [ ] App shows up in application menu under "Office" category

## Verifying Sync is Disabled

To confirm sync is properly disabled in the Flatpak build:

```bash
# Check if SyncManager symbols exist in the binary
flatpak run --command=sh work.worklog.desktop
strings /app/bin/worklog-desktop | grep -i sync
# Should show very few or no sync-related symbols

# Verify no network sockets are opened
# While app is running:
ss -tulpn | grep worklog
# Should show nothing (no listening ports)

# Check flatpak permissions
flatpak info --show-permissions work.worklog.desktop | grep network
# Should return nothing (no network permission)
```

## Performance Testing

- [ ] App starts in under 3 seconds
- [ ] Database queries are responsive (< 100ms for typical operations)
- [ ] UI animations are smooth
- [ ] Memory usage is reasonable (< 200MB typical)

## Data Verification

Check that data is properly sandboxed:

```bash
# Find the database
ls -lh ~/.var/app/work.worklog.desktop/data/WorkLog/

# Verify it's a valid SQLite database
file ~/.var/app/work.worklog.desktop/data/WorkLog/worklog.db

# Inspect database (optional)
flatpak run --command=sh work.worklog.desktop
sqlite3 ~/.local/share/WorkLog/worklog.db  # Note: path inside sandbox
```

## Common Issues

### Issue: App won't start
- Check: `flatpak run --verbose work.worklog.desktop`
- Look for missing dependencies or permission errors

### Issue: No data visible after install
- Expected: First run starts with empty database
- Create test data to verify functionality

### Issue: Icon not showing
- Run: `gtk-update-icon-cache /app/share/icons/hicolor/`
- Or: Log out and back in

## Exporting for Distribution

To create a distributable bundle:

```bash
flatpak-builder --repo=repo --force-clean flatpak-build-dir work.worklog.desktop.json
flatpak build-bundle repo work-log.flatpak work.worklog.desktop
```

The resulting `work-log.flatpak` file can be shared and installed with:

```bash
flatpak install work-log.flatpak
```

## Automated Testing (Future)

For CI/CD integration, consider:
- `flatpak-builder --verbose --disable-rofiles-fuse ...`
- `xvfb-run flatpak run --command=worklog-desktop work.worklog.desktop --test`
- Integration tests for database operations
