#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QDialog>
#include "database.h"

QT_BEGIN_NAMESPACE
namespace Ui { class LoginWindow; }
class RegisterWindow;
QT_END_NAMESPACE

class LoginWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LoginWindow(Database *db, QWidget *parent = nullptr);
    ~LoginWindow();

    UserRole getLoggedInRole() const { return loggedInRole; }
    QString getLoggedInStudentId() const { return loggedInStudentId; }
    QString getLoggedInName() const { return loggedInName; }
    bool isLoggedIn() const { return loggedIn; }

private slots:
    void onLoginButtonClicked();
    void onRegisterButtonClicked();
    void onNewWindowButtonClicked();  // 新增这一行
private:
    Ui::LoginWindow *ui;
    Database *database;
    UserRole loggedInRole;
    QString loggedInStudentId;
    QString loggedInName;
    bool loggedIn;
};

#endif // LOGINWINDOW_H

