#!/bin/bash
# Test script for building Work Log Desktop as Flatpak

set -e

echo "==> Cleaning previous build..."
rm -rf flatpak-build-dir .flatpak-builder

echo "==> Building Flatpak..."
flatpak-builder --user --install-deps-from=flathub --force-clean flatpak-build-dir work.worklog.desktop.json

echo "==> Build complete!"
echo ""
echo "To install and test:"
echo "  flatpak-builder --user --install --force-clean flatpak-build-dir work.worklog.desktop.json"
echo "  flatpak run work.worklog.desktop"
echo ""
echo "To export as bundle:"
echo "  flatpak-builder --repo=repo --force-clean flatpak-build-dir work.worklog.desktop.json"
echo "  flatpak build-bundle repo work-log.flatpak work.worklog.desktop"
