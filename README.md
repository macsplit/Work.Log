# Work.Log

A dual-platform work session tracking application with a native KDE Plasma desktop app and a self-hosted web application.

## Features

- **Hierarchical Navigation**: Browse sessions by Year > Month > Week > Day
- **Work Sessions**: Track work with time (0.5 hour increments), description, notes, and next planned stage
- **Cross-Platform**: Native KDE desktop app + Docker-ready web app
- **Self-Hosted**: Web app designed for self-hosting with SQLite database
- **Secure Authentication**: Web app uses bcrypt for password hashing

## Project Structure

```
Work.Log/
├── Core/                      # Shared schema and documentation
│   ├── schema.sql            # SQLite database schema
│   └── README.md             # Data model documentation
├── WorkLog.Desktop/          # KDE Plasma Kirigami app
│   ├── src/
│   │   ├── cpp/              # C++ backend (database, models)
│   │   └── qml/              # QML frontend (UI)
│   ├── resources/            # Qt resources
│   └── CMakeLists.txt        # CMake build configuration
├── WorkLog.Web/              # .NET web application
│   ├── Domain/               # Domain layer
│   │   ├── Entities/         # Data models
│   │   ├── Services/         # Business logic
│   │   └── Data/             # DbContext
│   ├── Website/              # ASP.NET MVC
│   │   ├── Controllers/
│   │   ├── Views/
│   │   ├── Models/           # View models
│   │   └── wwwroot/          # Static files
│   ├── Dockerfile
│   ├── docker-compose.yml    # Development
│   └── docker-compose.prod.yml # Production
├── CLAUDE.md                 # AI assistant instructions
└── README.md                 # This file
```

---

## Desktop Application (Kirigami)

### Dependencies

Install on Ubuntu/Debian:
```bash
sudo apt install \
    build-essential \
    cmake \
    extra-cmake-modules \
    qtbase5-dev \
    qtdeclarative5-dev \
    qtquickcontrols2-5-dev \
    libqt5sql5-sqlite \
    kirigami2-dev \
    libkf5i18n-dev \
    libkf5coreaddons-dev
```

Install on Fedora:
```bash
sudo dnf install \
    cmake \
    extra-cmake-modules \
    qt5-qtbase-devel \
    qt5-qtdeclarative-devel \
    qt5-qtquickcontrols2-devel \
    kf5-kirigami2-devel \
    kf5-ki18n-devel \
    kf5-kcoreaddons-devel
```

Install on Arch Linux:
```bash
sudo pacman -S \
    cmake \
    extra-cmake-modules \
    qt5-base \
    qt5-declarative \
    qt5-quickcontrols2 \
    kirigami2 \
    ki18n \
    kcoreaddons
```

### Building

```bash
cd WorkLog.Desktop
mkdir -p build && cd build
cmake ..
make
```

### Running

```bash
./worklog-desktop
```

### Data Location

The desktop app stores its SQLite database at:
- Linux: `~/.local/share/WorkLog/worklog.db`

---

## Web Application (.NET)

### Dependencies

**Option A: Direct (.NET SDK)**
- .NET 8.0 SDK or later
- Install from: https://dotnet.microsoft.com/download

**Option B: Docker**
- Docker Engine 20.10+
- Docker Compose v2+

### Running with .NET SDK

```bash
cd WorkLog.Web/Website
dotnet restore
dotnet run
```

Access at: http://localhost:5000

### Running with Docker

**Development:**
```bash
cd WorkLog.Web
docker-compose up --build
```
Access at: http://localhost:5080

**Production:**
```bash
cd WorkLog.Web
docker-compose -f docker-compose.prod.yml up -d
```
Access at: http://localhost:8080

### Initial Setup

On first run, the web application enters **Initial Admin Mode**:

1. Navigate to the web app URL
2. You'll be redirected to the Initial Setup page
3. Create your administrator account (username + password, min 8 characters)
4. After setup, normal login is required

**Note**: Initial admin mode is only available before the first account is created.

### Data Persistence

- **Docker**: Data is stored in a Docker volume (`worklog-data` or `worklog-data-dev`)
- **Direct**: Database is stored in the application directory as `worklog.db`

To backup data when using Docker:
```bash
docker cp worklog-web:/data/worklog.db ./backup-worklog.db
```

---

## Data Model

### Work Session

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| SessionDate | Date | Yes | The date of the work session |
| TimeHours | Decimal | Yes | Hours worked (0.5 increments: 0.5, 1.0, 1.5, ...) |
| Description | Text | Yes | Description of work performed |
| Notes | Text | No | Additional notes |
| NextPlannedStage | Text | No | What's planned next |

### Hierarchy

Sessions are organized hierarchically:
- **Year** → **Month** → **Week** → **Day** → **Sessions**

This is a navigational hierarchy, not a calendar view.

---

## Configuration

### Web App Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `ASPNETCORE_ENVIRONMENT` | Production | Environment mode |
| `ConnectionStrings__DefaultConnection` | `Data Source=/data/worklog.db` | SQLite connection string |

### Docker Ports

| Service | Dev Port | Prod Port |
|---------|----------|-----------|
| WorkLog.Web | 5080 | 8080 |

---

## Security Notes

- Passwords are hashed using bcrypt with a cost factor of 12
- Web app uses cookie-based authentication with 7-day expiry
- No 2FA or password reset (by design for self-hosted simplicity)
- SQLite database should be backed up regularly
- For production, consider placing behind a reverse proxy with HTTPS

---

## Development

### Desktop App

The desktop app uses:
- **Qt 5.15** with Qt Quick/QML for the UI
- **KDE Frameworks 5** (Kirigami2) for the application framework
- **SQLite** for local data storage

Key files:
- `src/cpp/databasemanager.cpp` - Database operations
- `src/cpp/worksessionmodel.cpp` - Session list model
- `src/cpp/hierarchymodel.cpp` - Hierarchy navigation model
- `src/qml/main.qml` - Main application window

### Web App

The web app uses:
- **.NET 8** with ASP.NET Core MVC
- **Entity Framework Core** with SQLite
- **BCrypt.Net** for password hashing

Key files:
- `Domain/Services/AuthService.cs` - Authentication logic
- `Domain/Services/WorkSessionService.cs` - Session CRUD operations
- `Website/Controllers/` - MVC controllers
- `Website/Views/Home/Index.cshtml` - Main dashboard view

---

## License

[Add your license here]

---

## Contributing

[Add contribution guidelines here]
