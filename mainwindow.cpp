#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QTimer> 
#include "csvexporter.h"
#include "exportthread.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , database(new Database(this))
    , loginWindow(nullptr)
    , activityManager(nullptr)
    , registrationManager(nullptr)
    , networkManager(new NetworkManager(this))
    , conflictChecker(nullptr)
    , exportThread(nullptr)
    , currentRole(UserRole::Student)
    , isLoggedIn(false)
{
    // 初始化数据库
    if (!database->initializeDatabase()) {
        QMessageBox::critical(this, "错误", "数据库初始化失败！");
        return;
    }
    
    setupUI();
    setupMenuBar();
    showLoginWindow();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("校园活动报名与签到管理系统");
    setMinimumSize(1000, 700);
    
    // 创建中央部件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // 用户信息标签
    QHBoxLayout *userLayout = new QHBoxLayout();
    userLabel = new QLabel("未登录");
    QPushButton *logoutButton = new QPushButton("退出登录");
    userLayout->addWidget(userLabel);
    userLayout->addStretch();
    userLayout->addWidget(logoutButton);
    mainLayout->addLayout(userLayout);
    
    connect(logoutButton, &QPushButton::clicked, this, &MainWindow::onLogout);
    
    // 创建标签页
    tabWidget = new QTabWidget();
    mainLayout->addWidget(tabWidget);
    
    // 状态栏
    statusLabel = new QLabel("就绪");
    statusBar()->addWidget(statusLabel);
}

void MainWindow::setupMenuBar()
{
    QMenuBar *menuBar = this->menuBar();
    
    // 文件菜单
    QMenu *fileMenu = menuBar->addMenu("文件(&F)");

     // 添加"新建窗口"菜单项
    QAction *newWindowAction = fileMenu->addAction("新建窗口(&N)");
    newWindowAction->setShortcut(QKeySequence::New);
    connect(newWindowAction, &QAction::triggered, this, &MainWindow::onNewWindow);
    fileMenu->addSeparator();

    QAction *exportStatsAction = fileMenu->addAction("导出统计报表");
    connect(exportStatsAction, &QAction::triggered, this, &MainWindow::onExportStatistics);
    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction("退出(&X)");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    // 网络菜单
    QMenu *networkMenu = menuBar->addMenu("网络(&N)");
    QAction *fetchCategoriesAction = networkMenu->addAction("获取活动类别");
    QAction *fetchAnnouncementsAction = networkMenu->addAction("获取公告");
    connect(fetchCategoriesAction, &QAction::triggered, this, &MainWindow::onFetchCategories);
    connect(fetchAnnouncementsAction, &QAction::triggered, this, &MainWindow::onFetchAnnouncements);
    
    // 帮助菜单
    QMenu *helpMenu = menuBar->addMenu("帮助(&H)");
    QAction *aboutAction = helpMenu->addAction("关于(&A)");
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "关于", 
            "校园活动报名与签到管理系统\n\n"
            "版本：1.0\n"
            "功能：活动发布、审批、报名、冲突检测、候补排队等");
    });
}

void MainWindow::showLoginWindow()
{
    loginWindow = new LoginWindow(database, this);
    if (loginWindow->exec() == QDialog::Accepted && loginWindow->isLoggedIn()) {
        currentRole = loginWindow->getLoggedInRole();
        currentStudentId = loginWindow->getLoggedInStudentId();
        currentName = loginWindow->getLoggedInName();
        isLoggedIn = true;
        
        updateUIForRole();
        statusLabel->setText(QString("欢迎，%1 (%2)").arg(currentName).arg(currentStudentId));
    } else {
        close();
    }
}

void MainWindow::onLogin()
{
    showLoginWindow();
}

void MainWindow::onNewWindow()
{
    // 创建新窗口实例
    MainWindow *newWindow = new MainWindow(nullptr);  // nullptr 表示独立窗口
    newWindow->setAttribute(Qt::WA_DeleteOnClose);    // 关闭时自动删除
    
        // 可选：为新窗口添加编号以便区分
        static int windowCount = 1;
        QString title = windowTitle();
        if (title.contains("窗口")) {
            // 如果当前窗口已经有编号，新窗口使用下一个编号
            newWindow->setWindowTitle(QString("校园活动管理系统 - 窗口 %1").arg(++windowCount));
        } else {
            // 第一个窗口保持原标题，新窗口添加编号
            setWindowTitle("校园活动管理系统 - 窗口 1");
            newWindow->setWindowTitle("校园活动管理系统 - 窗口 2");
            windowCount = 2;
        }
        
    newWindow->show();
}

void MainWindow::onLogout()
{
    if (QMessageBox::question(this, "确认", "确定要退出登录吗？") == QMessageBox::Yes) {
        isLoggedIn = false;
        currentRole = UserRole::Student;
        currentStudentId.clear();
        currentName.clear();
        
        // 清除标签页
        while (tabWidget->count() > 0) {
            QWidget *widget = tabWidget->widget(0);
            tabWidget->removeTab(0);
            widget->deleteLater();
        }
        
        activityManager = nullptr;
        registrationManager = nullptr;
        
        userLabel->setText("未登录");
        statusLabel->setText("已退出登录");
        
        showLoginWindow();
    }
}

void MainWindow::updateUIForRole()
{
    // 清除现有标签页
    while (tabWidget->count() > 0) {
        QWidget *widget = tabWidget->widget(0);
        tabWidget->removeTab(0);
        widget->deleteLater();
    }
    
    // 更新用户标签
    QString roleText;
    switch (currentRole) {
        case UserRole::Admin: roleText = "管理员"; break;
        case UserRole::Organizer: roleText = "发起人"; break;
        case UserRole::Student: roleText = "学生"; break;
    }
    userLabel->setText(QString("用户：%1 (%2) - %3").arg(currentName).arg(currentStudentId).arg(roleText));
    
    // 创建活动管理标签页
    activityManager = new ActivityManager(database, currentRole, currentStudentId, networkManager, this);
    tabWidget->addTab(activityManager, "活动管理");
    
    // 创建报名管理标签页
    registrationManager = new RegistrationManager(database, currentRole, currentStudentId, currentName, this);
    tabWidget->addTab(registrationManager, "报名管理");
}

void MainWindow::onExportStatistics()
{
    if (!isLoggedIn || currentRole != UserRole::Admin) {
        QMessageBox::warning(this, "提示", "只有管理员可以导出统计报表！");
        return;
    }
    
    QString filename = QFileDialog::getSaveFileName(this, "导出统计报表", 
        "活动统计报表.csv", "CSV Files (*.csv)");
    
    if (!filename.isEmpty()) {
        QList<QHash<QString, QVariant>> statistics = database->getAllStatistics();
        
        CsvExporter exporter;
        if (exporter.exportStatistics(filename, statistics)) {
            QMessageBox::information(this, "成功", "统计报表导出成功！");
            statusLabel->setText("统计报表已导出：" + filename);
        } else {
            QMessageBox::warning(this, "失败", "导出失败！");
        }
    }
}

void MainWindow::onFetchCategories()
{
    statusLabel->setText("正在获取活动类别...");
    networkManager->fetchActivityCategories();
    connect(networkManager, &NetworkManager::categoriesReceived, 
            this, &MainWindow::onCategoriesReceived);
    connect(networkManager, &NetworkManager::errorOccurred,
            this, &MainWindow::onNetworkError);
}

void MainWindow::onFetchAnnouncements()
{
    statusLabel->setText("正在获取公告...");
    networkManager->fetchAnnouncements();
    connect(networkManager, &NetworkManager::announcementsReceived,
            this, &MainWindow::onAnnouncementsReceived);
    connect(networkManager, &NetworkManager::errorOccurred,
            this, &MainWindow::onNetworkError);
}

void MainWindow::onCategoriesReceived(const QStringList &categories)
{
    QString message = "获取到的活动类别：\n";
    for (const QString &category : categories) {
        message += "• " + category + "\n";
    }
    QMessageBox::information(this, "活动类别", message);
    statusLabel->setText("活动类别获取成功");
}

void MainWindow::onAnnouncementsReceived(const QList<QHash<QString, QString>> &announcements)
{
    if (announcements.isEmpty()) {
        QMessageBox::information(this, "公告", "暂无公告");
        statusLabel->setText("暂无公告");
        return;
    }
    
    QString message = "公告列表：\n\n";
    for (const auto &announcement : announcements) {
        message += "标题：" + announcement["title"] + "\n";
        message += "内容：" + announcement["content"] + "\n";
        message += "日期：" + announcement["date"] + "\n\n";
    }
    QMessageBox::information(this, "公告", message);
    statusLabel->setText("公告获取成功");
}

void MainWindow::onNetworkError(const QString &error)
{
    QMessageBox::warning(this, "网络错误", error);
    statusLabel->setText("网络错误：" + error);
}
