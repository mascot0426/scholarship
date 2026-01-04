#include "networkmanager.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , networkManager(new QNetworkAccessManager(this))
    , categoriesReply(nullptr)
    , announcementsReply(nullptr)
    , syncActivityReply(nullptr)
    , syncingActivityId(-1)
{
    connect(networkManager, &QNetworkAccessManager::finished, this, [this](QNetworkReply *reply) {
        if (reply == categoriesReply) {
            onCategoriesReplyFinished();
        } else if (reply == announcementsReply) {
            onAnnouncementsReplyFinished();
        } else if (reply == syncActivityReply) {
            onSyncActivityReplyFinished();
        }
    });
}

NetworkManager::~NetworkManager()
{
    if (categoriesReply) {
        categoriesReply->deleteLater();
    }
    if (announcementsReply) {
        announcementsReply->deleteLater();
    }
    if (syncActivityReply) {
        syncActivityReply->deleteLater();
    }
}

void NetworkManager::fetchActivityCategories()
{
    QUrl url(baseUrl + "/categories");
    QNetworkRequest request(url);
    
    categoriesReply = networkManager->get(request);
    // Qt 5.12使用error信号，Qt 5.15+使用errorOccurred
    #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(categoriesReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &NetworkManager::onNetworkError);
    #else
    connect(categoriesReply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &NetworkManager::onNetworkError);
    #endif
}

void NetworkManager::fetchAnnouncements()
{
    QUrl url(baseUrl + "/announcements");
    QNetworkRequest request(url);
    
    announcementsReply = networkManager->get(request);
    // Qt 5.12使用error信号，Qt 5.15+使用errorOccurred
    #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(announcementsReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &NetworkManager::onNetworkError);
    #else
    connect(announcementsReply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &NetworkManager::onNetworkError);
    #endif
}

void NetworkManager::onCategoriesReplyFinished()
{
    if (!categoriesReply) return;
    
    if (categoriesReply->error() != QNetworkReply::NoError) {
        // 如果网络请求失败，返回默认类别列表
        QStringList defaultCategories;
        defaultCategories << "学术讲座" << "文体活动" << "社会实践" << "志愿服务" << "竞赛活动" << "其他";
        emit categoriesReceived(defaultCategories);
        categoriesReply->deleteLater();
        categoriesReply = nullptr;
        return;
    }
    
    QByteArray data = categoriesReply->readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        emit errorOccurred("解析JSON失败：" + error.errorString());
        categoriesReply->deleteLater();
        categoriesReply = nullptr;
        return;
    }
    
    QStringList categories;
    if (doc.isArray()) {
        QJsonArray array = doc.array();
        for (const QJsonValue &value : array) {
            if (value.isObject()) {
                QJsonObject obj = value.toObject();
                if (obj.contains("name")) {
                    categories.append(obj["name"].toString());
                }
            } else if (value.isString()) {
                categories.append(value.toString());
            }
        }
    }
    
    // 如果没有获取到类别，使用默认列表
    if (categories.isEmpty()) {
        categories = QStringList() << "学术讲座" << "文体活动" << "社会实践" << "志愿服务" << "竞赛活动" << "其他";
    }
    
    emit categoriesReceived(categories);
    categoriesReply->deleteLater();
    categoriesReply = nullptr;
}

void NetworkManager::onAnnouncementsReplyFinished()
{
    if (!announcementsReply) return;
    
    if (announcementsReply->error() != QNetworkReply::NoError) {
        emit errorOccurred("网络请求失败：" + announcementsReply->errorString());
        announcementsReply->deleteLater();
        announcementsReply = nullptr;
        return;
    }
    
    QByteArray data = announcementsReply->readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        emit errorOccurred("解析JSON失败：" + error.errorString());
        announcementsReply->deleteLater();
        announcementsReply = nullptr;
        return;
    }
    
    QList<QHash<QString, QString>> announcements;
    if (doc.isArray()) {
        QJsonArray array = doc.array();
        for (const QJsonValue &value : array) {
            if (value.isObject()) {
                QJsonObject obj = value.toObject();
                QHash<QString, QString> announcement;
                if (obj.contains("title")) {
                    announcement["title"] = obj["title"].toString();
                }
                if (obj.contains("content")) {
                    announcement["content"] = obj["content"].toString();
                }
                if (obj.contains("date")) {
                    announcement["date"] = obj["date"].toString();
                }
                announcements.append(announcement);
            }
        }
    }
    
    emit announcementsReceived(announcements);
    announcementsReply->deleteLater();
    announcementsReply = nullptr;
}

void NetworkManager::syncActivityToPlatform(int activityId, const QHash<QString, QVariant> &activityData)
{
    QUrl url(baseUrl + "/activities/sync");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // 构建JSON数据
    QJsonObject json;
    json["id"] = activityId;
    json["title"] = activityData["title"].toString();
    json["description"] = activityData["description"].toString();
    json["category"] = activityData["category"].toString();
    json["organizer"] = activityData["organizer"].toString();
    json["start_time"] = activityData["start_time"].toDateTime().toString(Qt::ISODate);
    json["end_time"] = activityData["end_time"].toDateTime().toString(Qt::ISODate);
    json["max_participants"] = activityData["max_participants"].toInt();
    json["location"] = activityData["location"].toString();
    json["status"] = activityData["status"].toInt();
    
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();
    
    qDebug() << "[同步请求] 活动ID:" << activityId;
    qDebug() << "[同步请求] URL:" << url.toString();
    qDebug() << "[同步请求] 数据:" << QString::fromUtf8(data);
    
    syncingActivityId = activityId;
    syncActivityReply = networkManager->post(request, data);
    
    #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(syncActivityReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &NetworkManager::onNetworkError);
    #else
    connect(syncActivityReply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &NetworkManager::onNetworkError);
    #endif
}

void NetworkManager::onSyncActivityReplyFinished()
{
    if (!syncActivityReply) return;
    
    bool success = false;
    int activityId = syncingActivityId;
    
    if (syncActivityReply->error() == QNetworkReply::NoError) {
        QByteArray data = syncActivityReply->readAll();
        qDebug() << "[同步响应] 活动ID:" << activityId << "响应数据:" << data;
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        
        if (error.error != QJsonParseError::NoError) {
            qDebug() << "[同步错误] JSON解析失败:" << error.errorString();
            qDebug() << "[同步错误] 响应内容:" << QString::fromUtf8(data);
        } else if (doc.isObject()) {
            QJsonObject obj = doc.object();
            qDebug() << "[同步响应] JSON对象:" << obj;
            
            if (obj.contains("success")) {
                success = obj["success"].toBool();
                qDebug() << "[同步结果] 活动ID:" << activityId << "成功:" << success;
                if (obj.contains("message")) {
                    qDebug() << "[同步消息]:" << obj["message"].toString();
                }
            } else {
                qDebug() << "[同步警告] 响应中缺少success字段";
            }
        } else {
            qDebug() << "[同步错误] 响应不是JSON对象";
        }
    } else {
        qDebug() << "[同步错误] 网络请求失败:" << syncActivityReply->errorString();
        qDebug() << "[同步错误] 错误代码:" << syncActivityReply->error();
    }
    
    qDebug() << "[同步完成] 活动ID:" << activityId << "最终结果:" << success;
    emit activitySynced(activityId, success);
    syncActivityReply->deleteLater();
    syncActivityReply = nullptr;
    syncingActivityId = -1;
}

void NetworkManager::onNetworkError(QNetworkReply::NetworkError error)
{
    QString errorString;
    switch (error) {
        case QNetworkReply::ConnectionRefusedError:
            errorString = "连接被拒绝";
            break;
        case QNetworkReply::HostNotFoundError:
            errorString = "主机未找到";
            break;
        case QNetworkReply::TimeoutError:
            errorString = "请求超时";
            break;
        default:
            errorString = "网络错误：" + QString::number(error);
    }
    
    emit errorOccurred(errorString);
}

