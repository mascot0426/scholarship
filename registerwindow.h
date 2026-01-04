#ifndef REGISTERWINDOW_H
#define REGISTERWINDOW_H

#include <QDialog>
#include "database.h"

QT_BEGIN_NAMESPACE
namespace Ui { class RegisterWindow; }
QT_END_NAMESPACE

class RegisterWindow : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterWindow(Database *db, QWidget *parent = nullptr);
    ~RegisterWindow();

    QString getRegisteredStudentId() const { return registeredStudentId; }
    bool isRegistered() const { return registered; }

private slots:
    void onRegisterButtonClicked();
    void onCancelButtonClicked();

private:
    bool validateInput();
    Ui::RegisterWindow *ui;
    Database *database;
    QString registeredStudentId;
    bool registered;
};

#endif // REGISTERWINDOW_H







