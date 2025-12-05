cmake_minimum_required(VERSION 3.16)
project(WorkLogDesktop VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Find ECM
find_package(ECM 5.80 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMSetupVersion)

# Build options
option(ENABLE_SYNC "Enable cloud sync functionality" ON)

# Use Qt5 with KF5 Kirigami (compatible with Ubuntu 24.04)
set(QT_COMPONENTS Core Quick Sql QuickControls2 Widgets)
if(ENABLE_SYNC)
    list(APPEND QT_COMPONENTS Network)
endif()

find_package(Qt5 5.15 REQUIRED COMPONENTS ${QT_COMPONENTS})
find_package(KF5 REQUIRED COMPONENTS Kirigami2 I18n CoreAddons)

# Application sources
set(worklog_SRCS
    src/cpp/main.cpp
    src/cpp/databasemanager.cpp
    src/cpp/worksessionmodel.cpp
    src/cpp/hierarchymodel.cpp
    src/cpp/tagmodel.cpp
)

if(ENABLE_SYNC)
    list(APPEND worklog_SRCS src/cpp/syncmanager.cpp)
    add_definitions(-DENABLE_SYNC)
endif()

# Qt resources
qt5_add_resources(worklog_SRCS resources/resources.qrc)

add_executable(worklog-desktop ${worklog_SRCS})

target_link_libraries(worklog-desktop
    Qt5::Core
    Qt5::Quick
    Qt5::Sql
    Qt5::QuickControls2
    Qt5::Widgets
    KF5::Kirigami2
    KF5::I18n
    KF5::CoreAddons
)

if(ENABLE_SYNC)
    target_link_libraries(worklog-desktop Qt5::Network)
endif()

install(TARGETS worklog-desktop ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES work.worklog.desktop.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES work.worklog.desktop.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES work.worklog.desktop.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)
