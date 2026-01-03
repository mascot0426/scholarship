#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QDialog>
#include "database.h"

QT_BEGIN_NAMESPACE
namespace Ui { class LoginWindow; }
QT_END_NAMESPACE

class LoginWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LoginWindow(Database *db, QWidget *parent = nullptr);
    ~LoginWindow();

    UserRole getLoggedInRole() const { return loggedInRole; }
    QString getLoggedInUsername() const { return loggedInUsername; }
    QString getLoggedInName() const { return loggedInName; }
    bool isLoggedIn() const { return loggedIn; }

private slots:
    void onLoginButtonClicked();
    void onRegisterButtonClicked();

private:
    Ui::LoginWindow *ui;
    Database *database;
    UserRole loggedInRole;
    QString loggedInUsername;
    QString loggedInName;
    bool loggedIn;
};

#endif // LOGINWINDOW_H

