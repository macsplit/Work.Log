# Contributing to Work.Log

Thank you for your interest in contributing to Work.Log! This document provides guidelines for contributing to the project.

## How to Contribute

### Reporting Bugs

1. Check if the bug has already been reported in [Issues](https://github.com/macsplit/Work.Log/issues)
2. If not, create a new issue with:
   - Clear, descriptive title
   - Steps to reproduce the bug
   - Expected vs actual behavior
   - Your environment (OS, app version, desktop/web)
   - Screenshots if applicable

### Suggesting Features

1. Check existing issues for similar suggestions
2. Create a new issue with the "enhancement" label
3. Describe the feature and its use case
4. Explain why it would be useful

### Pull Requests

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/your-feature-name`
3. Make your changes
4. Test your changes on both platforms if applicable
5. Commit with clear messages (see below)
6. Push to your fork
7. Open a Pull Request

## Development Setup

### Desktop App (KDE Kirigami)

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install build-essential cmake extra-cmake-modules \
    qtbase5-dev qtdeclarative5-dev qtquickcontrols2-5-dev \
    libqt5sql5-sqlite kirigami2-dev libkf5i18n-dev libkf5coreaddons-dev

# Build
cd WorkLog.Desktop
mkdir -p build && cd build
cmake ..
make
```

### Web App (.NET)

```bash
cd WorkLog.Web/Website
dotnet restore
dotnet run
```

Or with Docker:
```bash
cd WorkLog.Web
docker compose up --build
```

## Commit Message Guidelines

Use conventional commit style:

- `feat:` New feature
- `fix:` Bug fix
- `refactor:` Code refactoring
- `docs:` Documentation changes
- `style:` Formatting, no code change
- `test:` Adding tests
- `chore:` Maintenance tasks

Examples:
- `feat: Add tag filtering to session list`
- `fix: Correct week number calculation for Week 0`
- `docs: Update installation instructions`

## Code Style

### Desktop (C++/QML)
- Follow KDE/Qt coding conventions
- Use meaningful variable names
- Add comments for complex logic

### Web (.NET/C#)
- Follow .NET conventions
- Use nullable reference types
- Keep controllers thin, logic in services

## Testing

- Desktop: Run the built executable and test manually
- Web: Access http://localhost:5000 (dotnet) or http://localhost:5080 (Docker)
- Test both light and dark modes
- Test the cloud sync feature if you have AWS configured

## Questions?

Feel free to open an issue for questions or discussion.
