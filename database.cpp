#include "database.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>

Database::Database(QObject *parent)
    : QObject(parent)
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("activity_management.db");
}

Database::~Database()
{
    if (db.isOpen()) {
        db.close();
    }
}

bool Database::initializeDatabase()
{
    if (!db.open()) {
        qDebug() << "Error: Failed to open database" << db.lastError().text();
        return false;
    }
    
    return createTables();
}

bool Database::createTables()
{
    QSqlQuery query(db);
    
    // 用户表
    QString createUsersTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL,
            role INTEGER NOT NULL,
            name TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(createUsersTable)) {
        qDebug() << "Error creating users table:" << query.lastError().text();
        return false;
    }
    
    // 活动表
    QString createActivitiesTable = R"(
        CREATE TABLE IF NOT EXISTS activities (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            description TEXT,
            category TEXT,
            organizer TEXT NOT NULL,
            start_time DATETIME NOT NULL,
            end_time DATETIME NOT NULL,
            max_participants INTEGER NOT NULL,
            current_participants INTEGER DEFAULT 0,
            location TEXT,
            status INTEGER DEFAULT 0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            approved_at DATETIME,
            approved_by TEXT
        )
    )";
    
    if (!query.exec(createActivitiesTable)) {
        qDebug() << "Error creating activities table:" << query.lastError().text();
        return false;
    }
    
    // 报名表
    QString createRegistrationsTable = R"(
        CREATE TABLE IF NOT EXISTS registrations (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            activity_id INTEGER NOT NULL,
            student_id TEXT NOT NULL,
            student_name TEXT NOT NULL,
            status INTEGER DEFAULT 0,
            registered_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            checkin_time DATETIME,
            FOREIGN KEY (activity_id) REFERENCES activities(id) ON DELETE CASCADE,
            UNIQUE(activity_id, student_id)
        )
    )";
    
    if (!query.exec(createRegistrationsTable)) {
        qDebug() << "Error creating registrations table:" << query.lastError().text();
        return false;
    }
    
    // 候补表
    QString createWaitlistTable = R"(
        CREATE TABLE IF NOT EXISTS waitlist (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            activity_id INTEGER NOT NULL,
            student_id TEXT NOT NULL,
            student_name TEXT NOT NULL,
            added_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (activity_id) REFERENCES activities(id) ON DELETE CASCADE,
            UNIQUE(activity_id, student_id)
        )
    )";
    
    if (!query.exec(createWaitlistTable)) {
        qDebug() << "Error creating waitlist table:" << query.lastError().text();
        return false;
    }
    
    // 创建索引以提高查询性能
    query.exec("CREATE INDEX IF NOT EXISTS idx_registrations_activity ON registrations(activity_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_registrations_student ON registrations(student_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_waitlist_activity ON waitlist(activity_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_activities_status ON activities(status)");
    
    // 数据库迁移：为已存在的表添加签到字段（如果不存在）
    QSqlQuery checkColumnQuery(db);
    checkColumnQuery.prepare("PRAGMA table_info(registrations)");
    bool hasCheckinColumn = false;
    if (checkColumnQuery.exec()) {
        while (checkColumnQuery.next()) {
            if (checkColumnQuery.value("name").toString() == "checkin_time") {
                hasCheckinColumn = true;
                break;
            }
        }
    }
    if (!hasCheckinColumn) {
        query.prepare("ALTER TABLE registrations ADD COLUMN checkin_time DATETIME");
        if (!query.exec()) {
            qDebug() << "Warning: Failed to add checkin_time column:" << query.lastError().text();
        }
    }
    
    // 插入默认管理员账户（如果不存在）
    query.prepare("SELECT COUNT(*) FROM users WHERE username = 'admin'");
    if (query.exec() && query.next() && query.value(0).toInt() == 0) {
        addUser("admin", "admin123", UserRole::Admin, "系统管理员");
    }
    
    return true;
}

QString Database::hashPassword(const QString &password)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(password.toUtf8());
    return hash.result().toHex();
}

bool Database::addUser(const QString &username, const QString &password, UserRole role, const QString &name)
{
    QSqlQuery query(db);
    query.prepare("INSERT INTO users (username, password, role, name) VALUES (?, ?, ?, ?)");
    query.addBindValue(username);
    query.addBindValue(hashPassword(password));
    query.addBindValue(static_cast<int>(role));
    query.addBindValue(name);
    
    return query.exec();
}

bool Database::authenticateUser(const QString &username, const QString &password, UserRole &role, QString &name)
{
    QSqlQuery query(db);
    query.prepare("SELECT password, role, name FROM users WHERE username = ?");
    query.addBindValue(username);
    
    if (!query.exec() || !query.next()) {
        return false;
    }
    
    QString storedPassword = query.value(0).toString();
    if (storedPassword != hashPassword(password)) {
        return false;
    }
    
    role = static_cast<UserRole>(query.value(1).toInt());
    name = query.value(2).toString();
    return true;
}

UserRole Database::getUserRole(const QString &username)
{
    QSqlQuery query(db);
    query.prepare("SELECT role FROM users WHERE username = ?");
    query.addBindValue(username);
    
    if (query.exec() && query.next()) {
        return static_cast<UserRole>(query.value(0).toInt());
    }
    
    return UserRole::Student;
}

int Database::createActivity(const QString &title, const QString &description,
                            const QString &category, const QString &organizer,
                            const QDateTime &startTime, const QDateTime &endTime,
                            int maxParticipants, const QString &location)
{
    QSqlQuery query(db);
    query.prepare(R"(
        INSERT INTO activities (title, description, category, organizer, start_time, 
                               end_time, max_participants, location, status)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    query.addBindValue(title);
    query.addBindValue(description);
    query.addBindValue(category);
    query.addBindValue(organizer);
    query.addBindValue(startTime);
    query.addBindValue(endTime);
    query.addBindValue(maxParticipants);
    query.addBindValue(location);
    query.addBindValue(static_cast<int>(ActivityStatus::Pending));
    
    if (!query.exec()) {
        qDebug() << "Error creating activity:" << query.lastError().text();
        return -1;
    }
    
    return query.lastInsertId().toInt();
}

bool Database::updateActivityStatus(int activityId, ActivityStatus status)
{
    QSqlQuery query(db);
    query.prepare("UPDATE activities SET status = ?, approved_at = ?, approved_by = ? WHERE id = ?");
    query.addBindValue(static_cast<int>(status));
    query.addBindValue(QDateTime::currentDateTime());
    query.addBindValue(""); // 可以从当前登录用户获取
    query.addBindValue(activityId);
    
    return query.exec();
}

QList<QHash<QString, QVariant>> Database::getActivities(const QString &filter)
{
    QList<QHash<QString, QVariant>> activities;
    QSqlQuery query(db);
    
    QString sql = "SELECT * FROM activities";
    if (!filter.isEmpty()) {
        sql += " WHERE " + filter;
    }
    sql += " ORDER BY created_at DESC";
    
    if (query.exec(sql)) {
        while (query.next()) {
            QHash<QString, QVariant> activity;
            activity["id"] = query.value("id");
            activity["title"] = query.value("title");
            activity["description"] = query.value("description");
            activity["category"] = query.value("category");
            activity["organizer"] = query.value("organizer");
            activity["start_time"] = query.value("start_time");
            activity["end_time"] = query.value("end_time");
            activity["max_participants"] = query.value("max_participants");
            activity["current_participants"] = query.value("current_participants");
            activity["location"] = query.value("location");
            activity["status"] = query.value("status");
            activity["created_at"] = query.value("created_at");
            activities.append(activity);
        }
    }
    
    return activities;
}

QHash<QString, QVariant> Database::getActivity(int activityId)
{
    QHash<QString, QVariant> activity;
    QSqlQuery query(db);
    query.prepare("SELECT * FROM activities WHERE id = ?");
    query.addBindValue(activityId);
    
    if (query.exec() && query.next()) {
        activity["id"] = query.value("id");
        activity["title"] = query.value("title");
        activity["description"] = query.value("description");
        activity["category"] = query.value("category");
        activity["organizer"] = query.value("organizer");
        activity["start_time"] = query.value("start_time");
        activity["end_time"] = query.value("end_time");
        activity["max_participants"] = query.value("max_participants");
        activity["current_participants"] = query.value("current_participants");
        activity["location"] = query.value("location");
        activity["status"] = query.value("status");
    }
    
    return activity;
}

bool Database::registerActivity(int activityId, const QString &studentId, const QString &studentName)
{
    QSqlQuery query(db);
    
    // 检查是否已报名
    if (isRegistered(activityId, studentId)) {
        return false;
    }
    
    // 检查活动是否已满
    QHash<QString, QVariant> activity = getActivity(activityId);
    int current = activity["current_participants"].toInt();
    int max = activity["max_participants"].toInt();
    
    if (current >= max) {
        // 添加到候补列表
        return addToWaitlist(activityId, studentId, studentName);
    }
    
    // 添加报名
    query.prepare("INSERT INTO registrations (activity_id, student_id, student_name, status) VALUES (?, ?, ?, ?)");
    query.addBindValue(activityId);
    query.addBindValue(studentId);
    query.addBindValue(studentName);
    query.addBindValue(static_cast<int>(RegistrationStatus::Registered));
    
    if (!query.exec()) {
        return false;
    }
    
    // 更新活动参与人数
    query.prepare("UPDATE activities SET current_participants = current_participants + 1 WHERE id = ?");
    query.addBindValue(activityId);
    query.exec();
    
    return true;
}

bool Database::cancelRegistration(int activityId, const QString &studentId)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM registrations WHERE activity_id = ? AND student_id = ?");
    query.addBindValue(activityId);
    query.addBindValue(studentId);
    
    if (!query.exec()) {
        return false;
    }
    
    // 更新活动参与人数
    query.prepare("UPDATE activities SET current_participants = current_participants - 1 WHERE id = ?");
    query.addBindValue(activityId);
    query.exec();
    
    // 从候补列表中提升一个学生
    promoteFromWaitlist(activityId);
    
    return true;
}

bool Database::isRegistered(int activityId, const QString &studentId)
{
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM registrations WHERE activity_id = ? AND student_id = ?");
    query.addBindValue(activityId);
    query.addBindValue(studentId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

QList<QHash<QString, QVariant>> Database::getRegistrations(int activityId)
{
    QList<QHash<QString, QVariant>> registrations;
    QSqlQuery query(db);
    query.prepare("SELECT * FROM registrations WHERE activity_id = ? ORDER BY registered_at");
    query.addBindValue(activityId);
    
    if (query.exec()) {
        while (query.next()) {
            QHash<QString, QVariant> reg;
            reg["id"] = query.value("id");
            reg["activity_id"] = query.value("activity_id");
            reg["student_id"] = query.value("student_id");
            reg["student_name"] = query.value("student_name");
            reg["status"] = query.value("status");
            reg["registered_at"] = query.value("registered_at");
            registrations.append(reg);
        }
    }
    
    return registrations;
}

QList<QHash<QString, QVariant>> Database::getStudentRegistrations(const QString &studentId)
{
    QList<QHash<QString, QVariant>> registrations;
    QSqlQuery query(db);
    query.prepare(R"(
        SELECT r.*, a.title, a.start_time, a.end_time, a.location
        FROM registrations r
        JOIN activities a ON r.activity_id = a.id
        WHERE r.student_id = ?
        ORDER BY a.start_time
    )");
    query.addBindValue(studentId);
    
    if (query.exec()) {
        while (query.next()) {
            QHash<QString, QVariant> reg;
            reg["id"] = query.value("id");
            reg["activity_id"] = query.value("activity_id");
            reg["student_id"] = query.value("student_id");
            reg["student_name"] = query.value("student_name");
            reg["title"] = query.value("title");
            reg["start_time"] = query.value("start_time");
            reg["end_time"] = query.value("end_time");
            reg["location"] = query.value("location");
            reg["status"] = query.value("status");
            registrations.append(reg);
        }
    }
    
    return registrations;
}

int Database::getRegistrationCount(int activityId)
{
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM registrations WHERE activity_id = ?");
    query.addBindValue(activityId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    
    return 0;
}

bool Database::addToWaitlist(int activityId, const QString &studentId, const QString &studentName)
{
    QSqlQuery query(db);
    query.prepare("INSERT OR IGNORE INTO waitlist (activity_id, student_id, student_name) VALUES (?, ?, ?)");
    query.addBindValue(activityId);
    query.addBindValue(studentId);
    query.addBindValue(studentName);
    
    return query.exec();
}

QList<QHash<QString, QVariant>> Database::getWaitlist(int activityId)
{
    QList<QHash<QString, QVariant>> waitlist;
    QSqlQuery query(db);
    query.prepare("SELECT * FROM waitlist WHERE activity_id = ? ORDER BY added_at");
    query.addBindValue(activityId);
    
    if (query.exec()) {
        while (query.next()) {
            QHash<QString, QVariant> item;
            item["id"] = query.value("id");
            item["activity_id"] = query.value("activity_id");
            item["student_id"] = query.value("student_id");
            item["student_name"] = query.value("student_name");
            item["added_at"] = query.value("added_at");
            waitlist.append(item);
        }
    }
    
    return waitlist;
}

bool Database::promoteFromWaitlist(int activityId)
{
    QSqlQuery query(db);
    
    // 获取候补列表中的第一个学生
    query.prepare("SELECT student_id, student_name FROM waitlist WHERE activity_id = ? ORDER BY added_at LIMIT 1");
    query.addBindValue(activityId);
    
    if (!query.exec() || !query.next()) {
        return false; // 没有候补学生
    }
    
    QString studentId = query.value(0).toString();
    QString studentName = query.value(1).toString();
    
    // 从候补列表中删除
    query.prepare("DELETE FROM waitlist WHERE activity_id = ? AND student_id = ?");
    query.addBindValue(activityId);
    query.addBindValue(studentId);
    query.exec();
    
    // 添加到报名列表
    query.prepare("INSERT INTO registrations (activity_id, student_id, student_name, status) VALUES (?, ?, ?, ?)");
    query.addBindValue(activityId);
    query.addBindValue(studentId);
    query.addBindValue(studentName);
    query.addBindValue(static_cast<int>(RegistrationStatus::Registered));
    
    return query.exec();
}

QList<QHash<QString, QVariant>> Database::checkTimeConflict(const QString &studentId,
                                                            const QDateTime &startTime,
                                                            const QDateTime &endTime,
                                                            int excludeActivityId)
{
    QList<QHash<QString, QVariant>> conflicts;
    QSqlQuery query(db);
    
    QString sql = R"(
        SELECT a.id, a.title, a.start_time, a.end_time
        FROM activities a
        JOIN registrations r ON a.id = r.activity_id
        WHERE r.student_id = ?
        AND a.status = ?
        AND (
            (a.start_time <= ? AND a.end_time > ?) OR
            (a.start_time < ? AND a.end_time >= ?) OR
            (a.start_time >= ? AND a.end_time <= ?)
        )
    )";
    
    if (excludeActivityId > 0) {
        sql += " AND a.id != ?";
    }
    
    query.prepare(sql);
    query.addBindValue(studentId);
    query.addBindValue(static_cast<int>(ActivityStatus::Approved));
    query.addBindValue(startTime);
    query.addBindValue(startTime);
    query.addBindValue(endTime);
    query.addBindValue(endTime);
    query.addBindValue(startTime);
    query.addBindValue(endTime);
    
    if (excludeActivityId > 0) {
        query.addBindValue(excludeActivityId);
    }
    
    if (query.exec()) {
        while (query.next()) {
            QHash<QString, QVariant> conflict;
            conflict["id"] = query.value("id");
            conflict["title"] = query.value("title");
            conflict["start_time"] = query.value("start_time");
            conflict["end_time"] = query.value("end_time");
            conflicts.append(conflict);
        }
    }
    
    return conflicts;
}

QHash<QString, QVariant> Database::getActivityStatistics(int activityId)
{
    QHash<QString, QVariant> stats;
    QSqlQuery query(db);
    
    query.prepare(R"(
        SELECT 
            COUNT(DISTINCT r.id) as total_registrations,
            COUNT(DISTINCT w.id) as total_waitlist,
            a.max_participants,
            a.current_participants
        FROM activities a
        LEFT JOIN registrations r ON a.id = r.activity_id
        LEFT JOIN waitlist w ON a.id = w.activity_id
        WHERE a.id = ?
    )");
    query.addBindValue(activityId);
    
    if (query.exec() && query.next()) {
        stats["total_registrations"] = query.value("total_registrations");
        stats["total_waitlist"] = query.value("total_waitlist");
        stats["max_participants"] = query.value("max_participants");
        stats["current_participants"] = query.value("current_participants");
    }
    
    return stats;
}

QList<QHash<QString, QVariant>> Database::getAllStatistics()
{
    QList<QHash<QString, QVariant>> allStats;
    QSqlQuery query(db);
    
    query.prepare(R"(
        SELECT 
            a.id,
            a.title,
            a.category,
            a.organizer,
            a.start_time,
            a.end_time,
            a.max_participants,
            COUNT(DISTINCT r.id) as current_participants,
            COUNT(DISTINCT w.id) as waitlist_count,
            a.status
        FROM activities a
        LEFT JOIN registrations r ON a.id = r.activity_id
        LEFT JOIN waitlist w ON a.id = w.activity_id
        GROUP BY a.id
        ORDER BY a.start_time
    )");
    
    if (query.exec()) {
        while (query.next()) {
            QHash<QString, QVariant> stat;
            stat["id"] = query.value("id");
            stat["title"] = query.value("title");
            stat["category"] = query.value("category");
            stat["organizer"] = query.value("organizer");
            stat["start_time"] = query.value("start_time");
            stat["end_time"] = query.value("end_time");
            stat["max_participants"] = query.value("max_participants");
            stat["current_participants"] = query.value("current_participants");
            stat["waitlist_count"] = query.value("waitlist_count");
            stat["status"] = query.value("status");
            allStats.append(stat);
        }
    }
    
    return allStats;
}

bool Database::checkIn(int activityId, const QString &studentId)
{
    // 检查是否已报名
    if (!isRegistered(activityId, studentId)) {
        return false;
    }
    
    // 检查是否已签到
    if (isCheckedIn(activityId, studentId)) {
        return false;
    }
    
    // 检查活动是否已开始
    QHash<QString, QVariant> activity = getActivity(activityId);
    QDateTime startTime = activity["start_time"].toDateTime();
    QDateTime currentTime = QDateTime::currentDateTime();
    
    if (currentTime < startTime) {
        return false; // 活动尚未开始
    }
    
    // 执行签到
    QSqlQuery query(db);
    query.prepare("UPDATE registrations SET checkin_time = ? WHERE activity_id = ? AND student_id = ?");
    query.addBindValue(currentTime);
    query.addBindValue(activityId);
    query.addBindValue(studentId);
    
    return query.exec();
}

bool Database::isCheckedIn(int activityId, const QString &studentId)
{
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM registrations WHERE activity_id = ? AND student_id = ? AND checkin_time IS NOT NULL");
    query.addBindValue(activityId);
    query.addBindValue(studentId);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

QList<QHash<QString, QVariant>> Database::getCheckInList(int activityId)
{
    QList<QHash<QString, QVariant>> checkInList;
    QSqlQuery query(db);
    query.prepare(R"(
        SELECT student_id, student_name, checkin_time
        FROM registrations
        WHERE activity_id = ? AND checkin_time IS NOT NULL
        ORDER BY checkin_time
    )");
    query.addBindValue(activityId);
    
    if (query.exec()) {
        while (query.next()) {
            QHash<QString, QVariant> item;
            item["student_id"] = query.value("student_id");
            item["student_name"] = query.value("student_name");
            item["checkin_time"] = query.value("checkin_time");
            checkInList.append(item);
        }
    }
    
    return checkInList;
}

QHash<QString, QVariant> Database::getCheckInStatistics(int activityId)
{
    QHash<QString, QVariant> stats;
    QSqlQuery query(db);
    
    query.prepare(R"(
        SELECT 
            COUNT(*) as total_registered,
            COUNT(checkin_time) as total_checked_in,
            a.max_participants
        FROM registrations r
        JOIN activities a ON r.activity_id = a.id
        WHERE r.activity_id = ?
    )");
    query.addBindValue(activityId);
    
    if (query.exec() && query.next()) {
        int totalRegistered = query.value("total_registered").toInt();
        int totalCheckedIn = query.value("total_checked_in").toInt();
        int maxParticipants = query.value("max_participants").toInt();
        
        stats["total_registered"] = totalRegistered;
        stats["total_checked_in"] = totalCheckedIn;
        stats["max_participants"] = maxParticipants;
        stats["checkin_rate"] = totalRegistered > 0 ? (double)totalCheckedIn / totalRegistered * 100.0 : 0.0;
        stats["not_checked_in"] = totalRegistered - totalCheckedIn;
    }
    
    return stats;
}

