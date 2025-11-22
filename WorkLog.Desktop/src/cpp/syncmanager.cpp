#include "syncmanager.h"
#include "databasemanager.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QMessageAuthenticationCode>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <QDebug>

SyncManager::SyncManager(DatabaseManager *db, QObject *parent)
    : QObject(parent)
    , m_database(db)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &SyncManager::onSyncRequestFinished);
    loadConfiguration();
}

QString SyncManager::configFilePath() const
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configPath);
    if (!dir.exists()) {
        dir.mkpath(configPath);
    }
    return configPath + QStringLiteral("/worklog-sync.json");
}

void SyncManager::loadConfiguration()
{
    QFile file(configFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject obj = doc.object();

    m_config.awsAccessKeyId = obj[QStringLiteral("AwsAccessKeyId")].toString();
    m_config.awsSecretAccessKey = obj[QStringLiteral("AwsSecretAccessKey")].toString();
    m_config.awsRegion = obj[QStringLiteral("AwsRegion")].toString(QStringLiteral("us-east-1"));
    m_config.profileId = obj[QStringLiteral("ProfileId")].toString();
    m_config.sessionsTableName = obj[QStringLiteral("SessionsTableName")].toString(QStringLiteral("WorkLog_Sessions"));
    m_config.tagsTableName = obj[QStringLiteral("TagsTableName")].toString(QStringLiteral("WorkLog_Tags"));

    emit configurationChanged();
}

void SyncManager::saveConfiguration(const QString &accessKeyId,
                                    const QString &secretAccessKey,
                                    const QString &region,
                                    const QString &profileId)
{
    m_config.awsAccessKeyId = accessKeyId;
    m_config.awsSecretAccessKey = secretAccessKey;
    m_config.awsRegion = region.isEmpty() ? QStringLiteral("us-east-1") : region;
    m_config.profileId = profileId;

    QJsonObject obj;
    obj[QStringLiteral("AwsAccessKeyId")] = m_config.awsAccessKeyId;
    obj[QStringLiteral("AwsSecretAccessKey")] = m_config.awsSecretAccessKey;
    obj[QStringLiteral("AwsRegion")] = m_config.awsRegion;
    obj[QStringLiteral("ProfileId")] = m_config.profileId;
    obj[QStringLiteral("SessionsTableName")] = m_config.sessionsTableName;
    obj[QStringLiteral("TagsTableName")] = m_config.tagsTableName;

    QFile file(configFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    }

    emit configurationChanged();
}

bool SyncManager::isConfigured() const
{
    return m_config.isValid();
}

bool SyncManager::isSyncing() const
{
    return m_isSyncing;
}

QString SyncManager::lastSyncTime() const
{
    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT Value FROM SyncMetadata WHERE Key = 'LastSync'"));
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return QString();
}

QString SyncManager::getProfileId() const
{
    return m_config.profileId;
}

QString SyncManager::getAwsRegion() const
{
    return m_config.awsRegion;
}

void SyncManager::sync()
{
    if (m_isSyncing) {
        emit errorOccurred(tr("Sync already in progress"));
        return;
    }

    if (!isConfigured()) {
        emit errorOccurred(tr("Sync not configured"));
        return;
    }

    m_isSyncing = true;
    emit syncingChanged();

    m_currentResult = SyncResult();
    m_pendingRequests = 0;
    m_tagsDownloaded = false;
    m_sessionsDownloaded = false;
    m_cloudTags = QJsonArray();
    m_cloudSessions = QJsonArray();

    // Load local data
    m_localTags.clear();
    m_localSessions.clear();

    QSqlQuery tagQuery;
    tagQuery.exec(QStringLiteral("SELECT Id, Name, CloudId, UpdatedAt, IsDeleted FROM Tags"));
    while (tagQuery.next()) {
        QVariantMap tag;
        tag[QStringLiteral("id")] = tagQuery.value(0);
        tag[QStringLiteral("name")] = tagQuery.value(1);
        tag[QStringLiteral("cloudId")] = tagQuery.value(2);
        tag[QStringLiteral("updatedAt")] = tagQuery.value(3);
        tag[QStringLiteral("isDeleted")] = tagQuery.value(4).toBool();
        m_localTags.append(tag);
    }

    QSqlQuery sessionQuery;
    sessionQuery.exec(QStringLiteral(R"(
        SELECT Id, SessionDate, TimeHours, Description, Notes, NextPlannedStage,
               TagId, CreatedAt, UpdatedAt, CloudId, IsDeleted, TagCloudId
        FROM WorkSessions
    )"));
    while (sessionQuery.next()) {
        QVariantMap session;
        session[QStringLiteral("id")] = sessionQuery.value(0);
        session[QStringLiteral("sessionDate")] = sessionQuery.value(1);
        session[QStringLiteral("timeHours")] = sessionQuery.value(2);
        session[QStringLiteral("description")] = sessionQuery.value(3);
        session[QStringLiteral("notes")] = sessionQuery.value(4);
        session[QStringLiteral("nextPlannedStage")] = sessionQuery.value(5);
        session[QStringLiteral("tagId")] = sessionQuery.value(6);
        session[QStringLiteral("createdAt")] = sessionQuery.value(7);
        session[QStringLiteral("updatedAt")] = sessionQuery.value(8);
        session[QStringLiteral("cloudId")] = sessionQuery.value(9);
        session[QStringLiteral("isDeleted")] = sessionQuery.value(10).toBool();
        session[QStringLiteral("tagCloudId")] = sessionQuery.value(11);
        m_localSessions.append(session);
    }

    // Start by querying cloud data
    queryTable(m_config.tagsTableName, QStringLiteral("tags"));
    queryTable(m_config.sessionsTableName, QStringLiteral("sessions"));
}

void SyncManager::testConnection()
{
    if (!isConfigured()) {
        emit connectionTestCompleted(false, tr("Sync not configured"));
        return;
    }

    // Try to describe the sessions table
    QString host = QStringLiteral("dynamodb.%1.amazonaws.com").arg(m_config.awsRegion);
    QUrl url(QStringLiteral("https://%1").arg(host));
    QDateTime timestamp = QDateTime::currentDateTimeUtc();

    QJsonObject payload;
    payload[QStringLiteral("TableName")] = m_config.sessionsTableName;
    QByteArray payloadBytes = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-amz-json-1.0"));
    request.setRawHeader("X-Amz-Target", "DynamoDB_20120810.DescribeTable");
    request.setRawHeader("X-Amz-Date", timestamp.toString(QStringLiteral("yyyyMMddTHHmmssZ")).toLatin1());
    request.setRawHeader("Host", host.toLatin1());

    QString authHeader = signRequest(QStringLiteral("POST"), QStringLiteral("dynamodb"),
                                     host, QStringLiteral("/"), QString::fromUtf8(payloadBytes), timestamp);
    request.setRawHeader("Authorization", authHeader.toLatin1());

    request.setAttribute(QNetworkRequest::User, QStringLiteral("test"));

    m_networkManager->post(request, payloadBytes);
}

void SyncManager::queryTable(const QString &tableName, const QString &operation)
{
    QString host = QStringLiteral("dynamodb.%1.amazonaws.com").arg(m_config.awsRegion);
    QUrl url(QStringLiteral("https://%1").arg(host));
    QDateTime timestamp = QDateTime::currentDateTimeUtc();

    QJsonObject payload;
    payload[QStringLiteral("TableName")] = tableName;
    payload[QStringLiteral("KeyConditionExpression")] = QStringLiteral("ProfileId = :profileId");

    QJsonObject expressionValues;
    QJsonObject profileIdValue;
    profileIdValue[QStringLiteral("S")] = m_config.profileId;
    expressionValues[QStringLiteral(":profileId")] = profileIdValue;
    payload[QStringLiteral("ExpressionAttributeValues")] = expressionValues;

    QByteArray payloadBytes = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-amz-json-1.0"));
    request.setRawHeader("X-Amz-Target", "DynamoDB_20120810.Query");
    request.setRawHeader("X-Amz-Date", timestamp.toString(QStringLiteral("yyyyMMddTHHmmssZ")).toLatin1());
    request.setRawHeader("Host", host.toLatin1());

    QString authHeader = signRequest(QStringLiteral("POST"), QStringLiteral("dynamodb"),
                                     host, QStringLiteral("/"), QString::fromUtf8(payloadBytes), timestamp);
    request.setRawHeader("Authorization", authHeader.toLatin1());

    request.setAttribute(QNetworkRequest::User, operation);
    m_pendingRequests++;

    m_networkManager->post(request, payloadBytes);
}

void SyncManager::putItem(const QString &tableName, const QJsonObject &item)
{
    QString host = QStringLiteral("dynamodb.%1.amazonaws.com").arg(m_config.awsRegion);
    QUrl url(QStringLiteral("https://%1").arg(host));
    QDateTime timestamp = QDateTime::currentDateTimeUtc();

    QJsonObject payload;
    payload[QStringLiteral("TableName")] = tableName;
    payload[QStringLiteral("Item")] = item;

    QByteArray payloadBytes = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-amz-json-1.0"));
    request.setRawHeader("X-Amz-Target", "DynamoDB_20120810.PutItem");
    request.setRawHeader("X-Amz-Date", timestamp.toString(QStringLiteral("yyyyMMddTHHmmssZ")).toLatin1());
    request.setRawHeader("Host", host.toLatin1());

    QString authHeader = signRequest(QStringLiteral("POST"), QStringLiteral("dynamodb"),
                                     host, QStringLiteral("/"), QString::fromUtf8(payloadBytes), timestamp);
    request.setRawHeader("Authorization", authHeader.toLatin1());

    request.setAttribute(QNetworkRequest::User, QStringLiteral("put"));
    m_pendingRequests++;

    m_networkManager->post(request, payloadBytes);
}

void SyncManager::onSyncRequestFinished(QNetworkReply *reply)
{
    QString operation = reply->request().attribute(QNetworkRequest::User).toString();
    QByteArray responseData = reply->readAll();

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = reply->errorString();
        qWarning() << "Sync request failed:" << errorMsg << responseData;

        if (operation == QStringLiteral("test")) {
            emit connectionTestCompleted(false, tr("Connection failed: %1").arg(errorMsg));
        } else {
            m_currentResult.success = false;
            m_currentResult.errorMessage = errorMsg;
            m_pendingRequests--;
            if (m_pendingRequests <= 0) {
                finishSync();
            }
        }
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject response = doc.object();

    if (operation == QStringLiteral("test")) {
        emit connectionTestCompleted(true, tr("Connection successful!"));
    } else if (operation == QStringLiteral("tags")) {
        m_cloudTags = response[QStringLiteral("Items")].toArray();
        m_tagsDownloaded = true;
        m_pendingRequests--;
        if (m_tagsDownloaded && m_sessionsDownloaded) {
            syncTags();
        }
    } else if (operation == QStringLiteral("sessions")) {
        m_cloudSessions = response[QStringLiteral("Items")].toArray();
        m_sessionsDownloaded = true;
        m_pendingRequests--;
        if (m_tagsDownloaded && m_sessionsDownloaded) {
            syncTags();
        }
    } else if (operation == QStringLiteral("put")) {
        m_pendingRequests--;
        if (m_pendingRequests <= 0) {
            finishSync();
        }
    }

    reply->deleteLater();
}

void SyncManager::syncTags()
{
    // Build lookup maps
    QMap<QString, QJsonObject> cloudByCloudId;
    for (const QJsonValue &val : m_cloudTags) {
        QJsonObject obj = val.toObject();
        QString cloudId = obj[QStringLiteral("CloudId")].toObject()[QStringLiteral("S")].toString();
        cloudByCloudId[cloudId] = obj;
    }

    QMap<QString, QVariantMap> localByCloudId;
    for (const QVariantMap &tag : m_localTags) {
        QString cloudId = tag[QStringLiteral("cloudId")].toString();
        if (!cloudId.isEmpty()) {
            localByCloudId[cloudId] = tag;
        }
    }

    // Process cloud tags (download)
    for (const QJsonValue &val : m_cloudTags) {
        QJsonObject cloudTag = val.toObject();
        QString cloudId = cloudTag[QStringLiteral("CloudId")].toObject()[QStringLiteral("S")].toString();
        QString cloudName = cloudTag[QStringLiteral("Name")].toObject()[QStringLiteral("S")].toString();
        QString cloudUpdatedAt = cloudTag[QStringLiteral("UpdatedAt")].toObject()[QStringLiteral("S")].toString();
        bool cloudIsDeleted = cloudTag[QStringLiteral("IsDeleted")].toObject()[QStringLiteral("BOOL")].toBool();

        if (localByCloudId.contains(cloudId)) {
            // Exists locally - check timestamps
            QVariantMap localTag = localByCloudId[cloudId];
            QDateTime localUpdated = QDateTime::fromString(localTag[QStringLiteral("updatedAt")].toString(), Qt::ISODate);
            QDateTime cloudUpdated = QDateTime::fromString(cloudUpdatedAt, Qt::ISODate);

            if (cloudUpdated > localUpdated) {
                // Cloud is newer - update local
                QSqlQuery query;
                query.prepare(QStringLiteral("UPDATE Tags SET Name = :name, UpdatedAt = :updated, IsDeleted = :deleted WHERE Id = :id"));
                query.bindValue(QStringLiteral(":name"), cloudName);
                query.bindValue(QStringLiteral(":updated"), cloudUpdatedAt);
                query.bindValue(QStringLiteral(":deleted"), cloudIsDeleted ? 1 : 0);
                query.bindValue(QStringLiteral(":id"), localTag[QStringLiteral("id")]);
                query.exec();
                m_currentResult.tagsDownloaded++;
            }
        } else if (!cloudIsDeleted) {
            // New tag from cloud
            QSqlQuery query;
            query.prepare(QStringLiteral("INSERT INTO Tags (Name, CloudId, UpdatedAt, IsDeleted) VALUES (:name, :cloudId, :updated, 0)"));
            query.bindValue(QStringLiteral(":name"), cloudName);
            query.bindValue(QStringLiteral(":cloudId"), cloudId);
            query.bindValue(QStringLiteral(":updated"), cloudUpdatedAt);
            query.exec();
            m_currentResult.tagsDownloaded++;
        }
    }

    // Process local tags (upload)
    for (QVariantMap &tag : m_localTags) {
        QString cloudId = tag[QStringLiteral("cloudId")].toString();

        if (cloudId.isEmpty()) {
            // New local tag - assign CloudId and upload
            cloudId = QUuid::createUuid().toString(QUuid::WithoutBraces);
            QSqlQuery query;
            query.prepare(QStringLiteral("UPDATE Tags SET CloudId = :cloudId WHERE Id = :id"));
            query.bindValue(QStringLiteral(":cloudId"), cloudId);
            query.bindValue(QStringLiteral(":id"), tag[QStringLiteral("id")]);
            query.exec();
            tag[QStringLiteral("cloudId")] = cloudId;
            uploadTag(tag);
            m_currentResult.tagsUploaded++;
        } else if (cloudByCloudId.contains(cloudId)) {
            // Check if local is newer
            QJsonObject cloudTag = cloudByCloudId[cloudId];
            QString cloudUpdatedAt = cloudTag[QStringLiteral("UpdatedAt")].toObject()[QStringLiteral("S")].toString();
            QDateTime localUpdated = QDateTime::fromString(tag[QStringLiteral("updatedAt")].toString(), Qt::ISODate);
            QDateTime cloudUpdated = QDateTime::fromString(cloudUpdatedAt, Qt::ISODate);

            if (localUpdated > cloudUpdated) {
                uploadTag(tag);
                m_currentResult.tagsUploaded++;
            }
        } else {
            // Has CloudId but not in cloud
            uploadTag(tag);
            m_currentResult.tagsUploaded++;
        }
    }

    // Update tag CloudIds in local sessions for reference
    QSqlQuery updateTagCloudIds;
    updateTagCloudIds.exec(QStringLiteral(R"(
        UPDATE WorkSessions SET TagCloudId = (
            SELECT CloudId FROM Tags WHERE Tags.Id = WorkSessions.TagId
        ) WHERE TagId IS NOT NULL
    )"));

    // Now sync sessions
    syncSessions();
}

void SyncManager::syncSessions()
{
    // Build lookup maps
    QMap<QString, QJsonObject> cloudByCloudId;
    for (const QJsonValue &val : m_cloudSessions) {
        QJsonObject obj = val.toObject();
        QString cloudId = obj[QStringLiteral("CloudId")].toObject()[QStringLiteral("S")].toString();
        cloudByCloudId[cloudId] = obj;
    }

    // Build tag lookup
    QMap<QString, int> tagIdByCloudId;
    QSqlQuery tagQuery;
    tagQuery.exec(QStringLiteral("SELECT Id, CloudId FROM Tags WHERE CloudId IS NOT NULL"));
    while (tagQuery.next()) {
        tagIdByCloudId[tagQuery.value(1).toString()] = tagQuery.value(0).toInt();
    }

    QMap<QString, QVariantMap> localByCloudId;
    for (const QVariantMap &session : m_localSessions) {
        QString cloudId = session[QStringLiteral("cloudId")].toString();
        if (!cloudId.isEmpty()) {
            localByCloudId[cloudId] = session;
        }
    }

    // Process cloud sessions (download)
    for (const QJsonValue &val : m_cloudSessions) {
        QJsonObject cloudSession = val.toObject();
        QString cloudId = cloudSession[QStringLiteral("CloudId")].toObject()[QStringLiteral("S")].toString();
        QString cloudUpdatedAt = cloudSession[QStringLiteral("UpdatedAt")].toObject()[QStringLiteral("S")].toString();
        bool cloudIsDeleted = cloudSession[QStringLiteral("IsDeleted")].toObject()[QStringLiteral("BOOL")].toBool();

        if (localByCloudId.contains(cloudId)) {
            // Exists locally - check timestamps
            QVariantMap localSession = localByCloudId[cloudId];
            QDateTime localUpdated = QDateTime::fromString(localSession[QStringLiteral("updatedAt")].toString(), Qt::ISODate);
            QDateTime cloudUpdated = QDateTime::fromString(cloudUpdatedAt, Qt::ISODate);

            if (cloudUpdated > localUpdated) {
                // Cloud is newer - update local
                QString tagCloudId = cloudSession[QStringLiteral("TagCloudId")].toObject()[QStringLiteral("S")].toString();
                QVariant tagId = tagIdByCloudId.contains(tagCloudId) ? QVariant(tagIdByCloudId[tagCloudId]) : QVariant();

                QSqlQuery query;
                query.prepare(QStringLiteral(R"(
                    UPDATE WorkSessions SET
                        SessionDate = :date, TimeHours = :hours, Description = :desc,
                        Notes = :notes, NextPlannedStage = :next, TagId = :tagId,
                        TagCloudId = :tagCloudId, UpdatedAt = :updated, IsDeleted = :deleted
                    WHERE Id = :id
                )"));
                query.bindValue(QStringLiteral(":date"), cloudSession[QStringLiteral("SessionDate")].toObject()[QStringLiteral("S")].toString());
                query.bindValue(QStringLiteral(":hours"), cloudSession[QStringLiteral("TimeHours")].toObject()[QStringLiteral("N")].toString().toDouble());
                query.bindValue(QStringLiteral(":desc"), cloudSession[QStringLiteral("Description")].toObject()[QStringLiteral("S")].toString());
                query.bindValue(QStringLiteral(":notes"), cloudSession[QStringLiteral("Notes")].toObject()[QStringLiteral("S")].toString());
                query.bindValue(QStringLiteral(":next"), cloudSession[QStringLiteral("NextPlannedStage")].toObject()[QStringLiteral("S")].toString());
                query.bindValue(QStringLiteral(":tagId"), tagId);
                query.bindValue(QStringLiteral(":tagCloudId"), tagCloudId);
                query.bindValue(QStringLiteral(":updated"), cloudUpdatedAt);
                query.bindValue(QStringLiteral(":deleted"), cloudIsDeleted ? 1 : 0);
                query.bindValue(QStringLiteral(":id"), localSession[QStringLiteral("id")]);
                query.exec();
                m_currentResult.sessionsDownloaded++;
            }
        } else if (!cloudIsDeleted) {
            // New session from cloud
            QString tagCloudId = cloudSession[QStringLiteral("TagCloudId")].toObject()[QStringLiteral("S")].toString();
            QVariant tagId = tagIdByCloudId.contains(tagCloudId) ? QVariant(tagIdByCloudId[tagCloudId]) : QVariant();

            QSqlQuery query;
            query.prepare(QStringLiteral(R"(
                INSERT INTO WorkSessions (SessionDate, TimeHours, Description, Notes, NextPlannedStage,
                    TagId, TagCloudId, CreatedAt, UpdatedAt, CloudId, IsDeleted)
                VALUES (:date, :hours, :desc, :notes, :next, :tagId, :tagCloudId, :created, :updated, :cloudId, 0)
            )"));
            query.bindValue(QStringLiteral(":date"), cloudSession[QStringLiteral("SessionDate")].toObject()[QStringLiteral("S")].toString());
            query.bindValue(QStringLiteral(":hours"), cloudSession[QStringLiteral("TimeHours")].toObject()[QStringLiteral("N")].toString().toDouble());
            query.bindValue(QStringLiteral(":desc"), cloudSession[QStringLiteral("Description")].toObject()[QStringLiteral("S")].toString());
            query.bindValue(QStringLiteral(":notes"), cloudSession[QStringLiteral("Notes")].toObject()[QStringLiteral("S")].toString());
            query.bindValue(QStringLiteral(":next"), cloudSession[QStringLiteral("NextPlannedStage")].toObject()[QStringLiteral("S")].toString());
            query.bindValue(QStringLiteral(":tagId"), tagId);
            query.bindValue(QStringLiteral(":tagCloudId"), tagCloudId);
            query.bindValue(QStringLiteral(":created"), cloudSession[QStringLiteral("CreatedAt")].toObject()[QStringLiteral("S")].toString());
            query.bindValue(QStringLiteral(":updated"), cloudUpdatedAt);
            query.bindValue(QStringLiteral(":cloudId"), cloudId);
            query.exec();
            m_currentResult.sessionsDownloaded++;
        }
    }

    // Process local sessions (upload)
    for (QVariantMap &session : m_localSessions) {
        QString cloudId = session[QStringLiteral("cloudId")].toString();

        if (cloudId.isEmpty()) {
            // New local session - assign CloudId and upload
            cloudId = QUuid::createUuid().toString(QUuid::WithoutBraces);
            QSqlQuery query;
            query.prepare(QStringLiteral("UPDATE WorkSessions SET CloudId = :cloudId WHERE Id = :id"));
            query.bindValue(QStringLiteral(":cloudId"), cloudId);
            query.bindValue(QStringLiteral(":id"), session[QStringLiteral("id")]);
            query.exec();
            session[QStringLiteral("cloudId")] = cloudId;
            uploadSession(session);
            m_currentResult.sessionsUploaded++;
        } else if (cloudByCloudId.contains(cloudId)) {
            // Check if local is newer
            QJsonObject cloudSession = cloudByCloudId[cloudId];
            QString cloudUpdatedAt = cloudSession[QStringLiteral("UpdatedAt")].toObject()[QStringLiteral("S")].toString();
            QDateTime localUpdated = QDateTime::fromString(session[QStringLiteral("updatedAt")].toString(), Qt::ISODate);
            QDateTime cloudUpdated = QDateTime::fromString(cloudUpdatedAt, Qt::ISODate);

            if (localUpdated > cloudUpdated) {
                uploadSession(session);
                m_currentResult.sessionsUploaded++;
            }
        } else {
            // Has CloudId but not in cloud
            uploadSession(session);
            m_currentResult.sessionsUploaded++;
        }
    }

    if (m_pendingRequests <= 0) {
        finishSync();
    }
}

void SyncManager::uploadTag(const QVariantMap &tag)
{
    QJsonObject item;

    QJsonObject profileIdAttr;
    profileIdAttr[QStringLiteral("S")] = m_config.profileId;
    item[QStringLiteral("ProfileId")] = profileIdAttr;

    QJsonObject cloudIdAttr;
    cloudIdAttr[QStringLiteral("S")] = tag[QStringLiteral("cloudId")].toString();
    item[QStringLiteral("CloudId")] = cloudIdAttr;

    QJsonObject nameAttr;
    nameAttr[QStringLiteral("S")] = tag[QStringLiteral("name")].toString();
    item[QStringLiteral("Name")] = nameAttr;

    QJsonObject updatedAtAttr;
    updatedAtAttr[QStringLiteral("S")] = tag[QStringLiteral("updatedAt")].toString();
    item[QStringLiteral("UpdatedAt")] = updatedAtAttr;

    QJsonObject isDeletedAttr;
    isDeletedAttr[QStringLiteral("BOOL")] = tag[QStringLiteral("isDeleted")].toBool();
    item[QStringLiteral("IsDeleted")] = isDeletedAttr;

    putItem(m_config.tagsTableName, item);
}

void SyncManager::uploadSession(const QVariantMap &session)
{
    QJsonObject item;

    QJsonObject profileIdAttr;
    profileIdAttr[QStringLiteral("S")] = m_config.profileId;
    item[QStringLiteral("ProfileId")] = profileIdAttr;

    QJsonObject cloudIdAttr;
    cloudIdAttr[QStringLiteral("S")] = session[QStringLiteral("cloudId")].toString();
    item[QStringLiteral("CloudId")] = cloudIdAttr;

    QJsonObject dateAttr;
    dateAttr[QStringLiteral("S")] = session[QStringLiteral("sessionDate")].toString();
    item[QStringLiteral("SessionDate")] = dateAttr;

    QJsonObject hoursAttr;
    hoursAttr[QStringLiteral("N")] = QString::number(session[QStringLiteral("timeHours")].toDouble());
    item[QStringLiteral("TimeHours")] = hoursAttr;

    QJsonObject descAttr;
    descAttr[QStringLiteral("S")] = session[QStringLiteral("description")].toString();
    item[QStringLiteral("Description")] = descAttr;

    if (!session[QStringLiteral("notes")].toString().isEmpty()) {
        QJsonObject notesAttr;
        notesAttr[QStringLiteral("S")] = session[QStringLiteral("notes")].toString();
        item[QStringLiteral("Notes")] = notesAttr;
    }

    if (!session[QStringLiteral("nextPlannedStage")].toString().isEmpty()) {
        QJsonObject nextAttr;
        nextAttr[QStringLiteral("S")] = session[QStringLiteral("nextPlannedStage")].toString();
        item[QStringLiteral("NextPlannedStage")] = nextAttr;
    }

    if (!session[QStringLiteral("tagCloudId")].toString().isEmpty()) {
        QJsonObject tagCloudIdAttr;
        tagCloudIdAttr[QStringLiteral("S")] = session[QStringLiteral("tagCloudId")].toString();
        item[QStringLiteral("TagCloudId")] = tagCloudIdAttr;
    }

    QJsonObject createdAtAttr;
    createdAtAttr[QStringLiteral("S")] = session[QStringLiteral("createdAt")].toString();
    item[QStringLiteral("CreatedAt")] = createdAtAttr;

    QJsonObject updatedAtAttr;
    updatedAtAttr[QStringLiteral("S")] = session[QStringLiteral("updatedAt")].toString();
    item[QStringLiteral("UpdatedAt")] = updatedAtAttr;

    QJsonObject isDeletedAttr;
    isDeletedAttr[QStringLiteral("BOOL")] = session[QStringLiteral("isDeleted")].toBool();
    item[QStringLiteral("IsDeleted")] = isDeletedAttr;

    putItem(m_config.sessionsTableName, item);
}

void SyncManager::finishSync()
{
    updateLastSyncTime();

    m_currentResult.success = m_currentResult.errorMessage.isEmpty();
    m_isSyncing = false;
    emit syncingChanged();

    QString message;
    if (m_currentResult.success) {
        message = tr("Sync completed! Uploaded: %1 sessions, %2 tags. Downloaded: %3 sessions, %4 tags.")
            .arg(m_currentResult.sessionsUploaded)
            .arg(m_currentResult.tagsUploaded)
            .arg(m_currentResult.sessionsDownloaded)
            .arg(m_currentResult.tagsDownloaded);
    } else {
        message = tr("Sync failed: %1").arg(m_currentResult.errorMessage);
    }

    emit syncCompleted(m_currentResult.success, message);
    emit lastSyncTimeChanged();

    // Refresh the UI
    emit m_database->dataChanged();
    emit m_database->tagsChanged();
}

void SyncManager::updateLastSyncTime()
{
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QSqlQuery query;
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO SyncMetadata (Key, Value) VALUES ('LastSync', :value)"));
    query.bindValue(QStringLiteral(":value"), now);
    query.exec();
}

// AWS Signature Version 4 implementation
QString SyncManager::signRequest(const QString &method, const QString &service,
                                  const QString &host, const QString &canonicalUri,
                                  const QString &payload, const QDateTime &timestamp)
{
    QString amzDate = timestamp.toString(QStringLiteral("yyyyMMddTHHmmssZ"));
    QString dateStamp = timestamp.toString(QStringLiteral("yyyyMMdd"));

    // Create canonical request
    QString signedHeaders = QStringLiteral("content-type;host;x-amz-date;x-amz-target");
    QString canonicalHeaders = QStringLiteral("content-type:application/x-amz-json-1.0\n")
        + QStringLiteral("host:%1\n").arg(host)
        + QStringLiteral("x-amz-date:%1\n").arg(amzDate)
        + QStringLiteral("x-amz-target:DynamoDB_20120810.Query\n");

    QString payloadHash = hashSha256(payload);
    QString canonicalRequest = method + QStringLiteral("\n")
        + canonicalUri + QStringLiteral("\n")
        + QStringLiteral("\n")  // query string
        + canonicalHeaders + QStringLiteral("\n")
        + signedHeaders + QStringLiteral("\n")
        + payloadHash;

    // Create string to sign
    QString algorithm = QStringLiteral("AWS4-HMAC-SHA256");
    QString credentialScope = dateStamp + QStringLiteral("/") + m_config.awsRegion + QStringLiteral("/") + service + QStringLiteral("/aws4_request");
    QString stringToSign = algorithm + QStringLiteral("\n")
        + amzDate + QStringLiteral("\n")
        + credentialScope + QStringLiteral("\n")
        + hashSha256(canonicalRequest);

    // Create signing key
    QByteArray kDate = hmacSha256(QStringLiteral("AWS4%1").arg(m_config.awsSecretAccessKey).toUtf8(), dateStamp.toUtf8());
    QByteArray kRegion = hmacSha256(kDate, m_config.awsRegion.toUtf8());
    QByteArray kService = hmacSha256(kRegion, service.toUtf8());
    QByteArray kSigning = hmacSha256(kService, QByteArrayLiteral("aws4_request"));

    // Create signature
    QString signature = QString::fromLatin1(hmacSha256(kSigning, stringToSign.toUtf8()).toHex());

    // Create authorization header
    QString authHeader = algorithm + QStringLiteral(" ")
        + QStringLiteral("Credential=%1/%2, ").arg(m_config.awsAccessKeyId, credentialScope)
        + QStringLiteral("SignedHeaders=%1, ").arg(signedHeaders)
        + QStringLiteral("Signature=%1").arg(signature);

    return authHeader;
}

QByteArray SyncManager::hmacSha256(const QByteArray &key, const QByteArray &data)
{
    return QMessageAuthenticationCode::hash(data, key, QCryptographicHash::Sha256);
}

QString SyncManager::hashSha256(const QString &data)
{
    return QString::fromLatin1(QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Sha256).toHex());
}
