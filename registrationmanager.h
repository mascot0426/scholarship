#ifndef REGISTRATIONMANAGER_H
#define REGISTRATIONMANAGER_H

#include <QWidget>
#include "database.h"

QT_BEGIN_NAMESPACE
class QTableWidget;
class QPushButton;
class QLabel;
class QComboBox;
class QTabWidget;  // 新增：标签页
QT_END_NAMESPACE

class RegistrationManager : public QWidget
{
    Q_OBJECT

public:
    explicit RegistrationManager(Database *db, UserRole role, const QString &studentId, const QString &studentName = "", QWidget *parent = nullptr);
    void refreshRegistrations();

private slots:
    void onRegisterActivity();
    void onCancelRegistration();
    void onViewWaitlist();
    void onExportCSV();
    void onRegistrationSelectionChanged();
    void onRefreshRegistrations();
    void onViewActivityDetails();  // 新增：查看活动详情
    void onRegisterFromDetails();  // 新增：从详情对话框报名
    void onJoinWaitlist();  // 新增：进入候补序列
    void onCheckIn();  // 新增：签到
    void onViewCheckInList();  // 新增：查看签到列表
    void onViewCheckInStatistics();  // 新增：查看签到统计
private:
    Database *database;
    UserRole userRole;
    QString currentStudentId;
    QString currentStudentName;
    QTabWidget *tabWidget;  // 新增：标签页（学生角色使用）
    QTableWidget *registrationsTable;
    QTableWidget *availableActivitiesTable;  // 新增：可报名活动表格
    QPushButton *registerButton;
    QPushButton *cancelButton;
    QPushButton *waitlistButton;
    QPushButton *exportButton;
    QPushButton *selectActivityButton;
    QPushButton *viewDetailsButton;  // 新增：查看详情按钮
    QPushButton *checkInButton;  // 新增：签到按钮
    QPushButton *viewCheckInListButton;  // 新增：查看签到列表按钮
    QPushButton *viewCheckInStatsButton;  // 新增：查看签到统计按钮
    QComboBox *activityComboBox;
    QLabel *statusLabel;
    QPushButton *refreshButton;  // 新增：刷新按钮
    int selectedActivityIdForRegistration;  // 新增：当前选中的活动ID
    void setupUI();
    void populateTable();
    void populateAvailableActivities();  // 新增：填充可报名活动列表
    int getSelectedActivityId();
    void showActivityDetailsDialog(int activityId);  // 新增：显示活动详情对话框
    void showConflictDialog(const QList<QHash<QString, QVariant>> &conflicts);
};

#endif // REGISTRATIONMANAGER_H

