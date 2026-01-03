#include "loginwindow.h"
#include "ui_loginwindow.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>

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
    QString username = ui->usernameLineEdit->text().trimmed();
    QString password = ui->passwordLineEdit->text();
    int selectedRole = ui->roleComboBox->currentData().toInt();
    
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "登录失败", "请输入用户名和密码！");
        return;
    }
    
    UserRole role;
    QString name;
    if (database->authenticateUser(username, password, role, name)) {
        // 检查角色是否匹配
        if (static_cast<int>(role) != selectedRole) {
            QMessageBox::warning(this, "登录失败", "用户角色不匹配！");
            return;
        }
        
        loggedInRole = role;
        loggedInUsername = username;
        loggedInName = name;
        loggedIn = true;
        
        accept(); // 关闭对话框并返回QDialog::Accepted
    } else {
        QMessageBox::warning(this, "登录失败", "用户名或密码错误！");
        ui->passwordLineEdit->clear();
    }
}

void LoginWindow::onRegisterButtonClicked()
{
    QString username = ui->usernameLineEdit->text().trimmed();
    QString password = ui->passwordLineEdit->text();
    int selectedRole = ui->roleComboBox->currentData().toInt();
    
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "注册失败", "请输入用户名和密码！");
        return;
    }
    
    if (password.length() < 6) {
        QMessageBox::warning(this, "注册失败", "密码长度至少6位！");
        return;
    }
    
    // 学生和发起人可以注册，管理员只能由系统创建
    if (selectedRole == static_cast<int>(UserRole::Admin)) {
        QMessageBox::warning(this, "注册失败", "管理员账户不能通过注册创建！");
        return;
    }
    
    QString name = QInputDialog::getText(this, "注册", "请输入姓名：", QLineEdit::Normal, "");
    if (name.isEmpty()) {
        return;
    }
    
    if (database->addUser(username, password, static_cast<UserRole>(selectedRole), name)) {
        QMessageBox::information(this, "注册成功", "注册成功，请登录！");
        ui->passwordLineEdit->clear();
    } else {
        QMessageBox::warning(this, "注册失败", "用户名已存在或注册失败！");
    }
}

