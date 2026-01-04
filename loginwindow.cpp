#include "loginwindow.h"
#include "ui_loginwindow.h"
#include "registerwindow.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>
#include "mainwindow.h"

LoginWindow::LoginWindow(Database *db, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginWindow)
    , database(db)
    , loggedInRole(UserRole::Student)
    , loggedIn(false)
{
    ui->setupUi(this);
    setWindowTitle("校园活动管理系统 - 登录");
    
    // 连接信号和槽
    connect(ui->loginButton, &QPushButton::clicked, this, &LoginWindow::onLoginButtonClicked);
    connect(ui->registerButton, &QPushButton::clicked, this, &LoginWindow::onRegisterButtonClicked);
    connect(ui->newWindowButton, &QPushButton::clicked, this, &LoginWindow::onNewWindowButtonClicked);  // 新增这一行
    // 设置角色选择下拉框
    ui->roleComboBox->addItem("学生", static_cast<int>(UserRole::Student));
    ui->roleComboBox->addItem("发起人", static_cast<int>(UserRole::Organizer));
    ui->roleComboBox->addItem("管理员", static_cast<int>(UserRole::Admin));
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

void LoginWindow::onLoginButtonClicked()
{
    QString studentId = ui->usernameLineEdit->text().trimmed();
    QString password = ui->passwordLineEdit->text();
    int selectedRole = ui->roleComboBox->currentData().toInt();
    
    if (studentId.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "登录失败", "请输入学工号和密码！");
        return;
    }
    
    UserRole role;
    QString name;
    if (database->authenticateUser(studentId, password, role, name)) {
        // 检查角色是否匹配
        if (static_cast<int>(role) != selectedRole) {
            QMessageBox::warning(this, "登录失败", "用户角色不匹配！");
            return;
        }
        
        loggedInRole = role;
        loggedInStudentId = studentId;
        loggedInName = name;
        loggedIn = true;
        
        accept(); // 关闭对话框并返回QDialog::Accepted
    } else {
        QMessageBox::warning(this, "登录失败", "学工号或密码错误！");
        ui->passwordLineEdit->clear();
    }
}

void LoginWindow::onRegisterButtonClicked()
{
    // 打开注册窗口
    RegisterWindow *registerWindow = new RegisterWindow(database, this);
    if (registerWindow->exec() == QDialog::Accepted && registerWindow->isRegistered()) {
        // 注册成功，自动填充学工号
        QString registeredStudentId = registerWindow->getRegisteredStudentId();
        ui->usernameLineEdit->setText(registeredStudentId);
        ui->passwordLineEdit->setFocus();
    }
    registerWindow->deleteLater();
}

void LoginWindow::onNewWindowButtonClicked()
{
    // 创建新的主窗口实例
    MainWindow *newWindow = new MainWindow(nullptr);
    newWindow->setAttribute(Qt::WA_DeleteOnClose);
    newWindow->show();
}