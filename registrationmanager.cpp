#include "registrationmanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QHeaderView>
#include <QDebug>
#include "csvexporter.h"
#include "conflictchecker.h"

RegistrationManager::RegistrationManager(Database *db, UserRole role, const QString &username, QWidget *parent)
    : QWidget(parent)
    , database(db)
    , userRole(role)
    , currentUsername(username)
{
    // 如果是学生，需要输入学号和姓名
    if (role == UserRole::Student) {
        currentStudentId = QInputDialog::getText(this, "学生信息", "请输入学号：", QLineEdit::Normal, "").trimmed();
        if (currentStudentId.isEmpty()) {
            QMessageBox::warning(this, "错误", "学号不能为空！");
            return;
        }
        currentStudentName = QInputDialog::getText(this, "学生信息", "请输入姓名：", QLineEdit::Normal, "").trimmed();
        if (currentStudentName.isEmpty()) {
            QMessageBox::warning(this, "错误", "姓名不能为空！");
            return;
        }
    }
    
    setupUI();
    refreshRegistrations();
}

void RegistrationManager::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 状态标签
    statusLabel = new QLabel();
    mainLayout->addWidget(statusLabel);
    
    // 按钮栏
    QHBoxLayout *buttonLayout = new QHBoxLayout();
     // 添加刷新按钮（在所有按钮之前）
    refreshButton = new QPushButton("刷新");
    refreshButton->setShortcut(QKeySequence::Refresh);  // F5快捷键
    buttonLayout->addWidget(refreshButton);
    connect(refreshButton, &QPushButton::clicked, this, &RegistrationManager::onRefreshRegistrations);

    if (userRole == UserRole::Student) {
        registerButton = new QPushButton("报名活动");
        cancelButton = new QPushButton("取消报名");
        buttonLayout->addWidget(registerButton);
        buttonLayout->addWidget(cancelButton);
        connect(registerButton, &QPushButton::clicked, this, &RegistrationManager::onRegisterActivity);
        connect(cancelButton, &QPushButton::clicked, this, &RegistrationManager::onCancelRegistration);
    }
    
    if (userRole == UserRole::Organizer || userRole == UserRole::Admin) {
        QLabel *activityLabel = new QLabel("选择活动：");
        activityComboBox = new QComboBox();
        selectActivityButton = new QPushButton("查看报名");
        waitlistButton = new QPushButton("查看候补");
        exportButton = new QPushButton("导出CSV");
        
        // 填充活动列表
        QList<QHash<QString, QVariant>> activities;
        if (userRole == UserRole::Organizer) {
            activities = database->getActivities(QString("organizer = '%1'").arg(currentUsername));
        } else {
            activities = database->getActivities();
        }
        
        activityComboBox->addItem("请选择活动", -1);
        for (const auto &activity : activities) {
            QString text = QString("%1 - %2").arg(activity["id"].toString()).arg(activity["title"].toString());
            activityComboBox->addItem(text, activity["id"].toInt());
        }
        
        buttonLayout->addWidget(activityLabel);
        buttonLayout->addWidget(activityComboBox);
        buttonLayout->addWidget(selectActivityButton);
        buttonLayout->addWidget(waitlistButton);
        buttonLayout->addWidget(exportButton);
        
        connect(selectActivityButton, &QPushButton::clicked, this, [this]() {
            int activityId = activityComboBox->currentData().toInt();
            if (activityId <= 0) {
                QMessageBox::warning(this, "提示", "请选择活动！");
                return;
            }
            QList<QHash<QString, QVariant>> registrations = database->getRegistrations(activityId);
            registrationsTable->setRowCount(registrations.size());
            
            for (int i = 0; i < registrations.size(); ++i) {
                const auto &reg = registrations[i];
                registrationsTable->setItem(i, 0, new QTableWidgetItem(reg["student_id"].toString()));
                registrationsTable->setItem(i, 1, new QTableWidgetItem(reg["student_name"].toString()));
                registrationsTable->setItem(i, 2, new QTableWidgetItem(reg["registered_at"].toDateTime().toString("yyyy-MM-dd hh:mm")));
                
                QString statusText;
                RegistrationStatus status = static_cast<RegistrationStatus>(reg["status"].toInt());
                switch (status) {
                    case RegistrationStatus::Registered: statusText = "已报名"; break;
                    case RegistrationStatus::Cancelled: statusText = "已取消"; break;
                    case RegistrationStatus::Waitlisted: statusText = "候补"; break;
                    case RegistrationStatus::Confirmed: statusText = "已确认"; break;
                }
                registrationsTable->setItem(i, 3, new QTableWidgetItem(statusText));
                registrationsTable->setItem(i, 4, new QTableWidgetItem(reg["activity_id"].toString()));
            }
            
            statusLabel->setText(QString("活动报名列表：共 %1 人").arg(registrations.size()));
        });
        
        connect(waitlistButton, &QPushButton::clicked, this, &RegistrationManager::onViewWaitlist);
        connect(exportButton, &QPushButton::clicked, this, &RegistrationManager::onExportCSV);
    }
    
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
    
    // 报名列表表格
    registrationsTable = new QTableWidget();
    
    if (userRole == UserRole::Student) {
        registrationsTable->setColumnCount(6);
        QStringList headers = {"活动ID", "活动标题", "开始时间", "结束时间", "地点", "状态"};
        registrationsTable->setHorizontalHeaderLabels(headers);
    } else {
        registrationsTable->setColumnCount(5);
        QStringList headers = {"学号", "姓名", "报名时间", "状态", "活动ID"};
        registrationsTable->setHorizontalHeaderLabels(headers);
    }
    
    registrationsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    registrationsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    registrationsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    registrationsTable->horizontalHeader()->setStretchLastSection(true);
    
    connect(registrationsTable, &QTableWidget::itemSelectionChanged, this, &RegistrationManager::onRegistrationSelectionChanged);
    
    mainLayout->addWidget(registrationsTable);
}

void RegistrationManager::refreshRegistrations()
{
    populateTable();
}

void RegistrationManager::populateTable()
{
    if (userRole == UserRole::Student) {
        QList<QHash<QString, QVariant>> registrations = database->getStudentRegistrations(currentStudentId);
        registrationsTable->setRowCount(registrations.size());
        
        for (int i = 0; i < registrations.size(); ++i) {
            const auto &reg = registrations[i];
            
            registrationsTable->setItem(i, 0, new QTableWidgetItem(reg["activity_id"].toString()));
            registrationsTable->setItem(i, 1, new QTableWidgetItem(reg["title"].toString()));
            registrationsTable->setItem(i, 2, new QTableWidgetItem(reg["start_time"].toDateTime().toString("yyyy-MM-dd hh:mm")));
            registrationsTable->setItem(i, 3, new QTableWidgetItem(reg["end_time"].toDateTime().toString("yyyy-MM-dd hh:mm")));
            registrationsTable->setItem(i, 4, new QTableWidgetItem(reg["location"].toString()));
            
            QString statusText;
            RegistrationStatus status = static_cast<RegistrationStatus>(reg["status"].toInt());
            switch (status) {
                case RegistrationStatus::Registered: statusText = "已报名"; break;
                case RegistrationStatus::Cancelled: statusText = "已取消"; break;
                case RegistrationStatus::Waitlisted: statusText = "候补"; break;
                case RegistrationStatus::Confirmed: statusText = "已确认"; break;
            }
            registrationsTable->setItem(i, 5, new QTableWidgetItem(statusText));
        }
        
        statusLabel->setText(QString("我的报名：共 %1 项").arg(registrations.size()));
    } else {
        // 组织者和管理员需要先选择活动
        statusLabel->setText("请先选择活动查看报名列表");
    }
}

void RegistrationManager::onRegisterActivity()
{
    bool ok;
    int activityId = QInputDialog::getInt(this, "报名活动", "请输入活动ID：", 1, 1, 100000, 1, &ok);
    if (!ok) return;
    
    QHash<QString, QVariant> activity = database->getActivity(activityId);
    if (activity.isEmpty()) {
        QMessageBox::warning(this, "错误", "活动不存在！");
        return;
    }
    
    ActivityStatus status = static_cast<ActivityStatus>(activity["status"].toInt());
    if (status != ActivityStatus::Approved) {
        QMessageBox::warning(this, "错误", "该活动尚未批准，无法报名！");
        return;
    }
    
    // 检查是否已报名
    if (database->isRegistered(activityId, currentStudentId)) {
        QMessageBox::information(this, "提示", "您已经报名了此活动！");
        return;
    }
    
    // 检查时间冲突
    QDateTime startTime = activity["start_time"].toDateTime();
    QDateTime endTime = activity["end_time"].toDateTime();
    QList<QHash<QString, QVariant>> conflicts = database->checkTimeConflict(currentStudentId, startTime, endTime);
    
    if (!conflicts.isEmpty()) {
        showConflictDialog(conflicts);
        if (QMessageBox::question(this, "时间冲突", "检测到时间冲突，是否仍要报名？") != QMessageBox::Yes) {
            return;
        }
    }
    
    // 执行报名
    if (database->registerActivity(activityId, currentStudentId, currentStudentName)) {
        QMessageBox::information(this, "成功", "报名成功！");
        refreshRegistrations();
    } else {
        // 可能已满，添加到候补
        if (database->addToWaitlist(activityId, currentStudentId, currentStudentName)) {
            QMessageBox::information(this, "提示", "活动已满，已加入候补列表！");
        } else {
            QMessageBox::warning(this, "失败", "报名失败！");
        }
    }
}

void RegistrationManager::onCancelRegistration()
{
    int activityId = getSelectedActivityId();
    if (activityId <= 0) {
        QMessageBox::warning(this, "提示", "请选择要取消的报名！");
        return;
    }
    
    if (QMessageBox::question(this, "确认", "确定要取消报名吗？") == QMessageBox::Yes) {
        if (database->cancelRegistration(activityId, currentStudentId)) {
            QMessageBox::information(this, "成功", "已取消报名！");
            refreshRegistrations();
        } else {
            QMessageBox::warning(this, "失败", "取消报名失败！");
        }
    }
}

void RegistrationManager::onViewWaitlist()
{
    int activityId = -1;
    if (userRole == UserRole::Organizer || userRole == UserRole::Admin) {
        activityId = activityComboBox->currentData().toInt();
        if (activityId <= 0) {
            QMessageBox::warning(this, "提示", "请先选择活动！");
            return;
        }
    } else {
        bool ok;
        activityId = QInputDialog::getInt(this, "查看候补", "请输入活动ID：", 1, 1, 100000, 1, &ok);
        if (!ok) return;
    }
    
    QList<QHash<QString, QVariant>> waitlist = database->getWaitlist(activityId);
    
    if (waitlist.isEmpty()) {
        QMessageBox::information(this, "提示", "该活动没有候补学生！");
        return;
    }
    
    QString message = QString("候补列表（共 %1 人）：\n\n").arg(waitlist.size());
    for (int i = 0; i < waitlist.size(); ++i) {
        const auto &item = waitlist[i];
        message += QString("%1. %2 (%3) - %4\n")
            .arg(i + 1)
            .arg(item["student_name"].toString())
            .arg(item["student_id"].toString())
            .arg(item["added_at"].toDateTime().toString("yyyy-MM-dd hh:mm"));
    }
    
    QMessageBox::information(this, "候补列表", message);
}

void RegistrationManager::onExportCSV()
{
    int activityId = -1;
    if (userRole == UserRole::Organizer || userRole == UserRole::Admin) {
        activityId = activityComboBox->currentData().toInt();
        if (activityId <= 0) {
            QMessageBox::warning(this, "提示", "请先选择活动！");
            return;
        }
    } else {
        bool ok;
        activityId = QInputDialog::getInt(this, "导出报名名单", "请输入活动ID：", 1, 1, 100000, 1, &ok);
        if (!ok) return;
    }
    
    QList<QHash<QString, QVariant>> registrations = database->getRegistrations(activityId);
    QHash<QString, QVariant> activity = database->getActivity(activityId);
    
    if (registrations.isEmpty()) {
        QMessageBox::information(this, "提示", "该活动没有报名记录！");
        return;
    }
    
    QString filename = QFileDialog::getSaveFileName(this, "保存CSV文件", 
        activity["title"].toString() + "_报名名单.csv", "CSV Files (*.csv)");
    
    if (!filename.isEmpty()) {
        CsvExporter exporter;
        if (exporter.exportRegistrations(filename, registrations, activity)) {
            QMessageBox::information(this, "成功", "导出成功！");
        } else {
            QMessageBox::warning(this, "失败", "导出失败！");
        }
    }
}

void RegistrationManager::onRegistrationSelectionChanged()
{
    bool hasSelection = registrationsTable->currentRow() >= 0;
    if (userRole == UserRole::Student) {
        cancelButton->setEnabled(hasSelection);
    }
}

int RegistrationManager::getSelectedActivityId()
{
    int row = registrationsTable->currentRow();
    if (row < 0) return -1;
    
    QTableWidgetItem *item = registrationsTable->item(row, 0);
    if (!item) return -1;
    
    return item->text().toInt();
}

void RegistrationManager::showConflictDialog(const QList<QHash<QString, QVariant>> &conflicts)
{
    QString message = "检测到以下时间冲突的活动：\n\n";
    for (const auto &conflict : conflicts) {
        message += QString("• %1\n  时间：%2 - %3\n\n")
            .arg(conflict["title"].toString())
            .arg(conflict["start_time"].toDateTime().toString("yyyy-MM-dd hh:mm"))
            .arg(conflict["end_time"].toDateTime().toString("yyyy-MM-dd hh:mm"));
    }
    
    QMessageBox::warning(this, "时间冲突", message);
}

void RegistrationManager::onRefreshRegistrations()
{
    refreshRegistrations();
}