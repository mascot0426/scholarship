#ifndef ACTIVITYMANAGER_H
#define ACTIVITYMANAGER_H

#include <QWidget>
#include <QDateTime>
#include "database.h"

QT_BEGIN_NAMESPACE
class QTableWidget;
class QPushButton;
class QLineEdit;
class QTextEdit;
class QDateTimeEdit;
class QSpinBox;
class QComboBox;
class QLabel;
QT_END_NAMESPACE

class ActivityManager : public QWidget
{
    Q_OBJECT

public:
    explicit ActivityManager(Database *db, UserRole role, const QString &username, QWidget *parent = nullptr);
    void refreshActivities();

private slots:
    void onCreateActivity();
    void onApproveActivity();
    void onRejectActivity();
    void onViewDetails();
    void onSearchActivities();
    void onActivitySelectionChanged();
    void onRefreshActivities();

private:
    Database *database;
    UserRole userRole;
    QString currentUsername;
    
    QTableWidget *activitiesTable;
    QPushButton *createButton;
    QPushButton *approveButton;
    QPushButton *rejectButton;
    QPushButton *viewButton;
    QPushButton *searchButton;
    QLineEdit *searchLineEdit;
    QPushButton *refreshButton;  // 新增：刷新按钮
    void setupUI();
    void populateTable();
    int getSelectedActivityId();
    void showActivityDialog(const QHash<QString, QVariant> &activity, bool readOnly = false);
};

#endif // ACTIVITYMANAGER_H

