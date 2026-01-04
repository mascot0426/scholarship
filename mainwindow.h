#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "database.h"
#include "loginwindow.h"
#include "activitymanager.h"
#include "registrationmanager.h"
#include "networkmanager.h"
#include "conflictchecker.h"
#include "exportthread.h"

QT_BEGIN_NAMESPACE
class QTabWidget;
class QMenuBar;
class QStatusBar;
class QLabel;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLogin();
    void onLogout();
    void onExportStatistics();
    void onFetchCategories();
    void onFetchAnnouncements();
    void onCategoriesReceived(const QStringList &categories);
    void onAnnouncementsReceived(const QList<QHash<QString, QString>> &announcements);
    void onNetworkError(const QString &error);
    void onNewWindow();
private:
    void setupUI();
    void setupMenuBar();
    void setupNetworkConnections();
    void showLoginWindow();
    void updateUIForRole();
    
    Database *database;
    LoginWindow *loginWindow;
    ActivityManager *activityManager;
    RegistrationManager *registrationManager;
    NetworkManager *networkManager;
    ConflictChecker *conflictChecker;
    ExportThread *exportThread;
    
    QTabWidget *tabWidget;
    QLabel *statusLabel;
    QLabel *userLabel;
    
    UserRole currentRole;
    QString currentStudentId;
    QString currentName;
    bool isLoggedIn;
};

#endif // MAINWINDOW_H
