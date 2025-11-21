#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QQuickStyle>

#include <KLocalizedContext>
#include <KLocalizedString>

#include "databasemanager.h"
#include "worksessionmodel.h"
#include "hierarchymodel.h"
#include "tagmodel.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("worklog-desktop");
    QCoreApplication::setOrganizationName(QStringLiteral("WorkLog"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("worklog.local"));
    QCoreApplication::setApplicationName(QStringLiteral("Work Log"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("appointment-new")));

    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    // Initialize database
    DatabaseManager *dbManager = new DatabaseManager(&app);
    if (!dbManager->initialize()) {
        qCritical() << "Failed to initialize database";
        return 1;
    }

    // Create models
    WorkSessionModel *sessionModel = new WorkSessionModel(dbManager, &app);
    HierarchyModel *hierarchyModel = new HierarchyModel(dbManager, &app);
    TagModel *tagModel = new TagModel(dbManager, &app);

    QQmlApplicationEngine engine;

    // Register singletons for QML
    qmlRegisterSingletonInstance("org.worklog", 1, 0, "Database", dbManager);
    qmlRegisterSingletonInstance("org.worklog", 1, 0, "SessionModel", sessionModel);
    qmlRegisterSingletonInstance("org.worklog", 1, 0, "HierarchyModel", hierarchyModel);
    qmlRegisterSingletonInstance("org.worklog", 1, 0, "TagModel", tagModel);

    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
