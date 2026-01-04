#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QVariant>
#include <QHash>
#include <QList>
#include <QDateTime>

// 用户角色枚举
enum class UserRole {
    Admin,      // 管理员
    Organizer,  // 发起人
    Student     // 学生
};

// 活动状态枚举
enum class ActivityStatus {
    Pending,    // 待审批
    Approved,   // 已批准
    Rejected,   // 已拒绝
    Ongoing,    // 进行中
    Finished    // 已结束
};

// 报名状态枚举
enum class RegistrationStatus {
    Registered,     // 已报名
    Cancelled,      // 已取消
    Waitlisted,     // 候补
    Confirmed       // 已确认
};

class Database : public QObject
{
    Q_OBJECT

public:
    explicit Database(QObject *parent = nullptr);
    ~Database();

    // 初始化数据库，创建表结构
    bool initializeDatabase();
    
    // 用户相关操作
    bool addUser(const QString &studentId, const QString &password, UserRole role, const QString &name = "");
    bool authenticateUser(const QString &studentId, const QString &password, UserRole &role, QString &name);
    UserRole getUserRole(const QString &studentId);
    bool studentIdExists(const QString &studentId);
    
    // 活动相关操作
    int createActivity(const QString &title, const QString &description, 
                      const QString &category, const QString &organizer,
                      const QDateTime &startTime, const QDateTime &endTime,
                      int maxParticipants, const QString &location);
    bool updateActivityStatus(int activityId, ActivityStatus status);
    QList<QHash<QString, QVariant>> getActivities(const QString &filter = "");
    QHash<QString, QVariant> getActivity(int activityId);
    
    // 报名相关操作
    bool registerActivity(int activityId, const QString &studentId, const QString &studentName);
    bool cancelRegistration(int activityId, const QString &studentId);
    bool isRegistered(int activityId, const QString &studentId);
    QList<QHash<QString, QVariant>> getRegistrations(int activityId);
    QList<QHash<QString, QVariant>> getStudentRegistrations(const QString &studentId);
    int getRegistrationCount(int activityId);
    bool addToWaitlist(int activityId, const QString &studentId, const QString &studentName);
    QList<QHash<QString, QVariant>> getWaitlist(int activityId);
    bool promoteFromWaitlist(int activityId);
    
    // 冲突检测
    QList<QHash<QString, QVariant>> checkTimeConflict(const QString &studentId, 
                                                      const QDateTime &startTime, 
                                                      const QDateTime &endTime,
                                                      int excludeActivityId = -1);
    
    // 签到相关操作
    bool checkIn(int activityId, const QString &studentId);
    bool isCheckedIn(int activityId, const QString &studentId);
    QList<QHash<QString, QVariant>> getCheckInList(int activityId);
    QHash<QString, QVariant> getCheckInStatistics(int activityId);
    
    // 统计信息
    QHash<QString, QVariant> getActivityStatistics(int activityId);
    QList<QHash<QString, QVariant>> getAllStatistics();

private:
    QSqlDatabase db;
    bool createTables();
    QString hashPassword(const QString &password);
};

#endif // DATABASE_H

