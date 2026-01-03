#include "registrationmanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>

#include <QTextEdit>   // 新增：文本编辑框（用于详情对话框）
#include <QFormLayout> // 新增：表单布局（用于详情对话框）
#include <QDialog>      // 新增：对话框
#include <QDialogButtonBox>  // 新增：对话框按钮框
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
        // 学生角色：使用标签页
        tabWidget = new QTabWidget();
        
        // 标签1：可报名活动
        QWidget *availableTab = new QWidget();
        QVBoxLayout *availableLayout = new QVBoxLayout(availableTab);
        
        QHBoxLayout *availableButtonLayout = new QHBoxLayout();
        viewDetailsButton = new QPushButton("查看详情并报名");
        availableButtonLayout->addWidget(viewDetailsButton);
        availableButtonLayout->addStretch();
        availableLayout->addLayout(availableButtonLayout);
        
        availableActivitiesTable = new QTableWidget();
        availableActivitiesTable->setColumnCount(7);
        QStringList headers = {"ID", "标题", "类别", "发起人", "开始时间", "结束时间", "剩余名额"};
        availableActivitiesTable->setHorizontalHeaderLabels(headers);
        availableActivitiesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        availableActivitiesTable->setSelectionMode(QAbstractItemView::SingleSelection);
        availableActivitiesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        availableActivitiesTable->horizontalHeader()->setStretchLastSection(true);
        availableLayout->addWidget(availableActivitiesTable);
        
        connect(viewDetailsButton, &QPushButton::clicked, this, &RegistrationManager::onViewActivityDetails);
        connect(availableActivitiesTable, &QTableWidget::itemDoubleClicked, this, &RegistrationManager::onViewActivityDetails);
        
        tabWidget->addTab(availableTab, "可报名活动");
        
        // 标签2：我的报名
        QWidget *myRegistrationsTab = new QWidget();
        QVBoxLayout *myRegistrationsLayout = new QVBoxLayout(myRegistrationsTab);
        
        QHBoxLayout *myRegButtonLayout = new QHBoxLayout();
        cancelButton = new QPushButton("取消报名");
        myRegButtonLayout->addWidget(cancelButton);
        myRegButtonLayout->addStretch();
        myRegistrationsLayout->addLayout(myRegButtonLayout);
        
        registrationsTable = new QTableWidget();
        registrationsTable->setColumnCount(6);
        QStringList headers2 = {"活动ID", "活动标题", "开始时间", "结束时间", "地点", "状态"};
        registrationsTable->setHorizontalHeaderLabels(headers2);
        registrationsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        registrationsTable->setSelectionMode(QAbstractItemView::SingleSelection);
        registrationsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        registrationsTable->horizontalHeader()->setStretchLastSection(true);
        myRegistrationsLayout->addWidget(registrationsTable);
        
        connect(cancelButton, &QPushButton::clicked, this, &RegistrationManager::onCancelRegistration);
        connect(registrationsTable, &QTableWidget::itemSelectionChanged, this, &RegistrationManager::onRegistrationSelectionChanged);
        
        tabWidget->addTab(myRegistrationsTab, "我的报名");
        
        mainLayout->addLayout(buttonLayout);
        mainLayout->addWidget(tabWidget);
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
            
            // 获取报名列表
            QList<QHash<QString, QVariant>> registrations = database->getRegistrations(activityId);
            
            // 清空表格内容
            registrationsTable->clearContents();
            registrationsTable->setRowCount(registrations.size());
            
            // 如果没有报名记录
            if (registrations.isEmpty()) {
                statusLabel->setText(QString("活动ID %1 暂无报名记录").arg(activityId));
                QMessageBox::information(this, "提示", "该活动暂无报名记录！");
                return;
            }
            
            // 填充表格数据
            for (int i = 0; i < registrations.size(); ++i) {
                const auto &reg = registrations[i];
                
                // 确保数据不为空
                QString studentId = reg["student_id"].toString();
                QString studentName = reg["student_name"].toString();
                QString registeredAt = reg["registered_at"].toDateTime().toString("yyyy-MM-dd hh:mm");
                QString activityIdStr = reg["activity_id"].toString();
                
                registrationsTable->setItem(i, 0, new QTableWidgetItem(studentId.isEmpty() ? "未知" : studentId));
                registrationsTable->setItem(i, 1, new QTableWidgetItem(studentName.isEmpty() ? "未知" : studentName));
                registrationsTable->setItem(i, 2, new QTableWidgetItem(registeredAt));
                
                QString statusText;
                RegistrationStatus status = static_cast<RegistrationStatus>(reg["status"].toInt());
                switch (status) {
                    case RegistrationStatus::Registered: statusText = "已报名"; break;
                    case RegistrationStatus::Cancelled: statusText = "已取消"; break;
                    case RegistrationStatus::Waitlisted: statusText = "候补"; break;
                    case RegistrationStatus::Confirmed: statusText = "已确认"; break;
                    default: statusText = "未知"; break;
                }
                registrationsTable->setItem(i, 3, new QTableWidgetItem(statusText));
                registrationsTable->setItem(i, 4, new QTableWidgetItem(activityIdStr));
            }
            
            // 调整列宽以适应内容
            registrationsTable->resizeColumnsToContents();
            
            // 确保表格可见
            registrationsTable->show();
            
            // 更新状态标签
            statusLabel->setText(QString("活动报名列表：共 %1 人").arg(registrations.size()));
            
            // 调试输出
            qDebug() << "显示报名列表，活动ID:" << activityId << "，报名人数:" << registrations.size();
        });
        
        connect(waitlistButton, &QPushButton::clicked, this, &RegistrationManager::onViewWaitlist);
        connect(exportButton, &QPushButton::clicked, this, &RegistrationManager::onExportCSV);
    }
    
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
    
    // 报名列表表格（仅组织者和管理员使用）
    if (userRole != UserRole::Student) {
        registrationsTable = new QTableWidget();
        registrationsTable->setColumnCount(5);
        QStringList headers = {"学号", "姓名", "报名时间", "状态", "活动ID"};
        registrationsTable->setHorizontalHeaderLabels(headers);
        
        registrationsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        registrationsTable->setSelectionMode(QAbstractItemView::SingleSelection);
        registrationsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        registrationsTable->horizontalHeader()->setStretchLastSection(true);
        
        connect(registrationsTable, &QTableWidget::itemSelectionChanged, this, &RegistrationManager::onRegistrationSelectionChanged);
        
        mainLayout->addWidget(registrationsTable);
    }
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
        
        // 同时刷新可报名活动列表
        populateAvailableActivities();
        
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

void RegistrationManager::populateAvailableActivities()
{
    if (userRole != UserRole::Student) return;
    
    // 获取所有已批准的活动
    QString filter = "status = " + QString::number(static_cast<int>(ActivityStatus::Approved));
    QList<QHash<QString, QVariant>> activities = database->getActivities(filter);
    
    // 过滤掉已报名的活动
    QList<QHash<QString, QVariant>> availableActivities;
    for (const auto &activity : activities) {
        int activityId = activity["id"].toInt();
        if (!database->isRegistered(activityId, currentStudentId)) {
            availableActivities.append(activity);
        }
    }
    
    availableActivitiesTable->setRowCount(availableActivities.size());
    
    for (int i = 0; i < availableActivities.size(); ++i) {
        const auto &activity = availableActivities[i];
        
        availableActivitiesTable->setItem(i, 0, new QTableWidgetItem(activity["id"].toString()));
        availableActivitiesTable->setItem(i, 1, new QTableWidgetItem(activity["title"].toString()));
        availableActivitiesTable->setItem(i, 2, new QTableWidgetItem(activity["category"].toString()));
        availableActivitiesTable->setItem(i, 3, new QTableWidgetItem(activity["organizer"].toString()));
        availableActivitiesTable->setItem(i, 4, new QTableWidgetItem(activity["start_time"].toDateTime().toString("yyyy-MM-dd hh:mm")));
        availableActivitiesTable->setItem(i, 5, new QTableWidgetItem(activity["end_time"].toDateTime().toString("yyyy-MM-dd hh:mm")));
        
        int max = activity["max_participants"].toInt();
        int current = activity["current_participants"].toInt();
        int remaining = max - current;
        availableActivitiesTable->setItem(i, 6, new QTableWidgetItem(QString::number(remaining)));
    }
    
    statusLabel->setText(QString("可报名活动：共 %1 项").arg(availableActivities.size()));
}

void RegistrationManager::onViewActivityDetails()
{
    if (userRole != UserRole::Student) return;
    
    int row = availableActivitiesTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "提示", "请选择要查看的活动！");
        return;
    }
    
    QTableWidgetItem *item = availableActivitiesTable->item(row, 0);
    if (!item) return;
    
    int activityId = item->text().toInt();
    selectedActivityIdForRegistration = activityId;
    
    showActivityDetailsDialog(activityId);
}

void RegistrationManager::showActivityDetailsDialog(int activityId)
{
    QHash<QString, QVariant> activity = database->getActivity(activityId);
    if (activity.isEmpty()) {
        QMessageBox::warning(this, "错误", "活动不存在！");
        return;
    }
    
    QDialog dialog(this);
    dialog.setWindowTitle("活动详情");
    dialog.setMinimumWidth(600);
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    QLabel *titleLabel = new QLabel("<h2>" + activity["title"].toString() + "</h2>");
    layout->addWidget(titleLabel);
    
    QFormLayout *formLayout = new QFormLayout();
    
    QLabel *categoryLabel = new QLabel(activity["category"].toString());
    QLabel *organizerLabel = new QLabel(activity["organizer"].toString());
    QLabel *startTimeLabel = new QLabel(activity["start_time"].toDateTime().toString("yyyy-MM-dd hh:mm"));
    QLabel *endTimeLabel = new QLabel(activity["end_time"].toDateTime().toString("yyyy-MM-dd hh:mm"));
    QLabel *maxLabel = new QLabel(QString::number(activity["max_participants"].toInt()));
    QLabel *currentLabel = new QLabel(QString::number(activity["current_participants"].toInt()));
    QLabel *locationLabel = new QLabel(activity["location"].toString());
    
    QString statusText;
    ActivityStatus status = static_cast<ActivityStatus>(activity["status"].toInt());
    switch (status) {
        case ActivityStatus::Pending: statusText = "待审批"; break;
        case ActivityStatus::Approved: statusText = "已批准"; break;
        case ActivityStatus::Rejected: statusText = "已拒绝"; break;
        case ActivityStatus::Ongoing: statusText = "进行中"; break;
        case ActivityStatus::Finished: statusText = "已结束"; break;
    }
    QLabel *statusLabel = new QLabel(statusText);
    
    formLayout->addRow("类别：", categoryLabel);
    formLayout->addRow("发起人：", organizerLabel);
    formLayout->addRow("开始时间：", startTimeLabel);
    formLayout->addRow("结束时间：", endTimeLabel);
    formLayout->addRow("最大人数：", maxLabel);
    formLayout->addRow("当前人数：", currentLabel);
    formLayout->addRow("地点：", locationLabel);
    formLayout->addRow("状态：", statusLabel);
    
    layout->addLayout(formLayout);
    
    QLabel *descLabel = new QLabel("描述：");
    layout->addWidget(descLabel);
    QTextEdit *descEdit = new QTextEdit();
    descEdit->setPlainText(activity["description"].toString());
    descEdit->setReadOnly(true);
    descEdit->setMaximumHeight(150);
    layout->addWidget(descEdit);
    
    // 添加报名按钮
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText("立即报名");
    buttonBox->button(QDialogButtonBox::Cancel)->setText("关闭");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &RegistrationManager::onRegisterFromDetails);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    layout->addWidget(buttonBox);
    
    dialog.exec();
}

void RegistrationManager::onRegisterFromDetails()
{
    int activityId = selectedActivityIdForRegistration;
    if (activityId <= 0) return;
    
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
        refreshRegistrations();
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
            refreshRegistrations();
        } else {
            QMessageBox::warning(this, "失败", "报名失败！");
        }
    }
}