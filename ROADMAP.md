# Potential Cross-Platform Local App Roadmap

This document captures possible routes for making Work.Log available as local
applications beyond the current Linux desktop app, without weakening the core
project direction.

## Product Direction

Linux native is the first-class platform for Work.Log. The Kirigami desktop
application should remain the canonical local application unless that becomes
technically untenable.

The .NET web application is a secondary portability path. It is valuable because
self-hosting makes Work.Log reachable from almost any device, but it is not the
same product goal as native local applications.

## Current Architecture

- `WorkLog.Desktop` is a Qt 5/KF5/Kirigami C++/QML application.
- `WorkLog.Web` is an ASP.NET Core MVC application with a reusable-ish .NET
  domain project.
- `Core/schema.sql` is the shared SQLite schema and is the best existing
  cross-platform contract.
- Sync currently targets AWS DynamoDB and uses `CloudId`, `UpdatedAt`,
  `IsDeleted`, `TagCloudId`, and `SyncMetadata` fields.

## Key Finding

SQLite and DynamoDB sync are not the main blockers. The main blockers are UI
runtime portability, dependency packaging, platform-specific secure storage, and
keeping data/sync semantics identical across all clients.

Before adding more clients, the data contract should be made stricter:

- Use one versioned SQLite schema/migration path for all local clients.
- Make delete behavior consistent. The web app soft-deletes for sync, while the
  current desktop app hard-deletes sessions and tags.
- Keep `CloudId`, `UpdatedAt`, `IsDeleted`, and `TagCloudId` behavior identical
  across clients.
- Store sync credentials in platform secure storage where possible, not plain
  JSON config files.
- Add regression tests for create/edit/delete/sync conflict behavior before
  expanding the platform matrix.

## Route A: Linux-First Kirigami, Then Selective Ports

This is the recommended primary route.

Keep the Kirigami app as the canonical native application, modernize it, and
attempt ports only where the dependency stack is realistic.

### Why This Fits

- Preserves the Linux-native product identity.
- Reuses the existing C++/QML UI and local SQLite code.
- Aligns with KDE/Kirigami's strengths on Linux and Plasma Mobile.
- Gives a plausible Android path.

### Required Work

1. Stabilize the shared SQLite/sync contract.
2. Port the desktop app from Qt 5/KF5 to Qt 6/KF6.
3. Keep Linux Flatpak as the main distribution target.
4. Add an Android proof-of-concept after the Qt 6/KF6 port.
5. Treat Windows/macOS as packaging investigations, not guaranteed targets.

### Android Notes

Android should be treated as a real port, not a rebuild:

- Add a Craft Android blueprint/build path.
- Avoid linking Qt Widgets on Android where possible.
- Use Android-appropriate Qt Quick Controls/Kirigami styling.
- Bundle required Breeze icons explicitly.
- Verify SQLite paths, app lifecycle behavior, networking, OpenSSL, and AWS
  request signing.
- Move AWS credentials to Android secure storage before considering release.

### Windows/macOS Notes

Windows and macOS are possible, but the work is mainly packaging and dependency
management:

- Add Craft packaging.
- Bundle Kirigami, KDE Frameworks, Qt plugins, styles, icons, SQLite driver, and
  any TLS/OpenSSL dependencies.
- Verify app data paths and file permissions.
- Expect platform-specific CMake/C++ conditionals.

### iOS Notes

iOS should not be considered a credible target for the current Kirigami/KDE
Frameworks app without a separate proof-of-concept. Qt supports iOS, but the
current Work.Log desktop stack depends on KDE Frameworks/Kirigami pieces that
are much better documented and exercised on Linux, Android, Windows, and macOS.

If iOS becomes mandatory, expect a different app shell rather than a direct
port of the current Kirigami application.

## Route B: Avalonia/.NET Local App

This is the best route if the goal changes to "one native-ish codebase for
Linux, Windows, macOS, Android, and iOS."

### Why It Might Fit

- Avalonia has a real Linux story, unlike .NET MAUI.
- The .NET domain, SQLite, EF Core, and AWS sync logic can be reused or
  refactored from `WorkLog.Web/Domain`.
- It avoids the harder KDE/Kirigami packaging questions on non-Linux platforms.

### Costs

- The UI would be a rewrite into Avalonia views.
- Linux would be supported, but it would not feel as KDE-native as Kirigami.
- The existing QML UI would not be reused.
- Mobile UX would still require adaptation, testing, packaging, and secure
  storage work.

This route should only displace Kirigami if iOS or broad non-Linux native
support becomes more important than KDE-native Linux integration.

## Route C: WebView/Electron/Tauri Wrapper

This is not recommended as the canonical local-app route.

It can be useful as a convenience wrapper around the self-hosted web app, but it
does not solve the primary goal of a first-class Linux native local application.
It also does not directly package the current ASP.NET MVC app as a Cordova-style
mobile app without introducing a separate client/API architecture.

## Explicit Non-Goals

- Do not make .NET MAUI the primary route while Linux native is non-negotiable.
  Linux is not an official MAUI target.
- Do not rely on Cordova to package `WorkLog.Web` directly. The current web app
  is server-rendered ASP.NET MVC, not a static client application.
- Do not add per-platform data models. The SQLite schema and sync rules must be
  shared.

## Recommended Sequence

1. Fix desktop/web data contract drift, especially soft delete behavior.
2. Add schema versioning and migration tests around `Core/schema.sql`.
3. Port the Kirigami app to Qt 6/KF6 while keeping Linux Flatpak healthy.
4. Create a small Android proof-of-concept build that opens the app, creates a
   session, restarts, and verifies SQLite persistence.
5. Add secure credential storage and test DynamoDB sync on Android.
6. Only after Android is proven, evaluate Windows/macOS Craft packages.
7. Revisit Avalonia only if iOS or broad non-Linux native support becomes a
   firm requirement.

## Reference Documentation

- KDE Kirigami overview: https://develop.kde.org/frameworks/kirigami/
- KDE Frameworks overview: https://develop.kde.org/products/frameworks/
- KDE Android packaging: https://develop.kde.org/docs/packaging/android/
- KDE Android porting notes: https://develop.kde.org/docs/packaging/android/porting_applications/advanced/
- KDE Windows/Kirigami setup: https://develop.kde.org/docs/getting-started/kirigami/platforms-windows/
- KDE Craft build system: https://develop.kde.org/docs/getting-started/building/craft/
