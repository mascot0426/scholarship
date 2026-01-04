#include "registerwindow.h"
#include "ui_registerwindow.h"
#include <QMessageBox>
#include <QDebug>

RegisterWindow::RegisterWindow(Database *db, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RegisterWindow)
    , database(db)
    , registered(false)
{
    ui->setupUi(this);
    setWindowTitle("校园活动管理系统 - 注册");
    
    // 连接信号和槽
    connect(ui->registerButton, &QPushButton::clicked, this, &RegisterWindow::onRegisterButtonClicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &RegisterWindow::onCancelButtonClicked);
    
    // 设置密码输入框为密码模式
    ui->passwordLineEdit->setEchoMode(QLineEdit::Password);
    ui->confirmPasswordLineEdit->setEchoMode(QLineEdit::Password);
}

RegisterWindow::~RegisterWindow()
{
    delete ui;
}

bool RegisterWindow::validateInput()
{
    QString name = ui->nameLineEdit->text().trimmed();
    QString studentId = ui->studentIdLineEdit->text().trimmed();
    QString password = ui->passwordLineEdit->text();
    QString confirmPassword = ui->confirmPasswordLineEdit->text();
    
    // 检查姓名
    if (name.isEmpty()) {
        QMessageBox::warning(this, "注册失败", "请输入姓名！");
        ui->nameLineEdit->setFocus();
        return false;
    }
    
    // 检查学工号
    if (studentId.isEmpty()) {
        QMessageBox::warning(this, "注册失败", "请输入学工号！");
        ui->studentIdLineEdit->setFocus();
        return false;
    }
    
    // 检查学工号是否已存在
    if (database->studentIdExists(studentId)) {
        QMessageBox::warning(this, "注册失败", "该学工号已存在，请使用其他学工号！");
        ui->studentIdLineEdit->setFocus();
        ui->studentIdLineEdit->selectAll();
        return false;
    }
    
    // 检查密码
    if (password.isEmpty()) {
        QMessageBox::warning(this, "注册失败", "请输入密码！");
        ui->passwordLineEdit->setFocus();
        return false;
    }
    
    if (password.length() < 6) {
        QMessageBox::warning(this, "注册失败", "密码长度至少6位！");
        ui->passwordLineEdit->setFocus();
        ui->passwordLineEdit->selectAll();
        return false;
    }
    
    // 检查确认密码
    if (confirmPassword.isEmpty()) {
        QMessageBox::warning(this, "注册失败", "请确认密码！");
        ui->confirmPasswordLineEdit->setFocus();
        return false;
    }
    
    if (password != confirmPassword) {
        QMessageBox::warning(this, "注册失败", "两次输入的密码不一致！");
        ui->confirmPasswordLineEdit->setFocus();
        ui->confirmPasswordLineEdit->selectAll();
        return false;
    }
    
    return true;
}

void RegisterWindow::onRegisterButtonClicked()
{
    if (!validateInput()) {
        return;
    }
    
    QString name = ui->nameLineEdit->text().trimmed();
    QString studentId = ui->studentIdLineEdit->text().trimmed();
    QString password = ui->passwordLineEdit->text();
    
    // 默认注册为学生角色
    if (database->addUser(studentId, password, UserRole::Student, name)) {
        registeredStudentId = studentId;
        registered = true;
        QMessageBox::information(this, "注册成功", "注册成功！请使用学工号和密码登录。");
        accept(); // 关闭对话框并返回QDialog::Accepted
    } else {
        QMessageBox::warning(this, "注册失败", "注册失败，请重试！");
    }
}

void RegisterWindow::onCancelButtonClicked()
{
    reject(); // 关闭对话框并返回QDialog::Rejected
}





