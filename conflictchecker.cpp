#include "conflictchecker.h"
#include <QDebug>

ConflictChecker::ConflictChecker(Database *db, QObject *parent)
    : QThread(parent)
    , database(db)
    , activityId(-1)
{
}

void ConflictChecker::setStudentId(const QString &studentId)
{
    this->studentId = studentId;
}

void ConflictChecker::setActivityTime(int activityId, const QDateTime &startTime, const QDateTime &endTime)
{
    this->activityId = activityId;
    this->startTime = startTime;
    this->endTime = endTime;
}

void ConflictChecker::run()
{
    if (studentId.isEmpty() || activityId <= 0) {
        emit checkCompleted(false);
        return;
    }
    
    QList<QHash<QString, QVariant>> conflicts = database->checkTimeConflict(
        studentId, startTime, endTime, activityId);
    
    if (!conflicts.isEmpty()) {
        emit conflictDetected(conflicts);
        emit checkCompleted(true);
    } else {
        emit checkCompleted(false);
    }
}

