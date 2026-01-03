#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QHash>
#include <QList>

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    void fetchActivityCategories();
    void fetchAnnouncements();

signals:
    void categoriesReceived(const QStringList &categories);
    void announcementsReceived(const QList<QHash<QString, QString>> &announcements);
    void errorOccurred(const QString &error);

private slots:
    void onCategoriesReplyFinished();
    void onAnnouncementsReplyFinished();
    void onNetworkError(QNetworkReply::NetworkError error);

private:
    QNetworkAccessManager *networkManager;
    QNetworkReply *categoriesReply;
    QNetworkReply *announcementsReply;
    
    // 模拟服务器URL（实际使用时需要替换为真实服务器地址）
    QString baseUrl = "http://localhost:8080/api";
};

#endif // NETWORKMANAGER_H

