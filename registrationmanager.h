#ifndef REGISTRATIONMANAGER_H
#define REGISTRATIONMANAGER_H

#include <QWidget>
#include "database.h"

QT_BEGIN_NAMESPACE
class QTableWidget;
class QPushButton;
class QLabel;
class QComboBox;
QT_END_NAMESPACE

class RegistrationManager : public QWidget
{
    Q_OBJECT

public:
    explicit RegistrationManager(Database *db, UserRole role, const QString &username, QWidget *parent = nullptr);
    void refreshRegistrations();

private slots:
    void onRegisterActivity();
    void onCancelRegistration();
    void onViewWaitlist();
    void onExportCSV();
    void onRegistrationSelectionChanged();

private:
    Database *database;
    UserRole userRole;
    QString currentUsername;
    QString currentStudentId;
    QString currentStudentName;
    
    QTableWidget *registrationsTable;
    QPushButton *registerButton;
    QPushButton *cancelButton;
    QPushButton *waitlistButton;
    QPushButton *exportButton;
    QPushButton *selectActivityButton;
    QComboBox *activityComboBox;
    QLabel *statusLabel;
    
    void setupUI();
    void populateTable();
    int getSelectedActivityId();
    void showConflictDialog(const QList<QHash<QString, QVariant>> &conflicts);
};

#endif // REGISTRATIONMANAGER_H

