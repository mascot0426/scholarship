#include "activitymanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QTimer>
#include <QDebug>

ActivityManager::ActivityManager(Database *db, UserRole role, const QString &studentId, NetworkManager *networkMgr, QWidget *parent)
    : QWidget(parent)
    , database(db)
    , networkManager(networkMgr)
    , userRole(role)
    , currentStudentId(studentId)
{
    setupUI();
    refreshActivities();
}

void ActivityManager::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 搜索栏
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLineEdit = new QLineEdit();
    searchLineEdit->setPlaceholderText("搜索活动（标题、类别、发起人）");
    searchButton = new QPushButton("搜索");
    searchLayout->addWidget(searchLineEdit);
    searchLayout->addWidget(searchButton);
    mainLayout->addLayout(searchLayout);
    
    // 按钮栏
    QHBoxLayout *buttonLayout = new QHBoxLayout();

     // 添加刷新按钮（在所有按钮之前）
     refreshButton = new QPushButton("刷新");
     refreshButton->setShortcut(QKeySequence::Refresh);  // F5快捷键
     buttonLayout->addWidget(refreshButton);
     connect(refreshButton, &QPushButton::clicked, this, &ActivityManager::onRefreshActivities);

    if (userRole == UserRole::Organizer || userRole == UserRole::Admin) {
        createButton = new QPushButton("发布活动");
        buttonLayout->addWidget(createButton);
        connect(createButton, &QPushButton::clicked, this, &ActivityManager::onCreateActivity);
    }
    
    if (userRole == UserRole::Admin) {
        approveButton = new QPushButton("批准");
        rejectButton = new QPushButton("拒绝");
        buttonLayout->addWidget(approveButton);
        buttonLayout->addWidget(rejectButton);
        connect(approveButton, &QPushButton::clicked, this, &ActivityManager::onApproveActivity);
        connect(rejectButton, &QPushButton::clicked, this, &ActivityManager::onRejectActivity);
    }
    
    viewButton = new QPushButton("查看详情");
    buttonLayout->addWidget(viewButton);
    buttonLayout->addStretch();
    
    connect(viewButton, &QPushButton::clicked, this, &ActivityManager::onViewDetails);
    connect(searchButton, &QPushButton::clicked, this, &ActivityManager::onSearchActivities);
    
    mainLayout->addLayout(buttonLayout);
    
    // 活动列表表格
    activitiesTable = new QTableWidget();
    activitiesTable->setColumnCount(7);
    QStringList headers = {"ID", "标题", "类别", "发起人", "开始时间", "结束时间", "状态"};
    activitiesTable->setHorizontalHeaderLabels(headers);
    activitiesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    activitiesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    activitiesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    activitiesTable->horizontalHeader()->setStretchLastSection(true);
    
    connect(activitiesTable, &QTableWidget::itemDoubleClicked, this, &ActivityManager::onViewDetails);
    connect(activitiesTable, &QTableWidget::itemSelectionChanged, this, &ActivityManager::onActivitySelectionChanged);
    
    mainLayout->addWidget(activitiesTable);
}

void ActivityManager::refreshActivities()
{
    populateTable();
}

void ActivityManager::populateTable()
{
    QString filter = "";
    
    // 根据角色过滤
    if (userRole == UserRole::Organizer) {
        filter = QString("organizer = '%1'").arg(currentStudentId);
    } else if (userRole == UserRole::Student) {
        filter = "status = " + QString::number(static_cast<int>(ActivityStatus::Approved));
    }
    
    // 搜索过滤
    QString searchText = searchLineEdit->text().trimmed();
    if (!searchText.isEmpty()) {
        if (!filter.isEmpty()) filter += " AND ";
        filter += QString("(title LIKE '%%1%' OR category LIKE '%%1%' OR organizer LIKE '%%1%')").arg(searchText);
    }
    
    QList<QHash<QString, QVariant>> activities = database->getActivities(filter);
    
    activitiesTable->setRowCount(activities.size());
    
    for (int i = 0; i < activities.size(); ++i) {
        const auto &activity = activities[i];
        
        activitiesTable->setItem(i, 0, new QTableWidgetItem(activity["id"].toString()));
        activitiesTable->setItem(i, 1, new QTableWidgetItem(activity["title"].toString()));
        activitiesTable->setItem(i, 2, new QTableWidgetItem(activity["category"].toString()));
        activitiesTable->setItem(i, 3, new QTableWidgetItem(activity["organizer"].toString()));
        activitiesTable->setItem(i, 4, new QTableWidgetItem(activity["start_time"].toDateTime().toString("yyyy-MM-dd hh:mm")));
        activitiesTable->setItem(i, 5, new QTableWidgetItem(activity["end_time"].toDateTime().toString("yyyy-MM-dd hh:mm")));
        
        QString statusText;
        ActivityStatus status = static_cast<ActivityStatus>(activity["status"].toInt());
        switch (status) {
            case ActivityStatus::Pending: statusText = "待审批"; break;
            case ActivityStatus::Approved: statusText = "已批准"; break;
            case ActivityStatus::Rejected: statusText = "已拒绝"; break;
            case ActivityStatus::Ongoing: statusText = "进行中"; break;
            case ActivityStatus::Finished: statusText = "已结束"; break;
        }
        activitiesTable->setItem(i, 6, new QTableWidgetItem(statusText));
    }
}

void ActivityManager::onCreateActivity()
{
    QDialog dialog(this);
    dialog.setWindowTitle("发布活动");
    dialog.setMinimumWidth(500);
    
    QFormLayout *formLayout = new QFormLayout(&dialog);
    
    QLineEdit *titleEdit = new QLineEdit();
    QTextEdit *descriptionEdit = new QTextEdit();
    descriptionEdit->setMaximumHeight(100);
    QLineEdit *categoryEdit = new QLineEdit();
    QDateTimeEdit *startTimeEdit = new QDateTimeEdit();
    startTimeEdit->setDateTime(QDateTime::currentDateTime());
    startTimeEdit->setCalendarPopup(true);
    QDateTimeEdit *endTimeEdit = new QDateTimeEdit();
    endTimeEdit->setDateTime(QDateTime::currentDateTime().addDays(1));
    endTimeEdit->setCalendarPopup(true);
    QSpinBox *maxParticipantsEdit = new QSpinBox();
    maxParticipantsEdit->setMinimum(1);
    maxParticipantsEdit->setMaximum(10000);
    maxParticipantsEdit->setValue(50);
    QLineEdit *locationEdit = new QLineEdit();
    QLineEdit *checkinCodeEdit = new QLineEdit();
    checkinCodeEdit->setPlaceholderText("建议6位数字，如：123456");
    
    formLayout->addRow("标题：", titleEdit);
    formLayout->addRow("描述：", descriptionEdit);
    formLayout->addRow("类别：", categoryEdit);
    formLayout->addRow("开始时间：", startTimeEdit);
    formLayout->addRow("结束时间：", endTimeEdit);
    formLayout->addRow("最大人数：", maxParticipantsEdit);
    formLayout->addRow("地点：", locationEdit);
    formLayout->addRow("签到码：", checkinCodeEdit);
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    formLayout->addRow(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted) {
        if (titleEdit->text().isEmpty() || categoryEdit->text().isEmpty()) {
            QMessageBox::warning(this, "错误", "标题和类别不能为空！");
            return;
        }
        
        if (startTimeEdit->dateTime() >= endTimeEdit->dateTime()) {
            QMessageBox::warning(this, "错误", "结束时间必须晚于开始时间！");
            return;
        }
        
        int activityId = database->createActivity(
            titleEdit->text(),
            descriptionEdit->toPlainText(),
            categoryEdit->text(),
            currentStudentId,
            startTimeEdit->dateTime(),
            endTimeEdit->dateTime(),
            maxParticipantsEdit->value(),
            locationEdit->text(),
            checkinCodeEdit->text().trimmed()
        );
        
        if (activityId > 0) {
            QMessageBox::information(this, "成功", "活动发布成功，等待管理员审批！");
            refreshActivities(); // 添加这一行
        } else {    
            QMessageBox::warning(this, "失败", "活动发布失败！");
        }
    }
}

void ActivityManager::onApproveActivity()
{
    int activityId = getSelectedActivityId();
    if (activityId <= 0) {
        QMessageBox::warning(this, "提示", "请选择要批准的活动！");
        return;
    }
    
    // 获取当前活动的签到码
    QHash<QString, QVariant> activity = database->getActivity(activityId);
    QString currentCheckinCode = activity["checkin_code"].toString();
    
    // 显示对话框允许设置/修改签到码
    QDialog checkinCodeDialog(this);
    checkinCodeDialog.setWindowTitle("设置签到码");
    checkinCodeDialog.setMinimumWidth(400);
    
    QFormLayout *formLayout = new QFormLayout(&checkinCodeDialog);
    QLineEdit *checkinCodeEdit = new QLineEdit();
    checkinCodeEdit->setPlaceholderText("建议6位数字，如：123456");
    if (!currentCheckinCode.isEmpty()) {
        checkinCodeEdit->setText(currentCheckinCode);
    }
    
    QLabel *hintLabel = new QLabel("提示：签到码用于学生签到验证，可以为空（学生将无法签到）");
    hintLabel->setWordWrap(true);
    hintLabel->setStyleSheet("color: gray;");
    
    formLayout->addRow("签到码：", checkinCodeEdit);
    formLayout->addRow("", hintLabel);
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    formLayout->addRow(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, &checkinCodeDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &checkinCodeDialog, &QDialog::reject);
    
    if (checkinCodeDialog.exec() == QDialog::Accepted) {
        QString newCheckinCode = checkinCodeEdit->text().trimmed();
        // 更新签到码
        if (!database->updateCheckInCode(activityId, newCheckinCode)) {
            QMessageBox::warning(this, "警告", "更新签到码失败，但将继续审批活动");
        }
    }
    
    if (database->updateActivityStatus(activityId, ActivityStatus::Approved)) {
        QMessageBox::information(this, "成功", "活动已批准！");
        
        // 同步到校园平台
        if (networkManager) {
            activity = database->getActivity(activityId);
            if (!activity.isEmpty()) {
                qDebug() << "[活动批准] 准备同步活动ID:" << activityId;
                networkManager->syncActivityToPlatform(activityId, activity);
                // 连接信号以显示同步结果
                QMetaObject::Connection *connection = new QMetaObject::Connection();
                *connection = connect(networkManager, &NetworkManager::activitySynced, this, [this, activityId, connection](int id, bool success) {
                    qDebug() << "[同步回调] 活动ID:" << id << "成功:" << success;
                    if (id == activityId) {
                        // 使用QTimer延迟显示，确保"活动已批准"对话框已关闭
                        QTimer::singleShot(500, this, [this, success, connection]() {
                            if (success) {
                                qDebug() << "[同步成功] 显示成功提示";
                                QMessageBox::information(this, "同步成功", "活动已同步到校园平台！");
                            } else {
                                qDebug() << "[同步失败] 显示失败提示";
                                QMessageBox::warning(this, "同步失败", "活动同步到校园平台失败，请检查网络连接！");
                            }
                            // 断开连接（单次触发）
                            disconnect(*connection);
                            delete connection;
                        });
                    } else {
                        qDebug() << "[同步回调] 活动ID不匹配，忽略。期望:" << activityId << "实际:" << id;
                        // 即使ID不匹配也要清理连接
                        disconnect(*connection);
                        delete connection;
                    }
                });
            } else {
                qDebug() << "[活动批准] 警告：无法获取活动数据，跳过同步";
            }
        } else {
            qDebug() << "[活动批准] 警告：NetworkManager为空，跳过同步";
        }
        
        refreshActivities();
    } else {
        QMessageBox::warning(this, "失败", "操作失败！");
    }
}

void ActivityManager::onRejectActivity()
{
    int activityId = getSelectedActivityId();
    if (activityId <= 0) {
        QMessageBox::warning(this, "提示", "请选择要拒绝的活动！");
        return;
    }
    
    if (QMessageBox::question(this, "确认", "确定要拒绝此活动吗？") == QMessageBox::Yes) {
        if (database->updateActivityStatus(activityId, ActivityStatus::Rejected)) {
            QMessageBox::information(this, "成功", "活动已拒绝！");
            refreshActivities();
        } else {
            QMessageBox::warning(this, "失败", "操作失败！");
        }
    }
}

void ActivityManager::onViewDetails()
{
    int activityId = getSelectedActivityId();
    if (activityId <= 0) {
        QMessageBox::warning(this, "提示", "请选择要查看的活动！");
        return;
    }
    
    QHash<QString, QVariant> activity = database->getActivity(activityId);
    if (activity.isEmpty()) {
        QMessageBox::warning(this, "错误", "活动不存在！");
        return;
    }
    
    showActivityDialog(activity, true);
}

void ActivityManager::onSearchActivities()
{
    populateTable();
}

void ActivityManager::onActivitySelectionChanged()
{
    bool hasSelection = activitiesTable->currentRow() >= 0;
    viewButton->setEnabled(hasSelection);
    
    if (userRole == UserRole::Admin) {
        approveButton->setEnabled(hasSelection);
        rejectButton->setEnabled(hasSelection);
    }
}

int ActivityManager::getSelectedActivityId()
{
    int row = activitiesTable->currentRow();
    if (row < 0) return -1;
    
    QTableWidgetItem *item = activitiesTable->item(row, 0);
    if (!item) return -1;
    
    return item->text().toInt();
}

void ActivityManager::showActivityDialog(const QHash<QString, QVariant> &activity, bool readOnly)
{
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
    
    // 仅管理员/发起人可见签到码
    if (userRole == UserRole::Admin || userRole == UserRole::Organizer) {
        QString checkinCode = activity["checkin_code"].toString();
        QLabel *checkinCodeLabel = new QLabel(checkinCode.isEmpty() ? "未设置" : checkinCode);
        if (checkinCode.isEmpty()) {
            checkinCodeLabel->setStyleSheet("color: gray;");
        }
        formLayout->addRow("签到码：", checkinCodeLabel);
    }
    
    layout->addLayout(formLayout);
    
    QLabel *descLabel = new QLabel("描述：");
    layout->addWidget(descLabel);
    QTextEdit *descEdit = new QTextEdit();
    descEdit->setPlainText(activity["description"].toString());
    descEdit->setReadOnly(readOnly);
    descEdit->setMaximumHeight(150);
    layout->addWidget(descEdit);
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    layout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    
    dialog.exec();
}

void ActivityManager::onRefreshActivities()
{
    refreshActivities();
}