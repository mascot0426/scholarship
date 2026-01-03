#ifndef CONFLICTCHECKER_H
#define CONFLICTCHECKER_H

#include <QObject>
#include <QThread>
#include <QDateTime>
#include <QVariant>
#include <QHash>
#include "database.h"

class ConflictChecker : public QThread
{
    Q_OBJECT

public:
    explicit ConflictChecker(Database *db, QObject *parent = nullptr);
    void setStudentId(const QString &studentId);
    void setActivityTime(int activityId, const QDateTime &startTime, const QDateTime &endTime);

signals:
    void conflictDetected(const QList<QHash<QString, QVariant>> &conflicts);
    void checkCompleted(bool hasConflict);

protected:
    void run() override;

private:
    Database *database;
    QString studentId;
    int activityId;
    QDateTime startTime;
    QDateTime endTime;
};

#endif // CONFLICTCHECKER_H

