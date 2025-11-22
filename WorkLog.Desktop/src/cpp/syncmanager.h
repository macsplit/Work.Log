#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QDateTime>

class DatabaseManager;

struct SyncConfig {
    QString awsAccessKeyId;
    QString awsSecretAccessKey;
    QString awsRegion;
    QString profileId;
    QString sessionsTableName;
    QString tagsTableName;

    bool isValid() const {
        return !awsAccessKeyId.isEmpty() &&
               !awsSecretAccessKey.isEmpty() &&
               !profileId.isEmpty();
    }
};

struct SyncResult {
    bool success = false;
    QString errorMessage;
    int sessionsUploaded = 0;
    int sessionsDownloaded = 0;
    int tagsUploaded = 0;
    int tagsDownloaded = 0;
};

class SyncManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isConfigured READ isConfigured NOTIFY configurationChanged)
    Q_PROPERTY(bool isSyncing READ isSyncing NOTIFY syncingChanged)
    Q_PROPERTY(QString lastSyncTime READ lastSyncTime NOTIFY lastSyncTimeChanged)

public:
    explicit SyncManager(DatabaseManager *db, QObject *parent = nullptr);

    bool isConfigured() const;
    bool isSyncing() const;
    QString lastSyncTime() const;

    Q_INVOKABLE void loadConfiguration();
    Q_INVOKABLE void saveConfiguration(const QString &accessKeyId,
                                       const QString &secretAccessKey,
                                       const QString &region,
                                       const QString &profileId);
    Q_INVOKABLE void sync();
    Q_INVOKABLE void testConnection();

    Q_INVOKABLE QString getProfileId() const;
    Q_INVOKABLE QString getAwsRegion() const;

signals:
    void configurationChanged();
    void syncingChanged();
    void lastSyncTimeChanged();
    void syncCompleted(bool success, const QString &message);
    void connectionTestCompleted(bool success, const QString &message);
    void errorOccurred(const QString &error);

private slots:
    void onSyncRequestFinished(QNetworkReply *reply);

private:
    QString configFilePath() const;
    QString signRequest(const QString &method, const QString &service,
                       const QString &host, const QString &canonicalUri,
                       const QString &payload, const QDateTime &timestamp,
                       const QString &amzTarget);
    QByteArray hmacSha256(const QByteArray &key, const QByteArray &data);
    QString hashSha256(const QString &data);

    void syncTags();
    void syncSessions();
    void uploadTag(const QVariantMap &tag);
    void uploadSession(const QVariantMap &session);
    void downloadTags(const QJsonArray &items);
    void downloadSessions(const QJsonArray &items);

    void queryTable(const QString &tableName, const QString &operation);
    void putItem(const QString &tableName, const QJsonObject &item);

    void finishSync();
    void updateLastSyncTime();

    DatabaseManager *m_database;
    QNetworkAccessManager *m_networkManager;
    SyncConfig m_config;
    bool m_isSyncing = false;
    SyncResult m_currentResult;

    int m_pendingRequests = 0;
    QList<QVariantMap> m_localTags;
    QList<QVariantMap> m_localSessions;
    QJsonArray m_cloudTags;
    QJsonArray m_cloudSessions;
    bool m_tagsDownloaded = false;
    bool m_sessionsDownloaded = false;
};

#endif // SYNCMANAGER_H
